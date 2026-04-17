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
#include "dependencies\libs\exprtk.hpp"
#include "Graph2D.h"
#include "Shader.h"
#include "Camera.h"


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

Camera camera;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            camera.dragging = true;
            glfwGetCursorPos(window, &camera.lastX, &camera.lastY);
        }
        else {
            camera.dragging = false;
        }
    }
}

void cursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (camera.dragging) {
        double dx = xpos - camera.lastX;
        double dy = ypos - camera.lastY;

        camera.offsetX -= dx * 0.01f * camera.scale;
        camera.offsetY += dy * 0.01f * camera.scale;

        camera.lastX = xpos;
        camera.lastY = ypos;
    }
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    camera.scale *= (yoffset > 0) ? 0.9f : 1.1f;
    if (camera.scale < 0.1f) camera.scale = 0.1f;
    if (camera.scale > 10.0f) camera.scale = 10.0f;
}

void updateGraphFromExpression(Graph2D& graph, const std::string& expression,
    float xMin, float xMax, int points,
    bool& showErrorPopup, std::string& errorMessage) {
    ExprTkEvaluator evaluator;

    if (evaluator.compile(expression)) {
        graph.generatePoints([expression](float x) -> float {
            ExprTkEvaluator eval;
            if (eval.compile(expression)) {
                double result = eval.evaluate(x);
                if (std::isinf(result) || std::isnan(result)) {
                    return 0.0f;
                }
                return (float)result;
            }
            return 0.0f;
            }, xMin, xMax, points);

        std::cout << "Graph updated successfully: " << expression << std::endl;
        showErrorPopup = false;
    }
    else {
        errorMessage = evaluator.getError();
        showErrorPopup = true;
        std::cout <<  "Parse error : " << errorMessage << std::endl;
    }
}




