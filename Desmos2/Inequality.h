#pragma once

#include <string>
#include <vector>
#include <functional>
#include <glm/glm.hpp>

class Shader;

struct Inequality {
    std::string expression;
    std::function<bool(float, float)> condition;
    glm::vec3 color;
    float alpha;        
    bool enabled;
    bool isActive;

    Inequality() : enabled(true), isActive(false), alpha(0.3f) {}  
};

class InequalityRenderer {
public:
    InequalityRenderer();
    ~InequalityRenderer();

    void addInequality(const std::string& expr, const glm::vec3& color, float alpha = 0.3f); 
    void removeInequality(int index);
    void render(Shader& shader, float xMin, float xMax, float yMin, float yMax);

    int getCount() const { return (int)inequalities.size(); }
    Inequality& getInequality(int index) { return inequalities[index]; }
    void clear() { inequalities.clear(); meshes.clear(); }

private:
    struct InequalityMesh {
        unsigned int VAO = 0;
        unsigned int VBO = 0;
        int vertexCount = 0;
        glm::vec3 color;
        float alpha;      
        bool enabled = true;
    };

    std::vector<Inequality> inequalities;
    std::vector<InequalityMesh> meshes;

    void regenerateMesh(int index, float xMin, float xMax, float yMin, float yMax, int resolution);
    bool evaluateInequality(const Inequality& ineq, float x, float y);
};

bool isValidNumber(const std::string& s);