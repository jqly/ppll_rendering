#ifndef XY_SHADER
#define XY_SHADER


#include <string>
#include "glad/glad.h"
#include "xy_calc.h"



class Shader {
public:
	Shader();
	void Init(std::string comp_shader);
	void Init(std::string vert_shader, std::string frag_shader);
	void Init(std::string vert_shader, std::string geom_shader, std::string frag_shader);
	~Shader();
	GLuint Get();

	void Assign(const char *name, const xy::mat4 &value)
	{
		glUniformMatrix4fv(glGetUniformLocation(handle_, name), 1, GL_FALSE, value.data);
	}

	void Assign(const char *name, const xy::vec3 &value)
	{
		glUniform3f(glGetUniformLocation(handle_, name), value.r, value.g, value.b);
	}

	void Assign(const char *name, const xy::vec2 &value) {
		glUniform2f(glGetUniformLocation(handle_, name), value.x, value.y);
	}

	void Assign(const char *name, const xy::vec4 &value) {
		glUniform4f(glGetUniformLocation(handle_, name), value.x, value.y, value.z, value.w);
	}

	void Assign(const char *name, const GLuint value) {
		glUniform1ui(glGetUniformLocation(handle_, name), value);
	}

	void Assign(const char *name, const GLint value) {
		glUniform1i(glGetUniformLocation(handle_, name), value);
	}

	void Assign(const char *name, const GLfloat value) {
		glUniform1f(glGetUniformLocation(handle_, name), value);
	}

private:
	GLuint handle_;
};



#endif // !XY_SHADER
