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
#include <future>
#include <atomic>
#include <thread>
#include <chrono>
#include "Graph2D.h"
#include "Shader.h"
#include "Camera.h"
#include "ExprTkEvaluator.h"
#include "Inequality.h"

Camera camera;

int windowWidth = 1280;
int windowHeight = 720;

struct PendingUpdate {
    std::future<std::vector<float>> future;
    std::string expression;
    bool hasPending = false;
    bool showErrorPopup = false;
    std::string errorMessage;
};

PendingUpdate pending1, pending2;

InequalityRenderer inequalityRenderer;
char inequalityInput[256] = "x > 1";
glm::vec3 inequalityColor(0.2f, 0.8f, 0.2f);

void framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    std::cout << "Window resized to: " << width << " x " << height << std::endl;
}

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

std::vector<float> generatePointsInBackground(const std::string& expression,
    float xMin, float xMax, int points,
    bool& outSuccess, std::string& outError) {
    ExprTkEvaluator evaluator;
    std::vector<float> result;

    if (evaluator.compile(expression)) {
        result.reserve(points * 2);
        float step = (xMax - xMin) / (points - 1);

        for (int i = 0; i < points; i++) {
            float x = xMin + i * step;
            double y = evaluator.evaluate(x);

            if (std::isinf(y) || std::isnan(y)) {
                y = 0.0;
            }

            result.push_back(x);
            result.push_back((float)y);
        }

        outSuccess = true;
        outError = "";
        std::cout << "Background generation completed: " << expression << std::endl;
    }
    else {
        outSuccess = false;
        outError = evaluator.getError();
        std::cout << "Parse error: " << outError << std::endl;
    }

    return result;
}

void startBackgroundUpdate(Graph2D& graph, PendingUpdate& pending,
    const std::string& expression,
    float xMin, float xMax, int points) {
    if (pending.hasPending) {
        std::cout << "Update already in progress, please wait." << std::endl;
        return;
    }

    pending.expression = expression;
    pending.hasPending = true;
    pending.showErrorPopup = false;

    pending.future = std::async(std::launch::async,
        [expression, xMin, xMax, points]() -> std::vector<float> {
            bool success;
            std::string error;
            return generatePointsInBackground(expression, xMin, xMax, points, success, error);
        });
}

