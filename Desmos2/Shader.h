#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class Shader {
public:
    unsigned int ID; 

    Shader(const char* vertexPath, const char* fragmentPath);
    void use();
    void setMat4(const char* name, const glm::mat4& mat);
    void setVec3(const char* name, const glm::vec3& vec);
    void setFloat(const std::string& name, float value) const;

private:
    std::string readFile(const char* filepath);
    unsigned int compileShader(const char* source, unsigned int type, const std::string& typeName);
    void checkCompileErrors(unsigned int shader, const std::string& type);
};

