#pragma once

#include <GLFW/glfw3.h>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace input {
    class InputManager {
    public:
        explicit InputManager(GLFWwindow* window);

        using KeyCallback = std::function<void()>;
        using CharCallback = std::function<void(unsigned int)>;
        using CursorPosCallback = std::function<void(double, double)>;
        using ScrollCallback = std::function<void(double, double)>;
        using PollingFunc = std::function<void(float)>;

        void setActiveContext(const int context) { activeContext = context; };
        int  getActiveContext() const { return activeContext; };

        void registerKeyCallback(int code, KeyCallback cb, void* owner, int context);
        void registerMouseButtonCallback(int code, KeyCallback cb, void* owner, int context);
        void registerCharCallback(CharCallback cb, void* owner, int context);
        void registerCursorPosCallback(CursorPosCallback cb, void* owner, int context);
        void registerScrollCallback(ScrollCallback cb, void* owner, int context);
        void registerPollingAction(PollingFunc pf, void* owner, int context);

        void deregisterOwner(void* owner);
        void deregisterKey(int code, void* owner, int context);

        // called by GLFW callbacks
        void onKey(int code, int scancode, int action, int mods);
        void onMouseButton(int button, int action, int mods);
        void onChar(unsigned int codepoint);
        void onCursorPos(double x, double y);
        void onScroll(double xoffset, double yoffset);

        void processPolling(float dt);

        bool isKeyPressed(int code) const;
        std::pair<double, double> getCursorPos() const;
        std::pair<double, double> getScrollOffset();

    private:
        void installGlfwCallbacks(GLFWwindow* window);

        struct CB { KeyCallback cb; void* owner; };
        struct CMB { KeyCallback cb; void* owner; };
        struct CChar { CharCallback cb; void* owner; };
        struct CPos { CursorPosCallback cb; void* owner; };
        struct CSc { ScrollCallback cb; void* owner; };
        struct PF { PollingFunc pf; void* owner; };

        int activeContext = 0;

        std::unordered_map<int, std::unordered_map<int, std::vector<CB>>>     keyBindings;
        std::unordered_map<int, std::unordered_map<int, std::vector<CMB>>>    mouseBindings;
        std::unordered_map<int, std::vector<CChar>>                           charBindings;
        std::unordered_map<int, std::vector<CPos>>                            cursorBindings;
        std::unordered_map<int, std::vector<CSc>>                             scrollBindings;
        std::unordered_map<int, std::vector<PF>>                              pollers;

        std::unordered_set<int>                                               pressedKeys;

        double cursorX = 0, cursorY = 0;
        double scrollX = 0, scrollY = 0;
    };
}