#pragma once

struct Camera {
    float offsetX;   // Смещение по оси X 
    float offsetY;   // Смещение по оси Y 
    float scale;     
    bool dragging;   // флаг зажата ли левая кнопка мыши
    double lastX;    // Последняя X-коор мыши
    double lastY;    // Последняя Y-коор

    Camera() : offsetX(0.0f), offsetY(0.0f), scale(1.0f), dragging(false), lastX(0), lastY(0) {}
    void reset() {
        offsetX = 0.0f;
        offsetY = 0.0f;
        scale = 1.0f;
    }
};

