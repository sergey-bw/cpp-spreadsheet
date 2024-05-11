#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <vector>

class Sheet : public SheetInterface {
public:
    Sheet();
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    const Cell* GetConcreteCell(Position pos) const;
    Cell* GetConcreteCell(Position pos);

private:
    void PrintCells(std::ostream& output,
        const std::function<void(const CellInterface&)>& printCell) const;

    void PositionValidate(Position pos) const;

    void SetCellPosition(Position pos);
    void ClearCellPosition(Position pos);
    Size GetActualSize() const;

    struct PositionHasher {
        size_t operator()(const Position& pos) const {
            return static_cast<size_t>(pos.row) + static_cast<size_t>(pos.col) * Position::MAX_ROWS * 10;
        }
    };

    std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher> cells_;

    std::vector<int> rows_;
    std::vector<int> cols_;
    Size size_;
};
