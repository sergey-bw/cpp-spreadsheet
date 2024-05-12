#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    explicit Cell(Sheet& sheet);
    ~Cell() = default;

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    bool HasCache() const;
    void ClearCache();

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    bool WouldIntroduceCircularDependency(const Impl& new_impl) const;
    void InvalidateCacheRecursive(bool force = false);
    void UpdateRefs();

    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    mutable std::optional<CellInterface::Value> cache_;

    // Зависящие и зависимые ячейки
    std::unordered_set<Cell*> ref_to_cells_;
    std::unordered_set<Cell*> ref_from_cells_;
};

class Cell::Impl {
public:
    Impl() = default;
    ~Impl() = default;

    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;

    virtual std::vector<Position> GetReferencedCells() const {
        return {};
    }
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    Value GetValue() const override;
    std::string GetText() const override;
};

class Cell::TextImpl : public Cell::Impl {
public:
    TextImpl() = delete;
    explicit TextImpl(std::string text);

    Value GetValue() const override;
    virtual std::string GetText() const override;
private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    FormulaImpl() = delete;
    FormulaImpl(std::string text, const SheetInterface& sheet);

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;
};