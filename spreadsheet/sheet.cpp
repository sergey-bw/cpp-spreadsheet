#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
    if (static_cast<size_t>(pos.row) >= cells_.size()) {
        cells_.resize(static_cast<size_t>(pos.row) + 1);
    }
    if (static_cast<size_t>(pos.col) >= cells_[pos.row].size()) {
        cells_[pos.row].resize(static_cast<size_t>(pos.col) + 1);
    }
    cells_[pos.row][pos.col] = std::make_unique<Cell>(*this);
    cells_[pos.row][pos.col]->Set(std::move(text));

    // Если Cell::Set выбросит исключение, размер печатной области не увеличется
    size_.rows = pos.row + 1 > size_.rows ? pos.row + 1 : size_.rows;
    size_.cols = pos.col + 1 > size_.cols ? pos.col + 1 : size_.cols;

}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
    return reinterpret_cast<const CellInterface*>(GetConcreteCell(pos));
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
    return reinterpret_cast<CellInterface*>(GetConcreteCell(pos));
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
    if (cells_.size() > static_cast<size_t>(pos.row) && cells_[pos.row].size() > static_cast<size_t>(pos.col)) {
        cells_[pos.row][pos.col].reset();
        size_ = GetActualSize();
    }
}

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) {
        auto value = cell.GetValue();
        if (std::holds_alternative<double>(value)) {
            output << std::get<double>(value);
        }
        else if (std::holds_alternative<std::string>(value)) {
            output << std::get<std::string>(value);
        }
        else {
            output << std::get<FormulaError>(value);
        }
        });
}

void Sheet::PrintTexts(std::ostream& output) const {
    PrintCells(output, [&output](const CellInterface& cell) {
        output << cell.GetText();
        });
}

const Cell* Sheet::GetConcreteCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
    if (cells_.size() > static_cast<size_t>(pos.row) && cells_[pos.row].size() > static_cast<size_t>(pos.col)) {
        return cells_[pos.row][pos.col].get();
    }
    return nullptr;
}

Cell* Sheet::GetConcreteCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
    if (cells_.size() > static_cast<size_t>(pos.row) && cells_[pos.row].size() > static_cast<size_t>(pos.col)) {
        return cells_[pos.row][pos.col].get();
    }
    return nullptr;
}

void Sheet::PrintCells(std::ostream& output,
    const std::function<void(const CellInterface&)>& printCell) const {

    Size size = GetPrintableSize();
    for (int row = 0; row < size.rows; ++row) {
        bool is_first = true;
        for (int col = 0; col < size.cols; ++col) {
            if (!is_first) {
                output << '\t';
            }
            is_first = false;
            if (auto cell = GetCell({ row, col }); cell != nullptr) {
                printCell(*cell);
            }
        }
        output << '\n';
    }
}

Size Sheet::GetActualSize() const {
    // rows - первый ряд, в котором есть элемент
    // cols - максимальный индекс колонки из всех рядов
    // В каждом ряду количество колонок может отличаться
    Size actual_size = { 0, 0 };
    for (int row = size_.rows - 1; row >= 0; --row) {
        for (int col = size_.cols - 1; col >= 0; --col) {
            if (cells_[row].size() > static_cast<size_t>(col) && cells_[row][col] != nullptr) {
                if (actual_size.rows == 0) {
                    actual_size.rows = row + 1;
                }
                if (col + 1 > actual_size.cols) {
                    actual_size.cols = col + 1;
                }
            }
        }
    }
    return actual_size;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}