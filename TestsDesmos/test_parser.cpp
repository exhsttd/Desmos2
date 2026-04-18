#include <gtest/gtest.h>
#include <cmath>
#include <string>
#include "../Desmos2/ExprTkEvaluator.h" 


TEST(ParserTest, BasicFunctions) {
    ExprTkEvaluator evaluator;

    EXPECT_TRUE(evaluator.compile("sin(x)"));
    EXPECT_NEAR(evaluator.evaluate(0.0), 0.0, 1e-6);
    EXPECT_NEAR(evaluator.evaluate(3.14159), 0.0, 1e-4);
    EXPECT_NEAR(evaluator.evaluate(1.5708), 1.0, 1e-4);

    EXPECT_TRUE(evaluator.compile("cos(x)"));
    EXPECT_NEAR(evaluator.evaluate(0.0), 1.0, 1e-6);
    EXPECT_NEAR(evaluator.evaluate(3.14159), -1.0, 1e-4);

    EXPECT_TRUE(evaluator.compile("tan(x)"));
    EXPECT_NEAR(evaluator.evaluate(0.0), 0.0, 1e-6);
}


TEST(ParserTest, ArithmeticOperations) {
    ExprTkEvaluator evaluator;

    EXPECT_TRUE(evaluator.compile("x+2"));
    EXPECT_NEAR(evaluator.evaluate(3.0), 5.0, 1e-6);

    EXPECT_TRUE(evaluator.compile("x-5"));
    EXPECT_NEAR(evaluator.evaluate(10.0), 5.0, 1e-6);

    EXPECT_TRUE(evaluator.compile("3*x"));
    EXPECT_NEAR(evaluator.evaluate(4.0), 12.0, 1e-6);

    EXPECT_TRUE(evaluator.compile("x/2"));
    EXPECT_NEAR(evaluator.evaluate(10.0), 5.0, 1e-6);
}


TEST(ParserTest, ComplexExpressions) {
    ExprTkEvaluator evaluator;

    EXPECT_TRUE(evaluator.compile("sin(x) + cos(x)"));
    EXPECT_NEAR(evaluator.evaluate(0.0), 1.0, 1e-6);

    EXPECT_TRUE(evaluator.compile("pow(x,2)"));
    EXPECT_NEAR(evaluator.evaluate(5.0), 25.0, 1e-6);

    EXPECT_TRUE(evaluator.compile("sqrt(pow(x,2))"));
    EXPECT_NEAR(evaluator.evaluate(-5.0), 5.0, 1e-6);
}


TEST(ParserTest, ErrorHandling) {
    ExprTkEvaluator evaluator;

    EXPECT_FALSE(evaluator.compile("sin(x"));

    EXPECT_FALSE(evaluator.compile("unknown(x)"));

    EXPECT_FALSE(evaluator.compile(""));

    EXPECT_FALSE(evaluator.compile("xyi"));
}


TEST(ParserTest, EdgeCases) {
    ExprTkEvaluator evaluator;

    EXPECT_TRUE(evaluator.compile("1/x"));
    double result = evaluator.evaluate(0.0);
    EXPECT_TRUE(std::isinf(result) || std::isnan(result));

    EXPECT_TRUE(evaluator.compile("x*1000"));
    EXPECT_NEAR(evaluator.evaluate(1000000.0), 1e9, 1e6);

    EXPECT_TRUE(evaluator.compile("x"));
    EXPECT_NEAR(evaluator.evaluate(-10.0), -10.0, 1e-6);
}