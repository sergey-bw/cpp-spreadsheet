#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <unordered_map>
#include <vector>

class Sheet : public SheetInterface {
public:
    Sheet() = default;
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

    Size GetActualSize() const;

    std::vector<std::vector<std::unique_ptr<Cell>>> cells_;
    Size size_;
};
