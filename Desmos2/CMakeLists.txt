cmake_minimum_required(VERSION 3.20)
project(Desmos2)

add_compile_options(/bigobj)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    exprtk
    GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
    GIT_TAG master
)
FetchContent_MakeAvailable(exprtk)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.8
)
FetchContent_MakeAvailable(imgui)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${glm_SOURCE_DIR})
include_directories(${exprtk_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR})
include_directories(${imgui_SOURCE_DIR}/backends)

set(SOURCES
    Desmos2.cpp
    Graph2D.cpp
    Shader.cpp
    Inequality.cpp
    glad.c
)

set(IMGUI_SOURCES
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)


add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/shaders
    $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders
)
target_link_libraries(${PROJECT_NAME}
    glfw
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} opengl32)
endif()

message(STATUS "Project ready - libraries will be downloaded automatically!")