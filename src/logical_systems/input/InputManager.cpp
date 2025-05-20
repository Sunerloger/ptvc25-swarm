#include "InputManager.h"

#include <GLFW/glfw3.h>
#include <algorithm>

namespace input {
    
    InputManager::InputManager(GLFWwindow* window) {
        glfwSetWindowUserPointer(window, this);
        installGlfwCallbacks(window);
    }

    void InputManager::installGlfwCallbacks(GLFWwindow* w) {
        glfwSetKeyCallback(w, [](GLFWwindow* w, int c, int sc, int a, int m) {
            static_cast<InputManager*>(glfwGetWindowUserPointer(w))
                ->onKey(c, sc, a, m);
            });
        glfwSetMouseButtonCallback(w, [](GLFWwindow* w, int b, int a, int m) {
            static_cast<InputManager*>(glfwGetWindowUserPointer(w))
                ->onMouseButton(b, a, m);
            });
        glfwSetCharCallback(w, [](GLFWwindow* w, unsigned int cp) {
            static_cast<InputManager*>(glfwGetWindowUserPointer(w))
                ->onChar(cp);
            });
        glfwSetCursorPosCallback(w, [](GLFWwindow* w, double x, double y) {
            static_cast<InputManager*>(glfwGetWindowUserPointer(w))
                ->onCursorPos(x, y);
            });
        glfwSetScrollCallback(w, [](GLFWwindow* w, double xoffset, double yoffset) {
            static_cast<InputManager*>(glfwGetWindowUserPointer(w))
                ->onScroll(xoffset, yoffset);
            });
    }

    void InputManager::registerKeyCallback(int c, KeyCallback cb, void* o) {
        keyBindings[c].push_back({ std::move(cb), o });
    }
    void InputManager::registerMouseButtonCallback(int c, KeyCallback cb, void* o) {
        mouseBindings[c].push_back({ std::move(cb), o });
    }
    void InputManager::registerCharCallback(CharCallback cb, void* o) {
        charBindings.push_back({ std::move(cb), o });
    }
    void InputManager::registerCursorPosCallback(CursorPosCallback cb, void* o) {
        cursorBindings.push_back({ std::move(cb), o });
    }
    void InputManager::registerScrollCallback(ScrollCallback cb, void* o) {
        scrollBindings.push_back({ std::move(cb), o });
    }
    void InputManager::registerPollingAction(PollingFunc pf, void* o) {
        pollers.push_back({ std::move(pf), o });
    }

    void InputManager::deregisterOwner(void* o) {
        for (auto& kv : keyBindings)
            kv.second.erase(std::remove_if(
                kv.second.begin(), kv.second.end(),
                [&](auto& c) { return c.owner == o; }),
                kv.second.end());
        for (auto& kv : mouseBindings)
            kv.second.erase(std::remove_if(
                kv.second.begin(), kv.second.end(),
                [&](auto& c) { return c.owner == o; }),
                kv.second.end());
        charBindings.erase(std::remove_if(
            charBindings.begin(), charBindings.end(),
            [&](auto& c) { return c.owner == o; }),
            charBindings.end());
        cursorBindings.erase(std::remove_if(
            cursorBindings.begin(), cursorBindings.end(),
            [&](auto& c) { return c.owner == o; }),
            cursorBindings.end());
        scrollBindings.erase(std::remove_if(
            scrollBindings.begin(), scrollBindings.end(),
            [&](auto& c) { return c.owner == o; }),
            scrollBindings.end());
        pollers.erase(std::remove_if(
            pollers.begin(), pollers.end(),
            [&](auto& p) { return p.owner == o; }),
            pollers.end());
    }

    void InputManager::deregisterKey(int code, void* o) {
        if (auto it = keyBindings.find(code); it != keyBindings.end()) {
            auto& v = it->second;
            v.erase(std::remove_if(v.begin(), v.end(),
                [&](auto& c) { return c.owner == o; }),
                v.end());
        }
        deregisterOwner(o);
    }

    void InputManager::onKey(int code, int, int action, int) {
        if (action == GLFW_PRESS)   pressedKeys.insert(code);
        else if (action == GLFW_RELEASE) pressedKeys.erase(code);
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        if (auto it = keyBindings.find(code); it != keyBindings.end())
            for (auto& c : it->second) c.cb();
    }

    void InputManager::onMouseButton(int b, int action, int) {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        if (auto it = mouseBindings.find(b); it != mouseBindings.end())
            for (auto& c : it->second) c.cb();
    }

    void InputManager::onChar(unsigned int cp) {
        for (auto& c : charBindings) c.cb(cp);
    }

    void InputManager::onCursorPos(double x, double y) {
        cursorX = x; cursorY = y;
        for (auto& c : cursorBindings) c.cb(x, y);
    }

    void InputManager::onScroll(double xoffset, double yoffset) {
        scrollX += xoffset; scrollY += yoffset;
        for (auto& c : scrollBindings) c.cb(xoffset, yoffset);
    }

    void InputManager::processPolling(float deltaTime) {
        for (auto& p : pollers) p.pf(deltaTime);
    }

    bool InputManager::isKeyPressed(int code) const {
        return pressedKeys.count(code) > 0;
    }
    std::pair<double, double> InputManager::getCursorPos() const {
        return { cursorX,cursorY };
    }
    std::pair<double, double> InputManager::getScrollOffset() {
        auto so = std::make_pair(scrollX, scrollY);
        scrollX = scrollY = 0;
        return so;
    }
}