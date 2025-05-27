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

    void InputManager::registerKeyCallback(int c, KeyCallback cb, void* o, int ctx) {
        keyBindings[ctx][c].push_back({std::move(cb), o});
    }
    void InputManager::registerMouseButtonCallback(int c, KeyCallback cb, void* o, int ctx) {
        mouseBindings[ctx][c].push_back({std::move(cb), o});
    }
    void InputManager::registerCharCallback(CharCallback cb, void* o, int ctx) {
        charBindings[ctx].push_back({std::move(cb), o});
    }
    void InputManager::registerCursorPosCallback(CursorPosCallback cb, void* o, int ctx) {
        cursorBindings[ctx].push_back({std::move(cb), o});
    }
    void InputManager::registerScrollCallback(ScrollCallback cb, void* o, int ctx) {
        scrollBindings[ctx].push_back({std::move(cb), o});
    }
    void InputManager::registerPollingAction(PollingFunc pf, void* o, int ctx) {
        pollers[ctx].push_back({std::move(pf), o});
    }

    void InputManager::deregisterOwner(void* o) {
        for (auto& [ctx, map] : keyBindings)
            for (auto& [k, vec] : map)
                vec.erase(std::remove_if(vec.begin(), vec.end(), [&](auto& c) { return c.owner == o; }), vec.end());

        for (auto& [ctx, map] : mouseBindings)
            for (auto& [b, vec] : map)
                vec.erase(std::remove_if(vec.begin(), vec.end(), [&](auto& c) { return c.owner == o; }), vec.end());

        for (auto& [ctx, v] : charBindings)
            v.erase(std::remove_if(v.begin(), v.end(), [&](auto& c) { return c.owner == o; }), v.end());

        for (auto& [ctx, v] : cursorBindings)
            v.erase(std::remove_if(v.begin(), v.end(), [&](auto& c) { return c.owner == o; }), v.end());

        for (auto& [ctx, v] : scrollBindings)
            v.erase(std::remove_if(v.begin(), v.end(), [&](auto& c) { return c.owner == o; }), v.end());

        for (auto& [ctx, v] : pollers)
            v.erase(std::remove_if(v.begin(), v.end(), [&](auto& p) { return p.owner == o; }), v.end());
    }

    void InputManager::deregisterKey(int code, void* o, int ctx) {
        if (auto mit = keyBindings.find(ctx); mit != keyBindings.end()) {
            if (auto fit = mit->second.find(code); fit != mit->second.end()) {
                auto& v = fit->second;
                v.erase(std::remove_if(v.begin(), v.end(),
                    [&](auto& c) { return c.owner == o; }),
                    v.end());
            }
        }
        if (auto mit = mouseBindings.find(ctx); mit != mouseBindings.end()) {
            if (auto fit = mit->second.find(code); fit != mit->second.end()) {
                auto& v = fit->second;
                v.erase(std::remove_if(v.begin(), v.end(),
                    [&](auto& c) { return c.owner == o; }),
                    v.end());
            }
        }
    }

    void InputManager::onKey(int code, int, int action, int) {
        if (action == GLFW_PRESS)   pressedKeys.insert(code);
        else if (action == GLFW_RELEASE) pressedKeys.erase(code);
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

        auto& ctxMap = keyBindings[activeContext];
        if (auto it = ctxMap.find(code); it != ctxMap.end())
            for (auto& c : it->second) c.cb();

        // additionally trigger global context
        if (activeContext != 0) {
            auto& globalCtxMap = keyBindings[0];
            if (auto it = globalCtxMap.find(code); it != globalCtxMap.end())
                for (auto& c : it->second) c.cb();
        }
    }

    void InputManager::onMouseButton(int b, int action, int) {
        if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
        auto& ctxMap = mouseBindings[activeContext];
        if (auto it = ctxMap.find(b); it != ctxMap.end())
            for (auto& c : it->second) c.cb();

        // additionally trigger global context
        if (activeContext != 0) {
            auto& globalCtxMap = mouseBindings[0];
            if (auto it = globalCtxMap.find(b); it != globalCtxMap.end())
                for (auto& c : it->second) c.cb();
        }
    }

    void InputManager::onChar(unsigned int cp) {
        for (auto& c : charBindings[activeContext]) c.cb(cp);

        // additionally trigger global context
        if (activeContext != 0) {
            for (auto& c : charBindings[0]) c.cb(cp);
        }
    }

    void InputManager::onCursorPos(double x, double y) {
        cursorX = x; cursorY = y;
        for (auto& c : cursorBindings[activeContext]) c.cb(x, y);

        // additionally trigger global context
        if (activeContext != 0) {
            for (auto& c : cursorBindings[0]) c.cb(x, y);
        }
    }

    void InputManager::onScroll(double xoffset, double yoffset) {
        scrollX += xoffset; scrollY += yoffset;
        for (auto& c : scrollBindings[activeContext]) c.cb(xoffset, yoffset);

        // additionally trigger global context
        if (activeContext != 0) {
            for (auto& c : scrollBindings[0]) c.cb(xoffset, yoffset);
        }
    }

    void InputManager::processPolling(float deltaTime) {
        for (auto& p : pollers[activeContext]) p.pf(deltaTime);

        // additionally trigger global context
        if (activeContext != 0) {
            for (auto& p : pollers[0]) p.pf(deltaTime);
        }
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