#ifndef XY_WINDOW
#define XY_WINDOW


#include <functional>
#include <memory>
#include <cassert>

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "xy/xy_ext.h"


struct GLFWWindowDesc {
	GLFWWindowDesc(int width, int height, std::string title);

	int width, height;
	std::string title;
	GLFWwindow *wptr = nullptr;
	bool alive = false;

	enum class MouseState { Press, Release };
	MouseState mouse_state = MouseState::Release;

	// (GLFWwindow *, int key, int, int action, int)
	std::function<void(int, int)> keyboard_handler;
	// (GLFWwindow* wptr, double xpos, double ypos)
	std::function<void(float, float)> cursor_handler;
	// (GLFWwindow *wptr, int button, int action, int mods)
	std::function<void(int, int, int)> mouse_button_handler;
	// (GLFWwindow *wptr, double xoffset, double yoffset)
	std::function<void(float, float)> scroll_handler;

	~GLFWWindowDesc();
};


#endif // !XY_WINDOW
