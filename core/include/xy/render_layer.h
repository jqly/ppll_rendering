#ifndef XY_RENDER_LAYER
#define XY_RENDER_LAYER


#include "glad/glad.h"


class FrameLayer {

public:
	FrameLayer() = default;
	FrameLayer(FrameLayer const&) = delete;
	FrameLayer& operator=(FrameLayer const&) = delete;

	void Init(GLenum color_format, GLenum depth_format, GLsizei msaa_level, int width, int height);
	void Init(GLenum color_format, int width, int height);

	void BlitColor(const FrameLayer &dest) const;
	GLuint GetColor() const;
	void BlitDepth(const FrameLayer &dest) const;

	int Width() const { return width_; }
	int Height() const { return height_; }
	GLuint Get() const { return handle_; }

	~FrameLayer();

private:
	int width_, height_, msaa_level_;
	GLuint handle_, color_buf_, depth_buf_;

};

class TextureLayer {

public:
	TextureLayer() = default;
	TextureLayer(TextureLayer const&) = delete;
	TextureLayer& operator=(TextureLayer const&) = delete;

	void Init(GLenum format, int width, int height, int levels = 1, GLint min_filter_param = GL_LINEAR_MIPMAP_LINEAR, GLint mag_filter_param = GL_LINEAR)
	{
		width_ = width;
		height_ = height;

		glGenTextures(1, &handle_);
		glBindTexture(GL_TEXTURE_2D, handle_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter_param);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter_param);
		glTexStorage2D(GL_TEXTURE_2D, levels, format, width_, height_);
	}

	GLuint Get() { return handle_; }

	~TextureLayer()
	{
		glDeleteTextures(1, &handle_);
	}

private:
	int width_, height_;
	GLuint handle_;

};


#endif // !XY_RENDER_LAYER