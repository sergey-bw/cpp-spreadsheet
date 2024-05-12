#include "cell.h"

#include "sheet.h"

#include <cassert>
#include <deque>
#include <iostream>
#include <string>
#include <optional>

/**
* Class Cell
*/

Cell::Cell(Sheet& sheet)
	: sheet_(sheet)
{
}

void Cell::Set(std::string text) {
	// Если ячейка существует и не меняется, ничего не делаем
	if (impl_ && text == GetText()) {
		return;
	}

	std::unique_ptr<Impl> new_impl;
	if (text.empty()) {
		new_impl = std::make_unique<EmptyImpl>();
	}
	else if (text[0] == FORMULA_SIGN && text.size() > 1) {
		new_impl = std::make_unique<FormulaImpl>(std::move(text.erase(0, 1)), sheet_);
	}
	else {
		new_impl = std::make_unique<TextImpl>(std::move(text));
	}

	if (WouldIntroduceCircularDependency(*new_impl)) {
		using namespace std::literals;
		throw CircularDependencyException("Circular Dependency Exception"s);
	}

	impl_ = std::move(new_impl);
	UpdateRefs();
	InvalidateCacheRecursive(true);
}

void Cell::Clear() {
	assert(impl_);
	impl_ = std::make_unique<EmptyImpl>();
	InvalidateCacheRecursive(true);
	UpdateRefs();
}

Cell::Value Cell::GetValue() const {
	assert(impl_);
	if (!cache_.has_value()) {
		cache_ = impl_->GetValue();
	}
	return cache_.value();
}

std::string Cell::GetText() const {
	assert(impl_);
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	assert(impl_);
	return impl_->GetReferencedCells();
}

bool Cell::HasCache() const {
	return cache_.has_value();
}

void Cell::ClearCache() {
	cache_.reset();
}

bool Cell::WouldIntroduceCircularDependency(const Impl& new_impl) const {
	if (new_impl.GetReferencedCells().empty()) {
		return false;
	}

	std::deque<const Cell*> ref_to_cells;
	for (const auto& pos : new_impl.GetReferencedCells()) {
		// Если еще нет ячейки, на которую ссылается, пропускаем
		// Создадим ее при обновлении связей
		if (sheet_.GetConcreteCell(pos) == nullptr) {
			continue;
		}
		ref_to_cells.push_back(sheet_.GetConcreteCell(pos));
	}

	// В начале дека проверяем элементы на циклицескую ссылку
	// В конец дека добавляем дочерние узлы
	std::unordered_set<const Cell*> visited_cells;
	while (!ref_to_cells.empty()) {
		if (ref_to_cells.front() != nullptr && visited_cells.count(ref_to_cells.front()) == 0) {
			if (ref_to_cells.front() == this) {
				return true;
			}
			visited_cells.insert(ref_to_cells.front());
			ref_to_cells.insert(ref_to_cells.end()
				, ref_to_cells.front()->ref_to_cells_.begin()
				, ref_to_cells.front()->ref_to_cells_.end());
		}
		ref_to_cells.pop_front();
	}
	return false;
}

void Cell::InvalidateCacheRecursive(bool force) {
	if (force || HasCache()) {
		ClearCache();
	}
	for (Cell* cell_ptr : ref_from_cells_) {
		cell_ptr->InvalidateCacheRecursive(force);
	}
}

void Cell::UpdateRefs() {
	// Удаляем старые связи
	for (Cell* cell_ptr : ref_to_cells_) {
		cell_ptr->ref_from_cells_.erase(this);
	}
	ref_to_cells_.clear();

	// Строим новые
	for (auto& ref_pos : GetReferencedCells()) {
		Cell* cell_ptr = sheet_.GetConcreteCell(ref_pos);
		// Если нет ячейки, создаем пустую для организации связей
		if (!cell_ptr) {
			sheet_.SetCell(ref_pos, {});
			cell_ptr = sheet_.GetConcreteCell(ref_pos);
		}
		cell_ptr->ref_from_cells_.insert(this);
		ref_to_cells_.insert(cell_ptr);
	}
}

/**
* Class Cell::EmptyImpl
*/

CellInterface::Value Cell::EmptyImpl::GetValue() const {
	return std::string{};
}

std::string Cell::EmptyImpl::GetText() const {
	return {};
}

/**
* Class Cell::TextImpl
*/

Cell::TextImpl::TextImpl(std::string text)
	: text_(text)
{
}

CellInterface::Value Cell::TextImpl::GetValue() const {
	if (text_[0] == ESCAPE_SIGN) {
		return text_.size() > 1 ? text_.substr(1) : std::string{};
	}
	return text_;
}

std::string Cell::TextImpl::GetText() const {
	return text_;
}

/**
* Class Cell::FormulaImpl
*/

Cell::FormulaImpl::FormulaImpl(std::string text, const SheetInterface& sheet)
	: formula_(ParseFormula(std::move(text)))
	, sheet_(sheet)
{
}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
	FormulaInterface::Value value = formula_->Evaluate(sheet_);
	if (std::holds_alternative<double>(value)) {
		return std::get<double>(value);
	}
	return std::get<FormulaError>(value);
}

std::string Cell::FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_->GetReferencedCells();
}