//
// Created by Vlad Dancea on 28.03.24.
//

#include "first_app.h"
#include "cstdlib"
#include "iostream"
#include "stdexcept"

int main() {
    vk::FirstApp app{};

    try {
        app.run();
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}