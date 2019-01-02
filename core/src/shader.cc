#include "shader.h"

#include <string>
#include "xy_ext.h"


GLuint CreateShader(GLenum type, std::string code)
{
	GLuint shader_handle = glCreateShader(type);
	auto code_ = code.c_str();
	glShaderSource(shader_handle, 1, &code_, nullptr);
	glCompileShader(shader_handle);

	int success = 0, log_len = 0;
	glGetShaderiv(shader_handle, GL_COMPILE_STATUS, &success);

	if (!success) {
		std::string shader_type_name{ "Unknown shader" };
		switch (type) {
		case GL_VERTEX_SHADER:
			shader_type_name = "Vertex shader";
			break;
		case GL_FRAGMENT_SHADER:
			shader_type_name = "Fragment shader";
			break;
		case GL_GEOMETRY_SHADER:
			shader_type_name = "Geometry shader";
			break;
		case GL_COMPUTE_SHADER:
			shader_type_name = "Compute shader";
			break;
		default:
			XY_Die("Unknown shader type");
		}

		glGetShaderiv(shader_handle, GL_INFO_LOG_LENGTH, &log_len);
		if (log_len <= 0)
			xy::Print("{}: no log found", shader_type_name);
		else {
			auto log = std::unique_ptr<char>(new char[log_len]);
			glGetShaderInfoLog(shader_handle, log_len, &log_len, log.get());
			xy::Print("{} compile log: {}", shader_type_name, log.get());
		}
		XY_Die(shader_type_name + " compile error");
	}
	return shader_handle;
}

void CheckShader(GLuint shader)
{
	int success = 0, log_len = 0;
	glGetProgramiv(shader, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		if (log_len <= 0)
			xy::Print("No log found");
		else {
			auto log = std::unique_ptr<char>(new char[log_len]);
			glGetProgramInfoLog(shader, log_len, nullptr, log.get());
			xy::Print("Program compile log: {}", log.get());
		}
		xy::Print("Program compile error");
	}
}

Shader::Shader()
	:handle_{ 0 } {}

void Shader::Init(std::string comp_shader)
{
	GLuint compobj = CreateShader(GL_COMPUTE_SHADER, comp_shader);

	handle_ = glCreateProgram();
	glAttachShader(handle_, compobj);
	glLinkProgram(handle_);

	CheckShader(handle_);

	glDeleteShader(compobj);
}

void Shader::Init(std::string vert_shader, std::string frag_shader)
{
	auto vertobj = CreateShader(GL_VERTEX_SHADER, vert_shader);
	auto fragobj = CreateShader(GL_FRAGMENT_SHADER, frag_shader);

	handle_ = glCreateProgram();
	glAttachShader(handle_, vertobj);
	glAttachShader(handle_, fragobj);
	glLinkProgram(handle_);

	CheckShader(handle_);

	glDeleteShader(vertobj);
	glDeleteShader(fragobj);
}

void Shader::Init(std::string vert_shader, std::string geom_shader, std::string frag_shader)
{
	auto vertobj = CreateShader(GL_VERTEX_SHADER, vert_shader);
	auto geomobj = CreateShader(GL_GEOMETRY_SHADER, geom_shader);
	auto fragobj = CreateShader(GL_FRAGMENT_SHADER, frag_shader);

	handle_ = glCreateProgram();
	glAttachShader(handle_, vertobj);
	glAttachShader(handle_, geomobj);
	glAttachShader(handle_, fragobj);
	glLinkProgram(handle_);

	CheckShader(handle_);

	glDeleteShader(vertobj);
	glDeleteShader(geomobj);
	glDeleteShader(fragobj);
}

Shader::~Shader()
{
	glDeleteProgram(handle_);
}

GLuint Shader::Get() { return handle_; }