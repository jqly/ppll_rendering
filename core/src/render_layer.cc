#include "render_layer.h"

#include "glad/glad.h"
#include "xy_ext.h"


void FrameLayer::Init(GLenum color_format, GLenum depth_format, GLsizei msaa_level, int width, int height)
{
	if (msaa_level <= 1) msaa_level = 1;

	width_ = width;
	height_ = height;
	msaa_level_ = msaa_level > 1 ? msaa_level : 1;

	// When no msaa, use plain old color texture, renderbuffer otherwise.
	if (msaa_level_ > 1) {
		glGenRenderbuffers(1, &color_buf_);
		glBindRenderbuffer(GL_RENDERBUFFER, color_buf_);
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level_, color_format, width_, height_);
	}
	else {
		glGenTextures(1, &color_buf_);
		glBindTexture(GL_TEXTURE_2D, color_buf_);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexStorage2D(GL_TEXTURE_2D, 1, color_format, width, height);
	}

	glGenRenderbuffers(1, &depth_buf_);
	glBindRenderbuffer(GL_RENDERBUFFER, depth_buf_);
	if (msaa_level_ > 1)
		glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level_, depth_format, width_, height_);
	else
		glRenderbufferStorage(GL_RENDERBUFFER, depth_format, width_, height_);

	glGenFramebuffers(1, &handle_);
	glBindFramebuffer(GL_FRAMEBUFFER, handle_);

	if (msaa_level_ > 1)
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buf_);
	else
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buf_, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buf_);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		XY_Die("Framebuffer not complete");
}

void FrameLayer::Init(GLenum color_format, int width, int height)
{
	width_ = width;
	height_ = height;
	msaa_level_ = 1;
	depth_buf_ = 0;

	glGenTextures(1, &color_buf_);
	glBindTexture(GL_TEXTURE_2D, color_buf_);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexStorage2D(GL_TEXTURE_2D, 1, color_format, width_, height_);

	glGenFramebuffers(1, &handle_);
	glBindFramebuffer(GL_FRAMEBUFFER, handle_);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color_buf_, 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		XY_Die("Framebuffer not complete");
}

void FrameLayer::BlitColor(const FrameLayer & dest) const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, handle_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest.handle_);
	glBlitFramebuffer(0, 0, width_, height_, 0, 0, dest.Width(), dest.Height(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

GLuint FrameLayer::GetColor() const
{
	return color_buf_;
}

void FrameLayer::BlitDepth(const FrameLayer & dest) const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, handle_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest.handle_);
	glBlitFramebuffer(0, 0, width_, height_, 0, 0, dest.Width(), dest.Height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

FrameLayer::~FrameLayer()
{
	if (msaa_level_ == 1)
		glDeleteTextures(1, &color_buf_);
	else
		glDeleteRenderbuffers(1, &color_buf_);

	glDeleteRenderbuffers(1, &depth_buf_);

	glDeleteFramebuffers(1, &handle_);
}
