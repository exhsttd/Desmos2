#include "Inequality.h"
#include "Shader.h"
#include "ExprTkEvaluator.h"
#include <glad/glad.h>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cctype>

int popcnt(int x) {
    int count = 0;
    for (int i = 0; i < 4; i++) {
        if (x & (1 << i)) count++;
    }
    return count;
}

bool isValidNumber(const std::string& s) {
    try {
        size_t pos;
        std::stof(s, &pos);
        return pos == s.length();
    }
    catch (...) {
        return false;
    }
}

InequalityRenderer::InequalityRenderer() {}

InequalityRenderer::~InequalityRenderer() {
    for (auto& mesh : meshes) {
        if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
        if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);
    }
}

bool InequalityRenderer::evaluateInequality(const Inequality& ineq, float x, float y) {
    if (!ineq.condition) return false;
    return ineq.condition(x, y);
}

void InequalityRenderer::addInequality(const std::string& expr, const glm::vec3& color, float alpha) {
    Inequality ineq;
    ineq.expression = expr;
    ineq.color = color;
    ineq.alpha = alpha;
    ineq.enabled = true;
    ineq.isActive = true;

    std::string exprTrimmed = expr;
    exprTrimmed.erase(std::remove_if(exprTrimmed.begin(), exprTrimmed.end(), ::isspace), exprTrimmed.end());

    std::string op;
    size_t opPos = std::string::npos;

    if (exprTrimmed.find(">=") != std::string::npos) {
        op = ">=";
        opPos = exprTrimmed.find(">=");
    }
    else if (exprTrimmed.find("<=") != std::string::npos) {
        op = "<=";
        opPos = exprTrimmed.find("<=");
    }
    else if (exprTrimmed.find(">") != std::string::npos) {
        op = ">";
        opPos = exprTrimmed.find(">");
    }
    else if (exprTrimmed.find("<") != std::string::npos) {
        op = "<";
        opPos = exprTrimmed.find("<");
    }
    else if (exprTrimmed.find("=") != std::string::npos) {
        op = "=";
        opPos = exprTrimmed.find("=");
    }

    if (opPos == std::string::npos) {
        std::cout << "Invalid inequality: " << expr << std::endl;
        return;
    }

    std::string leftExpr = exprTrimmed.substr(0, opPos);
    std::string rightExpr = exprTrimmed.substr(opPos + op.length());

    if ((leftExpr == "x" || leftExpr == "y") && isValidNumber(rightExpr)) {
        float val = std::stof(rightExpr);
        bool isX = (leftExpr == "x");

        if (op == ">") {
            if (isX) ineq.condition = [val](float x, float y) { return x > val; };
            else ineq.condition = [val](float x, float y) { return y > val; };
        }
        else if (op == ">=") {
            if (isX) ineq.condition = [val](float x, float y) { return x >= val; };
            else ineq.condition = [val](float x, float y) { return y >= val; };
        }
        else if (op == "<") {
            if (isX) ineq.condition = [val](float x, float y) { return x < val; };
            else ineq.condition = [val](float x, float y) { return y < val; };
        }
        else if (op == "<=") {
            if (isX) ineq.condition = [val](float x, float y) { return x <= val; };
            else ineq.condition = [val](float x, float y) { return y <= val; };
        }
    }
    else if ((rightExpr == "x" || rightExpr == "y") && isValidNumber(leftExpr)) {
        float val = std::stof(leftExpr);
        bool isX = (rightExpr == "x");

        if (op == ">") {
            if (isX) ineq.condition = [val](float x, float y) { return val > x; };
            else ineq.condition = [val](float x, float y) { return val > y; };
        }
        else if (op == ">=") {
            if (isX) ineq.condition = [val](float x, float y) { return val >= x; };
            else ineq.condition = [val](float x, float y) { return val >= y; };
        }
        else if (op == "<") {
            if (isX) ineq.condition = [val](float x, float y) { return val < x; };
            else ineq.condition = [val](float x, float y) { return val < y; };
        }
        else if (op == "<=") {
            if (isX) ineq.condition = [val](float x, float y) { return val <= x; };
            else ineq.condition = [val](float x, float y) { return val <= y; };
        }
    }
    else {
        ineq.condition = [expr](float x, float y) -> bool {
            return x > 0;
            };
        std::cout << "Complex inequality simplified: " << expr << std::endl;
    }

    inequalities.push_back(ineq);

    InequalityMesh mesh;
    mesh.alpha = alpha;
    meshes.push_back(mesh);

    std::cout << "Added inequality: " << expr << " with alpha: " << alpha << std::endl;
}

