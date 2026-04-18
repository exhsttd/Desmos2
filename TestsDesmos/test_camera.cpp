#include <gtest/gtest.h>
#include "../Desmos2/Camera.h"

TEST(CameraTest, Initialization) {
    Camera camera;

    EXPECT_FLOAT_EQ(camera.offsetX, 0.0f);
    EXPECT_FLOAT_EQ(camera.offsetY, 0.0f);
    EXPECT_FLOAT_EQ(camera.scale, 1.0f);
    EXPECT_FALSE(camera.dragging);
}

TEST(CameraTest, Reset) {
    Camera camera;

    camera.offsetX = 5.0f;
    camera.offsetY = -3.0f;
    camera.scale = 2.5f;

    camera.reset();

    EXPECT_FLOAT_EQ(camera.offsetX, 0.0f);
    EXPECT_FLOAT_EQ(camera.offsetY, 0.0f);
    EXPECT_FLOAT_EQ(camera.scale, 1.0f);
}

TEST(CameraTest, ScaleBounds) {
    Camera camera;

    camera.scale = 0.05f;
    EXPECT_FLOAT_EQ(camera.scale, 0.05f);

    camera.scale = 15.0f;
    EXPECT_FLOAT_EQ(camera.scale, 15.0f);
}