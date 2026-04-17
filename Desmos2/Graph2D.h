#pragma once
#include <glad/glad.h>
#include <vector>
#include <functional>

class Graph2D {
public:
    Graph2D();
    ~Graph2D();
    void generatePoints(const std::function<float(float)>& func, float xMin, float xMax, int points);
    void render();

private:
    unsigned int VAO;
    unsigned int VBO;
    int numPoints;
    std::vector<float> vertices;
    
};