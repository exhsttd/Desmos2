#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>

struct Graph2D {
    unsigned int VAO, VBO;
    int numPoints;
    std::vector<float> vertices;

    Graph2D() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }

    ~Graph2D() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
    }

    void generatePoints(float (*func)(float), float xMin, float xMax, int points) {
        numPoints = points;
        vertices.clear();

        float step = (xMax - xMin) / (points - 1);

        for (int i = 0; i < points; i++) {
            float x = xMin + i * step;
            float y = func(x);
            float z = 0.0f;

            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
        }

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
            vertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }

    void render() {
        glBindVertexArray(VAO);
        glDrawArrays(GL_LINE_STRIP, 0, numPoints);
        glBindVertexArray(0);
    }
};

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath) {
        FILE* vFile = fopen(vertexPath, "rb");
        FILE* fFile = fopen(fragmentPath, "rb");

        if (!vFile || !fFile) {
            std::cout << "Failed to open shader files!" << std::endl;
            return;
        }

        fseek(vFile, 0, SEEK_END);
        long vSize = ftell(vFile);
        rewind(vFile);

        fseek(fFile, 0, SEEK_END);
        long fSize = ftell(fFile);
        rewind(fFile);

        char* vSource = new char[vSize + 1];
        char* fSource = new char[fSize + 1];

        fread(vSource, 1, vSize, vFile);
        fread(fSource, 1, fSize, fFile);

        vSource[vSize] = 0;
        fSource[fSize] = 0;

        fclose(vFile);
        fclose(fFile);

        unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vSource, NULL);
        glCompileShader(vertex);

        unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fSource, NULL);
        glCompileShader(fragment);

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);

        delete[] vSource;
        delete[] fSource;
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    void use() {
        glUseProgram(ID);
    }

    void setMat4(const char* name, const glm::mat4& mat) {
        glUniformMatrix4fv(glGetUniformLocation(ID, name), 1, GL_FALSE, glm::value_ptr(mat));
    }

    void setVec3(const char* name, const glm::vec3& vec) {
        glUniform3fv(glGetUniformLocation(ID, name), 1, glm::value_ptr(vec));
    }
};

float offsetX = 0.0f, offsetY = 0.0f;
float scale = 1.0f;
bool dragging = false;
double lastMouseX, lastMouseY;

float func1(float x) {
    return sin(x);
}

float func2(float x) {
    return cos(x) * 2.0f;
}

float func3(float x) {
    return x * x * 0.2f;
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            dragging = true;
            glfwGetCursorPos(window, &lastMouseX, &lastMouseY);
        }
        else {
            dragging = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (dragging) {
        double dx = xpos - lastMouseX;
        double dy = ypos - lastMouseY;

        offsetX -= dx * 0.01f * scale;
        offsetY += dy * 0.01f * scale;

        lastMouseX = xpos;
        lastMouseY = ypos;
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    scale *= (yoffset > 0) ? 0.9f : 1.1f;
    if (scale < 0.1f) scale = 0.1f;
    if (scale > 10.0f) scale = 10.0f;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "CHHHUUUUUUUVAAAAAAAAKKK", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetScrollCallback(window, scrollCallback);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Shader shader("./shaders/simple.vert", "./shaders/simple.frag");


    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    float gridVertices[] = {
        -100.0f, 0.0f, 0.0f,
        100.0f, 0.0f, 0.0f,
        0.0f, -100.0f, 0.0f,
        0.0f, 100.0f, 0.0f
    };

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Graph2D graph1, graph2;
    graph1.generatePoints(func1, -10.0f, 10.0f, 500);
    graph2.generatePoints(func2, -10.0f, 10.0f, 500);

    glm::vec3 color1(1.0f, 0.5f, 0.0f);
    glm::vec3 color2(0.0f, 0.8f, 1.0f);

    bool showDemo = false;
    char funcInput1[64] = "sin(x)";
    char funcInput2[64] = "cos(x)*2";

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Graph Controls");

        ImGui::Text("Camera Controls:");
        ImGui::Text("  Drag mouse - pan");
        ImGui::Text("  Scroll - zoom");
        ImGui::Separator();

        ImGui::Text("Camera Position: (%.2f, %.2f)", offsetX, offsetY);
        ImGui::Text("Scale: %.2f", scale);
        ImGui::Separator();

        ImGui::Text("Graph 1:");
        ImGui::ColorEdit3("Color 1", glm::value_ptr(color1));
        ImGui::InputText("Function 1", funcInput1, IM_ARRAYSIZE(funcInput1));
        if (ImGui::Button("Update Graph 1")) {
            if (strcmp(funcInput1, "sin(x)") == 0)
                graph1.generatePoints(func1, -10.0f, 10.0f, 500);
            else if (strcmp(funcInput1, "x^2/5") == 0)
                graph1.generatePoints(func3, -10.0f, 10.0f, 500);
        }

        ImGui::Separator();

        ImGui::Text("Graph 2:");
        ImGui::ColorEdit3("Color 2", glm::value_ptr(color2));
        ImGui::InputText("Function 2", funcInput2, IM_ARRAYSIZE(funcInput2));
        if (ImGui::Button("Update Graph 2")) {
            if (strcmp(funcInput2, "cos(x)*2") == 0)
                graph2.generatePoints(func2, -10.0f, 10.0f, 500);
        }

        ImGui::Separator();

        if (ImGui::Button("Reset View")) {
            offsetX = offsetY = 0.0f;
            scale = 1.0f;
        }

        ImGui::Checkbox("Show Demo", &showDemo);
        ImGui::Text("FPS: %.1f", io.Framerate);

        ImGui::End();

        if (showDemo)
            ImGui::ShowDemoWindow(&showDemo);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(-offsetX, -offsetY, 0.0f));

        glm::mat4 projection = glm::ortho(
            -10.0f * scale, 10.0f * scale,
            -7.2f * scale, 7.2f * scale,
            -1.0f, 1.0f
        );

        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setVec3("color", glm::vec3(0.3f, 0.3f, 0.3f)); 

        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, 4); // 4 вершины = 2 линии
        glBindVertexArray(0);

        shader.setVec3("color", color1);
        graph1.render();

        shader.setVec3("color", color2);
        graph2.render();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &gridVAO);
    glDeleteBuffers(1, &gridVBO);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}