void processPendingUpdate(Graph2D& graph, PendingUpdate& pending,
    bool& showErrorPopup, std::string& errorMessage) {
    if (!pending.hasPending) return;

    auto status = pending.future.wait_for(std::chrono::milliseconds(0));

    if (status == std::future_status::ready) {

        try {
            std::vector<float> newVertices = pending.future.get();

            if (!newVertices.empty()) {
                graph.uploadToGPU(newVertices);
                showErrorPopup = false;
                std::cout << "Graph updated and uploaded to GPU!" << std::endl;
            }
            else {
                showErrorPopup = true;
                errorMessage = "Failed to parse expression";
            }
        }
        catch (const std::exception& e) {
            showErrorPopup = true;
            errorMessage = e.what();
        }

        pending.hasPending = false;
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
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
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
    ImGui::StyleColorsLight();
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
    graph1.generatePoints([](float x) { return sin(x); }, -100.0f, 100.0f, 2000);
    graph2.generatePoints([](float x) { return cos(x) * 2.0f; }, -100.0f, 100.0f, 2000);

    glm::vec3 color1(1.0f, 0.5f, 0.0f);
    glm::vec3 color2(0.0f, 0.8f, 1.0f);

    bool showDemo = false;
    char funcInput1[256] = "sin(x)";
    char funcInput2[256] = "cos(x)*2";

    bool showErrorPopup1 = false;
    bool showErrorPopup2 = false;
    std::string errorMessage1;
    std::string errorMessage2;

    float aspectRatio, baseHeight, baseWidth, currentXMin, currentXMax;

    while (!glfwWindowShouldClose(window)) {
        processPendingUpdate(graph1, pending1, showErrorPopup1, errorMessage1);
        processPendingUpdate(graph2, pending2, showErrorPopup2, errorMessage2);

        glfwPollEvents();

        aspectRatio = (float)windowWidth / (float)windowHeight;
        baseHeight = 9.0f * camera.scale;
        baseWidth = baseHeight * aspectRatio;
        currentXMin = -baseWidth + camera.offsetX;
        currentXMax = baseWidth + camera.offsetX;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(350, 0), ImGuiCond_FirstUseEver);
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
            startBackgroundUpdate(graph1, pending1, expr, currentXMin, currentXMax, 500);
        }

        if (pending1.hasPending) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "⏳");
        }

        ImGui::Separator();

        ImGui::Text("Graph 2:");
        ImGui::ColorEdit3("Color 2", glm::value_ptr(color2));
        ImGui::InputText("Function 2", funcInput2, IM_ARRAYSIZE(funcInput2));

        if (ImGui::Button("Update Graph 2", ImVec2(-1, 0))) {
            std::string expr(funcInput2);
            startBackgroundUpdate(graph2, pending2, expr, currentXMin, currentXMax, 500);
        }

        if (pending2.hasPending) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "⏳");
        }

        ImGui::Separator();

        if (ImGui::Button("Reset View", ImVec2(-1, 0))) {
            camera.reset();
        }

        ImGui::Separator();

        if (ImGui::Button("Help", ImVec2(-1, 0))) {
            ImGui::OpenPopup("Help");
        }

        if (ImGui::BeginPopupModal("Help", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "CAMERA CONTROLS");
            ImGui::BulletText("Pan the graph: hold left mouse button and drag");
            ImGui::BulletText("Zoom: scroll the mouse wheel");
            ImGui::BulletText("Reset view: press \"Reset View\" button");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "PLOTTING GRAPHS");
            ImGui::BulletText("Enter a mathematical expression in the \"Function\" field");
            ImGui::BulletText("Press \"Update Graph\" to plot");
            ImGui::BulletText("Each graph has its own color picker");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "INEQUALITIES & REGIONS");
            ImGui::BulletText("Enter inequality like: x > 1, y < -2, x >= 0, y <= 5");
            ImGui::BulletText("Press \"Add Inequality\" to highlight the region");
            ImGui::BulletText("Use checkbox to show/hide each region");
            ImGui::BulletText("Click color square to change region color");
            ImGui::BulletText("Click X to remove individual inequality");
            ImGui::BulletText("Press \"Clear All\" to remove all inequalities");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "SUPPORTED FUNCTIONS");
            ImGui::BulletText("sin(x), cos(x), tan(x) — trigonometric functions");
            ImGui::BulletText("pow(x,2) — exponentiation (instead of x^2)");
            ImGui::BulletText("sqrt(x), abs(x), exp(x)");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "IMPORTANT");
            ImGui::Text("Don't use ^ for exponentiation! Use pow(x,2) instead");
            ImGui::Text("Supported inequalities: x > value, x < value, y > value, y < value");

            ImGui::Separator();
            if (ImGui::Button("Close", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Separator();

        // неравенства(области выделения)
        ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "INEQUALITIES");
        ImGui::InputText("##inequality", inequalityInput, IM_ARRAYSIZE(inequalityInput));
        if (ImGui::Button("Add Inequality", ImVec2(-1, 0))) {
            inequalityRenderer.addInequality(std::string(inequalityInput), inequalityColor, 0.3f);
        }
        ImGui::Text("Active inequalities:");
        for (int i = 0; i < inequalityRenderer.getCount(); i++) {
            auto& ineq = inequalityRenderer.getInequality(i);
            ImGui::PushID(i);
            bool enabled = ineq.enabled;
            if (ImGui::Checkbox("##enabled", &enabled)) {
                ineq.enabled = enabled;
            }
            ImGui::SameLine();

            ImGui::ColorEdit3("##color", glm::value_ptr(ineq.color), ImGuiColorEditFlags_NoInputs);
            ImGui::SameLine();

            ImGui::Text("%s", ineq.expression.c_str());
            ImGui::SameLine();

            if (ImGui::Button("X")) {
                inequalityRenderer.removeInequality(i);
                ImGui::PopID();
                break;
            }

            ImGui::PopID();
        }

        if (ImGui::Button("Clear All Inequalities", ImVec2(-1, 0))) {
            inequalityRenderer.clear();
        }

        ImGui::Separator();

        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::End();

        if (showErrorPopup1) {
            ImGui::OpenPopup("Error in Graph 1");
        }

        if (ImGui::BeginPopupModal("Error in Graph 1", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to parse expression:");
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", errorMessage1.c_str());
            ImGui::Separator();
            ImGui::Text("Valid example: sin(x), cos(x)*2, pow(x,2), sin(x)*cos(x)");
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
            ImGui::Text("Valid example: sin(x), cos(x)*2, pow(x,2), sin(x)*cos(x)");
            if (ImGui::Button("OK", ImVec2(120, 0))) {
                showErrorPopup2 = false;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (showDemo)
            ImGui::ShowDemoWindow(&showDemo);

        // отрисовка
        glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(-camera.offsetX, -camera.offsetY, 0.0f));

        glm::mat4 projection = glm::ortho(
            -baseWidth, baseWidth,
            -baseHeight, baseHeight,
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

        // Отрисовка неравенств
        inequalityRenderer.render(shader,
            -baseWidth + camera.offsetX,
            baseWidth + camera.offsetX,
            -baseHeight + camera.offsetY,
            baseHeight + camera.offsetY
        );

        // Graph Overlay
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)windowWidth, (float)windowHeight));

        ImGui::Begin("Graph Overlay", nullptr,
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoBringToFrontOnFocus);

        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Для разметки осей используем отдельные переменные (не перезаписываем baseHeight для графиков)
        float overlayBaseHeight = 7.2f * camera.scale;
        float overlayBaseWidth = overlayBaseHeight * aspectRatio;

        float left = -overlayBaseWidth + camera.offsetX;
        float right = overlayBaseWidth + camera.offsetX;
        float bottom = -overlayBaseHeight + camera.offsetY;
        float top = overlayBaseHeight + camera.offsetY;

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

        float rangeX = right - left;
        float stepX = powf(10.0f, floorf(log10f(rangeX) - 0.5f));
        if (stepX < 0.1f) stepX = 0.1f;

        float startX = ceilf(left / stepX) * stepX;
        if (fabs(startX) < 0.001f) startX = stepX;

        for (float x = startX; x <= right; x += stepX) {
            if (fabs(x) < 0.001f) continue;

            ImVec2 pos = worldToScreen(x, 0.0f);

            if (pos.x >= winPos.x && pos.x <= winPos.x + winSize.x) {
                char label[32];
                sprintf_s(label, "%.1f", x);
                draw->AddText(ImVec2(pos.x - 12, pos.y + 10), IM_COL32(50, 50, 50, 255), label);
            }
        }

        float rangeY = top - bottom;
        float stepY = powf(10.0f, floorf(log10f(rangeY) - 0.5f));
        if (stepY < 0.1f) stepY = 0.1f;

        float startY = ceilf(bottom / stepY) * stepY;
        if (fabs(startY) < 0.001f) startY = stepY;

        for (float y = startY; y <= top; y += stepY) {
            if (fabs(y) < 0.001f) continue;

            ImVec2 pos = worldToScreen(0.0f, y);

            if (pos.y >= winPos.y && pos.y <= winPos.y + winSize.y) {
                char label[32];
                sprintf_s(label, "%.1f", y);
                draw->AddText(ImVec2(pos.x - 28, pos.y - 8), IM_COL32(50, 50, 50, 255), label);
            }
        }

        for (float x = startX; x <= right; x += stepX) {
            if (fabs(x) < 0.001f) continue;
            ImVec2 pos = worldToScreen(x, 0.0f);
            draw->AddLine(ImVec2(pos.x, pos.y - 5), ImVec2(pos.x, pos.y + 5), IM_COL32(100, 100, 100, 255), 1.0f);
        }

        for (float y = startY; y <= top; y += stepY) {
            if (fabs(y) < 0.001f) continue;
            ImVec2 pos = worldToScreen(0.0f, y);
            draw->AddLine(ImVec2(pos.x - 5, pos.y), ImVec2(pos.x + 5, pos.y), IM_COL32(100, 100, 100, 255), 1.0f);
        }
        

        ImVec2 xPos = worldToScreen(right * 0.92f, bottom * 0.1f);
        draw->AddText(ImVec2(xPos.x + 15, xPos.y + 15), IM_COL32(50, 50, 50, 255), "X");

        ImVec2 yPos = worldToScreen(left * 0.05f, top * 0.85f);
        draw->AddText(ImVec2(yPos.x - 25, yPos.y - 25), IM_COL32(50, 50, 50, 255), "Y");

        for (const auto& line : inequalityRenderer.getGUILines()) {
            if (line.isX) {
                draw->AddLine(
                    ImVec2(line.screenPos, winPos.y),
                    ImVec2(line.screenPos, winPos.y + winSize.y),
                    IM_COL32(line.color.r * 255, line.color.g * 255, line.color.b * 255, (int)(line.alpha * 255)),
                    2.0f
                );
            }
            else {
                draw->AddLine(
                    ImVec2(winPos.x, line.screenPos),
                    ImVec2(winPos.x + winSize.x, line.screenPos),
                    IM_COL32(line.color.r * 255, line.color.g * 255, line.color.b * 255, (int)(line.alpha * 255)),
                    2.0f
                );
            }
        }
        inequalityRenderer.clearGUI();
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