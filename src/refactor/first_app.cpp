//
// Created by Vlad Dancea on 28.03.24.
//

#include "first_app.h"

namespace vk {
    void FirstApp::run() {
        while (!window.shouldClose()) {
            glfwPollEvents();
        }
    }
}