#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <fstream>
#include <iostream>

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode = readFile(vertexPath);
    std::string fragmentCode = readFile(fragmentPath);

    if (vertexCode.empty() || fragmentCode.empty()) {
        std::cout << "ERROR: Failed to read shader files!" << std::endl;
        ID = 0;
        return;
    }

    unsigned int vertex = compileShader(vertexCode.c_str(), GL_VERTEX_SHADER, "VERTEX");
    unsigned int fragment = compileShader(fragmentCode.c_str(), GL_FRAGMENT_SHADER, "FRAGMENT");

    if (vertex == 0 || fragment == 0) {
        ID = 0;
        return;
    }

    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);

    checkCompileErrors(ID, "PROGRAM");

    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use() {
    glUseProgram(ID);
}

void Shader::setMat4(const char* name, const glm::mat4& mat) {
    glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setVec3(const char* name, const glm::vec3& vec) {
    glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vec));
}

std::string Shader::readFile(const char* filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        std::cout << "ERROR: Could not open file: " << filepath << std::endl;
        return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());
    return content;
}

unsigned int Shader::compileShader(const char* source, unsigned int type, const std::string& typeName) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    checkCompileErrors(shader, typeName);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        return 0;
    }

    return shader;
}

void Shader::checkCompileErrors(unsigned int shader, const std::string& type) {
    GLint success;
    char infoLog[1024];

    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n"
                << infoLog << "\n" << std::endl;
        }
    }
    else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n"
                << infoLog << "\n" << std::endl;
        }
    }
}