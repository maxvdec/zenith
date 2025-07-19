/*
* main.cpp
* As part of the Zenith project
* Created by Max Van den Eynde in 2025
* --------------------------------------
* Description: A simple example that renders a triangle with Zenith.
* Copyright (c) 2025 Max Van den Eynde
*/

#include <zenith/zenith.h>
#include <zenith/window.h>
#include <glfw/glfw3.h>

int main() {
    zen::glfw::WindowConfiguration config{};
    config.name = "Zenith Triangle Example";
    config.width = 800;
    config.height = 600;
    config.enableDebugMessenger = true;
    zen::glfw::Window window(config);
    window.init();

    while (!window.shouldClose()) {

    }
    return 0;
}