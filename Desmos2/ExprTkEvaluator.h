#pragma once
#include <string>
#include "dependencies\libs\exprtk.hpp"
class ExprTkEvaluator {
private:
    typedef double T;
    exprtk::symbol_table<T> symbol_table;
    exprtk::expression<T> expression;
    exprtk::parser<T> parser;
    T x;
    bool isCompiled = false;

public:
    ExprTkEvaluator() : x(0.0) {
        symbol_table.add_variable("x", x);
        expression.register_symbol_table(symbol_table);
    }

    bool compile(const std::string& expr_str) {
        isCompiled = parser.compile(expr_str, expression);
        return isCompiled;
    }

    T evaluate(double x_val) {
        x = x_val;
        return expression.value();
    }

    std::string getError() const {
        return parser.error();
    }

    bool isValid() const {
        return isCompiled;
    }
};