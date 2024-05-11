#include "formula.h"
#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <set>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << "#ARITHM!";
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) try 
        : ast_(ParseFormulaAST(expression)) {
    } catch (const std::exception& e) {
        throw FormulaException(e.what());
    }



    Value Evaluate(const SheetInterface& sheet) const override {
        const CellLookup cell_lookup = [&sheet](const Position& pos)->double {
            if (!pos.IsValid()) {
                throw FormulaError(FormulaError::Category::Ref);
            }

            if (!sheet.GetCell(pos)) {
                return 0.;
            }

            CellInterface::Value value = sheet.GetCell(pos)->GetValue();
            if (std::holds_alternative<std::string>(value)) {
                if (std::get<std::string>(value).empty()) {
                    return 0;
                }
                double result = 0.;
                std::istringstream str_in{ std::get<std::string>(value) };
                if (!(str_in >> result) || !str_in.eof()) {
                    throw FormulaError(FormulaError::Category::Value);
                }
                return result;
            } else if (std::holds_alternative<double>(value)) {
                return std::get<double>(value);
            } else {
                throw FormulaError(std::get<FormulaError>(value));
            }
        };

        try {
            return ast_.Execute(cell_lookup);
        } catch (const FormulaError& err) {
            return err;
        }
    }

    std::string GetExpression() const override {
        std::ostringstream output;
        ast_.PrintFormula(output);
        return output.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::set<Position> sorted_unique_cells(ast_.GetCells().begin(), ast_.GetCells().end());
        return { sorted_unique_cells.begin(), sorted_unique_cells.end() };
    }

private:
    FormulaAST ast_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}

FormulaError::FormulaError(Category category)
    : category_(category)
{
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return GetCategory() == rhs.GetCategory();
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
        case Category::Ref:
            return "#REF!"sv;
        case Category::Value:
            return "#VALUE!"sv;
        case Category::Arithmetic:
            return "#ARITHM!"sv;
    }
    return "#UNDEFINED"sv;
}