int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "2D Graph Calculator", NULL, NULL);
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

    Shader shader("shaders/simple.vert", "shaders/simple.frag");

    unsigned int gridVAO, gridVBO;
    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);

    float gridVertices[] = {
        -100.0f, 0.0f,   
        100.0f, 0.0f,
        0.0f, -100.0f,   
        0.0f, 100.0f
    };

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gridVertices), gridVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    Graph2D graph1, graph2;
    graph1.generatePoints([](float x) { return sin(x); }, -10.0f, 10.0f, 500);
    graph2.generatePoints([](float x) { return cos(x) * 2.0f; }, -10.0f, 10.0f, 500);

    glm::vec3 color1(1.0f, 0.5f, 0.0f); 
    glm::vec3 color2(0.0f, 0.8f, 1.0f); 

    bool showDemo = false;
    char funcInput1[256] = "sin(x)";
    char funcInput2[256] = "cos(x)*2";

    bool showErrorPopup1 = false;
    bool showErrorPopup2 = false;
    std::string errorMessage1;
    std::string errorMessage2;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Graph Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Camera Controls:");
        ImGui::Separator();

        ImGui::Text("Position: (%.2f, %.2f)", camera.offsetX, camera.offsetY);
        ImGui::Text("Scale: %.2f", camera.scale);
        ImGui::Separator();

        ImGui::Text("Graph 1");
        ImGui::ColorEdit3("Color 1", glm::value_ptr(color1));
        ImGui::InputText("Function 1", funcInput1, IM_ARRAYSIZE(funcInput1));

        if (ImGui::Button("Update Graph 1", ImVec2(-1, 0))) {
            std::string expr(funcInput1);
            updateGraphFromExpression(graph1, expr, -10.0f, 10.0f, 500,
                showErrorPopup1, errorMessage1);
        }

        ImGui::Separator();

        ImGui::Text("Graph 2:");
        ImGui::ColorEdit3("Color 2", glm::value_ptr(color2));
        ImGui::InputText("Function 2", funcInput2, IM_ARRAYSIZE(funcInput2));

        if (ImGui::Button("Update Graph 2", ImVec2(-1, 0))) {
            std::string expr(funcInput2);
            updateGraphFromExpression(graph2, expr, -10.0f, 10.0f, 500,
                showErrorPopup2, errorMessage2);
        }

        ImGui::Separator();

        if (ImGui::Button("Reset View", ImVec2(-1, 0))) {
            camera.reset();
        }

        ImGui::Text("FPS: %.1f", io.Framerate);

        ImGui::End();

        if (showErrorPopup1) {
            ImGui::OpenPopup("Error in Graph 1");
        }

        if (ImGui::BeginPopupModal("Error in Graph 1", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to parse expression:");
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", errorMessage1.c_str());
            ImGui::Separator();
            ImGui::Text("Valid example: sin(x), cos(x)*2, x^2, sin(x)*cos(x)");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                showErrorPopup1 = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (showErrorPopup2) {
            ImGui::OpenPopup("Error in Graph 2");
        }

        if (ImGui::BeginPopupModal("Error in Graph 2", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to parse expression:");
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", errorMessage2.c_str());
            ImGui::Separator();
            ImGui::Text("Valid example: sin(x), cos(x)*2, x^2, sin(x)*cos(x)");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                showErrorPopup2 = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (showDemo)
            ImGui::ShowDemoWindow(&showDemo);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(-camera.offsetX, -camera.offsetY, 0.0f));

        glm::mat4 projection = glm::ortho(
            -10.0f * camera.scale, 10.0f * camera.scale,
            -7.2f * camera.scale, 7.2f * camera.scale,
            -1.0f, 1.0f
        );


        shader.use();
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setVec3("color", glm::vec3(0.3f, 0.3f, 0.3f));

        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, 4);
        glBindVertexArray(0);

        shader.setVec3("color", color1);
        graph1.render();

        shader.setVec3("color", color2);
        graph2.render();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)1280, (float)720));

        ImGui::Begin("Graph Overlay", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImDrawList* draw = ImGui::GetWindowDrawList();

        float left = -10.0f * camera.scale + camera.offsetX;
        float right = 10.0f * camera.scale + camera.offsetX;
        float bottom = -7.2f * camera.scale + camera.offsetY;
        float top = 7.2f * camera.scale + camera.offsetY;

        ImVec2 winPos = ImGui::GetWindowPos();
        ImVec2 winSize = ImGui::GetWindowSize();

        auto worldToScreen = [&](float x, float y) -> ImVec2 {
            float nx = (x - left) / (right - left);
            float ny = (y - bottom) / (top - bottom);
            return ImVec2(
                winPos.x + nx * winSize.x,
                winPos.y + (1.0f - ny) * winSize.y
            );
            };

        float stepX = powf(10.0f, floorf(log10f(right - left) - 0.5f));
        if (stepX < 0.1f) stepX = 0.1f;

        for (float x = ceilf(left / stepX) * stepX; x <= right; x += stepX) {
            if (fabs(x) < 0.001f) continue;
            ImVec2 pos = worldToScreen(x, 0.0f);
            char label[32];
            sprintf_s(label, "%.1f", x);
            draw->AddText(ImVec2(pos.x - 12, pos.y + 10), IM_COL32(220, 220, 220, 255), label);
        }

        float stepY = powf(10.0f, floorf(log10f(top - bottom) - 0.5f));
        if (stepY < 0.1f) stepY = 0.1f;

        for (float y = ceilf(bottom / stepY) * stepY; y <= top; y += stepY) {
            if (fabs(y) < 0.001f) continue;
            ImVec2 pos = worldToScreen(0.0f, y);
            char label[32];
            sprintf_s(label, "%.1f", y);
            draw->AddText(ImVec2(pos.x - 28, pos.y - 8), IM_COL32(220, 220, 220, 255), label);
        }

        ImVec2 xPos = worldToScreen(right * 0.92f, bottom * 0.1f);
        draw->AddText(ImVec2(xPos.x + 15, xPos.y + 15), IM_COL32(230, 230, 230, 255), "X");

        ImVec2 yPos = worldToScreen(left * 0.05f, top * 0.85f);
        draw->AddText(ImVec2(yPos.x - 25, yPos.y - 25), IM_COL32(230, 230, 230, 255), "Y");

        ImGui::End();

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