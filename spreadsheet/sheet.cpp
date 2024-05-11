#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::Sheet()
    : rows_(Position::MAX_ROWS)
    , cols_(Position::MAX_COLS)
{
};

void Sheet::SetCell(Position pos, std::string text) {
    PositionValidate(pos);
    bool is_exists = cells_.count(pos) == 1;
    if (!is_exists) {
        SetCellPosition(pos);
        cells_[pos] = std::make_unique<Cell>(*this);
    }
    try {
        cells_[pos]->Set(std::move(text));
    }
    catch (CircularDependencyException& e) {
        // Если ячейки не было в таблице, удаляяем её
        if (!is_exists) {
            ClearCell(pos);
        }
        throw CircularDependencyException(e);
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    PositionValidate(pos);
    if (cells_.count(pos) != 0) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

CellInterface* Sheet::GetCell(Position pos) {
    PositionValidate(pos);
    if (cells_.count(pos) != 0) {
        return cells_.at(pos).get();
    }
    return nullptr;
}

void Sheet::ClearCell(Position pos) {
    PositionValidate(pos);
    if (cells_.count(pos) == 0) {
        return;
    }
    cells_.erase(pos);
    ClearCellPosition(pos);
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
    return reinterpret_cast<const Cell*>(GetCell(pos));
}

Cell* Sheet::GetConcreteCell(Position pos) {
    return reinterpret_cast<Cell*>(GetCell(pos));
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

void Sheet::PositionValidate(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Invalid cell position: {"s + std::to_string(pos.row) + std::to_string(pos.col) + "}"s);
    }
}

void Sheet::SetCellPosition(Position pos) {
    ++rows_[pos.row];
    ++cols_[pos.col];
    size_.rows = pos.row + 1 > size_.rows ? pos.row + 1 : size_.rows;
    size_.cols = pos.col + 1 > size_.cols ? pos.col + 1 : size_.cols;
}

void Sheet::ClearCellPosition(Position pos) {
    --rows_[pos.row];
    --cols_[pos.col];

    if (size_.rows == pos.row + 1 || size_.cols == pos.col + 1) {
        size_ = GetActualSize();
    }
}

Size Sheet::GetActualSize() const {
    Size actual_size = { 0, 0 };
    for (int row = size_.rows; row >= 0; --row) {
        if (rows_[row] > 0) {
            actual_size.rows = row + 1;
            break;
        }
    }
    for (int col = size_.rows; col >= 0; --col) {
        if (cols_[col] > 0) {
            actual_size.cols = col + 1;
            break;
        }
    }
    return actual_size;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}