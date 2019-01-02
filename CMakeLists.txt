cmake_minimum_required(VERSION 3.9)

project(xyapp)


# GLFW target.
add_subdirectory("${CMAKE_SOURCE_DIR}/glfw-3.2.1-modified")

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${CMAKE_SOURCE_DIR}/stb)

add_library(tinyobjloader INTERFACE)
target_include_directories(tinyobjloader INTERFACE ${CMAKE_SOURCE_DIR}/tinyobjloader)

# GLAD target.
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/glad/src/glad.c
    ${CMAKE_SOURCE_DIR}/glad/include/glad/glad.h)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/glad/include/)

add_library(game_infra STATIC
    ${CMAKE_SOURCE_DIR}/core/include/xy/aabb.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/asset.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/camera.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/window.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/gpu_array.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/render_layer.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/shader.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/xy_calc.h
    ${CMAKE_SOURCE_DIR}/core/include/xy/xy_ext.h
    ${CMAKE_SOURCE_DIR}/core/src/aabb.cc
    ${CMAKE_SOURCE_DIR}/core/src/asset.cc
    ${CMAKE_SOURCE_DIR}/core/src/gpu_array.cc
    ${CMAKE_SOURCE_DIR}/core/src/render_layer.cc
    ${CMAKE_SOURCE_DIR}/core/src/shader.cc
    ${CMAKE_SOURCE_DIR}/core/src/camera.cc
    ${CMAKE_SOURCE_DIR}/core/src/window.cc
	)
target_include_directories(game_infra PUBLIC ${CMAKE_SOURCE_DIR}/core/include)
target_include_directories(game_infra PRIVATE ${CMAKE_SOURCE_DIR}/core/include/xy)
target_link_libraries(game_infra PRIVATE stb)
target_link_libraries(game_infra PRIVATE tinyobjloader)
target_link_libraries(game_infra PRIVATE glad)
target_link_libraries(game_infra PRIVATE glfw)

# IMGUI target.
add_library(imgui STATIC 
    ${CMAKE_SOURCE_DIR}/imgui/imgui.cpp
    ${CMAKE_SOURCE_DIR}/imgui/imgui_demo.cpp
    ${CMAKE_SOURCE_DIR}/imgui/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/imgui/imgui_impl_opengl3.cpp
    ${CMAKE_SOURCE_DIR}/imgui/imgui_draw.cpp
    ${CMAKE_SOURCE_DIR}/imgui/include/imgui/imgui.h
    ${CMAKE_SOURCE_DIR}/imgui/include/imgui/imconfig.h
    ${CMAKE_SOURCE_DIR}/imgui/include/imgui/imgui_internal.h
    ${CMAKE_SOURCE_DIR}/imgui/include/imgui/imgui_impl_glfw.h
    ${CMAKE_SOURCE_DIR}/imgui/include/imgui/imgui_impl_opengl3.h)
target_include_directories(imgui PRIVATE ${CMAKE_SOURCE_DIR}/imgui/include/imgui)
target_include_directories(imgui INTERFACE ${CMAKE_SOURCE_DIR}/imgui/include)

target_link_libraries(imgui PRIVATE stb)
target_link_libraries(imgui PRIVATE glad)
target_link_libraries(imgui PRIVATE glfw)

add_executable(xyapp 
    src/main.cc 
    src/shader.h 
    include/xy_config.h)

target_include_directories(xyapp
PRIVATE 
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(xyapp glad)
target_link_libraries(xyapp glfw)
target_link_libraries(xyapp stb)
target_link_libraries(xyapp imgui)
target_link_libraries(xyapp tinyobjloader)
target_link_libraries(xyapp game_infra)

install(TARGETS xyapp
    DESTINATION bin)
