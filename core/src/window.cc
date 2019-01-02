#include "window.h"


inline void DebugCallback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar * message,
	const void * userParam)
{
	// ignore non-significant error/warning codes
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	xy::Print(
		"---------------\n"
		"Debug message ({}): {}",
		id, message
	);

	switch (source) {
	case GL_DEBUG_SOURCE_API:
		xy::Print("Source: API\n"); break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		xy::Print("Source: Window System\n"); break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		xy::Print("Source: Shader Compiler\n"); break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		xy::Print("Source: Third Party\n"); break;
	case GL_DEBUG_SOURCE_APPLICATION:
		xy::Print("Source: Application\n"); break;
	case GL_DEBUG_SOURCE_OTHER:
		xy::Print("Source: Other\n"); break;
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		xy::Print("Type: Error\n"); break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		xy::Print("Type: Deprecated Behaviour\n"); break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		xy::Print("Type: Undefined Behaviour\n"); break;
	case GL_DEBUG_TYPE_PORTABILITY:
		xy::Print("Type: Portability\n"); break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		xy::Print("Type: Performance\n"); break;
	case GL_DEBUG_TYPE_MARKER:
		xy::Print("Type: Marker\n"); break;
	case GL_DEBUG_TYPE_PUSH_GROUP:
		xy::Print("Type: Push Group\n"); break;
	case GL_DEBUG_TYPE_POP_GROUP:
		xy::Print("Type: Pop Group\n"); break;
	case GL_DEBUG_TYPE_OTHER:
		xy::Print("Type: Other\n"); break;
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH:
		xy::Print("Severity: high\n"); break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		xy::Print("Severity: medium\n"); break;
	case GL_DEBUG_SEVERITY_LOW:
		xy::Print("Severity: low\n"); break;
	case GL_DEBUG_SEVERITY_NOTIFICATION:
		xy::Print("Severity: notification\n"); break;
	}
	return;
}

GLFWWindowDesc::GLFWWindowDesc(int width, int height, std::string title)
	:width{ width }, height{ height }, title{ title }
{
	if (glfwInit() != GLFW_TRUE)
		XY_Die("glfw init failed");

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

	wptr = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (wptr == nullptr) {
		XY_Die("glfw cannot create window");
	}

	glfwMakeContextCurrent(wptr);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glfwSwapInterval(0);
	glfwSetWindowUserPointer(wptr, this);

	GLint flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(DebugCallback, nullptr);
		glDebugMessageControl(GL_DEBUG_SOURCE_API,
			GL_DEBUG_TYPE_ERROR,
			GL_DEBUG_SEVERITY_MEDIUM,
			0, nullptr, GL_TRUE);
	}

	glfwSetKeyCallback(wptr, [](GLFWwindow *wptr, int key, int, int action, int) {
		auto desc = static_cast<GLFWWindowDesc*>(glfwGetWindowUserPointer(wptr));
		if (desc->keyboard_handler)
			desc->keyboard_handler(key, action);
	});

	glfwSetCursorPosCallback(wptr, [](GLFWwindow* wptr, double xpos, double ypos) {
		auto desc = static_cast<GLFWWindowDesc*>(glfwGetWindowUserPointer(wptr));
		if (desc->cursor_handler)
			desc->cursor_handler(static_cast<float>(xpos), static_cast<float>(ypos));
	});

	glfwSetMouseButtonCallback(wptr, [](GLFWwindow *wptr, int button, int action, int mods) {
		auto desc = static_cast<GLFWWindowDesc*>(glfwGetWindowUserPointer(wptr));

		if (desc->mouse_state == MouseState::Release && action == GLFW_PRESS)
			desc->mouse_state = MouseState::Press;
		else if (desc->mouse_state == MouseState::Press && action == GLFW_RELEASE)
			desc->mouse_state = MouseState::Release;

		if (desc->mouse_button_handler)
			desc->mouse_button_handler(button, action, mods);
	});

	glfwSetScrollCallback(wptr, [](GLFWwindow *wptr, double xoffset, double yoffset) {
		auto desc = static_cast<GLFWWindowDesc*>(glfwGetWindowUserPointer(wptr));
		if (desc->scroll_handler)
			desc->scroll_handler(static_cast<float>(xoffset), static_cast<float>(yoffset));
	});

	alive = true;
}

GLFWWindowDesc::~GLFWWindowDesc()
{
	if (wptr)
		glfwDestroyWindow(wptr);
	glfwTerminate();
}