void InequalityRenderer::regenerateMesh(int index, float xMin, float xMax, float yMin, float yMax, int resolution) {
    if (index >= (int)inequalities.size() || index >= (int)meshes.size()) return;

    const auto& ineq = inequalities[index];
    auto& mesh = meshes[index];

    if (mesh.VAO) glDeleteVertexArrays(1, &mesh.VAO);
    if (mesh.VBO) glDeleteBuffers(1, &mesh.VBO);

    mesh.VAO = 0;
    mesh.VBO = 0;

    std::vector<float> vertices;

    float stepX = (xMax - xMin) / resolution;
    float stepY = (yMax - yMin) / resolution;

    for (int i = 0; i < resolution; i++) {
        for (int j = 0; j < resolution; j++) {
            float x0 = xMin + i * stepX;
            float y0 = yMin + j * stepY;
            float x1 = x0 + stepX;
            float y1 = y0 + stepY;

            bool c00 = evaluateInequality(ineq, x0, y0);
            bool c10 = evaluateInequality(ineq, x1, y0);
            bool c01 = evaluateInequality(ineq, x0, y1);
            bool c11 = evaluateInequality(ineq, x1, y1);

            int mask = (c00 ? 1 : 0) | (c10 ? 2 : 0) | (c01 ? 4 : 0) | (c11 ? 8 : 0);

            if (popcnt(mask) >= 3) {
                vertices.push_back(x0); vertices.push_back(y0);
                vertices.push_back(x1); vertices.push_back(y0);
                vertices.push_back(x1); vertices.push_back(y1);
                vertices.push_back(x0); vertices.push_back(y0);
                vertices.push_back(x1); vertices.push_back(y1);
                vertices.push_back(x0); vertices.push_back(y1);
            }
        }
    }

    if (vertices.empty()) {
        mesh.vertexCount = 0;
        return;
    }

    mesh.vertexCount = (int)(vertices.size() / 2);

    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    mesh.color = ineq.color;
    mesh.alpha = ineq.alpha;
    mesh.enabled = ineq.enabled;
}

void InequalityRenderer::render(Shader& shader, float xMin, float xMax, float yMin, float yMax) {
    const int resolution = 80;

    for (size_t i = 0; i < inequalities.size() && i < meshes.size(); i++) {
        if (!inequalities[i].enabled || !inequalities[i].isActive) continue;

        regenerateMesh((int)i, xMin, xMax, yMin, yMax, resolution);

        auto& mesh = meshes[i];
        if (mesh.vertexCount > 0 && mesh.VAO != 0) {
            shader.setVec3("color", inequalities[i].color);
            shader.setFloat("alpha", inequalities[i].alpha);

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            glBindVertexArray(mesh.VAO);
            glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);

            glBindVertexArray(0);
            glDisable(GL_BLEND);
        }
    }
}

void InequalityRenderer::removeInequality(int index) {
    if (index >= 0 && index < (int)inequalities.size()) {
        if (meshes[index].VAO) glDeleteVertexArrays(1, &meshes[index].VAO);
        if (meshes[index].VBO) glDeleteBuffers(1, &meshes[index].VBO);
        inequalities.erase(inequalities.begin() + index);
        meshes.erase(meshes.begin() + index);
    }
}