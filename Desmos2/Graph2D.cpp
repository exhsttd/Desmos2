#include "Graph2D.h"

Graph2D::Graph2D() : numPoints(0) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Graph2D::~Graph2D() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Graph2D::generatePoints(const std::function<float(float)>& func, float xMin, float xMax, int points) {
    std::vector<float> newVertices = generatePointsInMemory(func, xMin, xMax, points);
    uploadToGPU(newVertices);
}

std::vector<float> Graph2D::generatePointsInMemory(const std::function<float(float)>& func,
    float xMin, float xMax, int points) {
    std::vector<float> newVertices;
    newVertices.reserve(points * 2);  

    float step = (xMax - xMin) / (points - 1);

    for (int i = 0; i < points; i++) {
        float x = xMin + i * step;
        float y = func(x);

        newVertices.push_back(x);
        newVertices.push_back(y);
    }

    return newVertices;
}

void Graph2D::uploadToGPU(const std::vector<float>& newVertices) {
    numPoints = (int)(newVertices.size() / 2);
    vertices = newVertices;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
        vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Graph2D::render() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_STRIP, 0, numPoints);
    glBindVertexArray(0);
}