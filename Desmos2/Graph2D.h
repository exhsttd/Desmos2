#ifndef GRAPH2D_H
#define GRAPH2D_H

#include <glad/glad.h>
#include <vector>
#include <functional>

class Graph2D {
public:
    Graph2D();
    ~Graph2D();

    void generatePoints(const std::function<float(float)>& func, float xMin, float xMax, int points);
    std::vector<float> generatePointsInMemory(const std::function<float(float)>& func,
        float xMin, float xMax, int points);
    void uploadToGPU(const std::vector<float>& newVertices);

    void render();

private:
    unsigned int VAO;
    unsigned int VBO;
    int numPoints;
    std::vector<float> vertices;
};

#endif