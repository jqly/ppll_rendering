#include "render_layer.h"

#include "glad/glad.h"
#include "xy_ext.h"


void RenderLayer::Init(GLenum color_format, GLenum depth_format, GLsizei msaa_level, int width, int height)
{
	width_ = width;
	height_ = height;
	msaa_level_ = msaa_level;
	is_for_color_blit_ = false;

	glGenRenderbuffers(1, &color_buf_);
	glGenRenderbuffers(1, &depth_buf_);

	glBindRenderbuffer(GL_RENDERBUFFER, color_buf_);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level_, color_format, width_, height_);

	glBindRenderbuffer(GL_RENDERBUFFER, depth_buf_);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa_level_, depth_format, width_, height_);

	glGenFramebuffers(1, &handle_);
	glBindFramebuffer(GL_FRAMEBUFFER, handle_);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buf_);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buf_);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		XY_Die("Framebuffer not complete");
}

void RenderLayer::Init(GLenum color_format, GLenum depth_format, int width, int height)
{
	this->width_ = width;
	this->height_ = height_;
	this->msaa_level_ = 1;
	this->is_for_color_blit_ = false;

	glGenRenderbuffers(1, &color_buf_);
	glGenRenderbuffers(1, &depth_buf_);

	glBindRenderbuffer(GL_RENDERBUFFER, color_buf_);
	glRenderbufferStorage(GL_RENDERBUFFER, color_format, width_, height_);

	glBindRenderbuffer(GL_RENDERBUFFER, depth_buf_);
	glRenderbufferStorage(GL_RENDERBUFFER, depth_format, width_, height_);

	glGenFramebuffers(1, &handle_);
	glBindFramebuffer(GL_FRAMEBUFFER, handle_);

	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color_buf_);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth_buf_);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
		XY_Die("Framebuffer not complete");
}

void RenderLayer::InitForColorBlit(GLenum color_format, int width, int height)
{
	width_ = width;
	height_ = height;
	msaa_level_ = 1;
	depth_buf_ = 0;
	is_for_color_blit_ = true;

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

void RenderLayer::BlitColor(const RenderLayer & dest) const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, handle_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest.handle_);
	glBlitFramebuffer(0, 0, width_, height_, 0, 0, dest.Width(), dest.Height(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
}

void RenderLayer::BlitDepth(const RenderLayer & dest) const
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, handle_);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dest.handle_);
	glBlitFramebuffer(0, 0, width_, height_, 0, 0, dest.Width(), dest.Height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

RenderLayer::~RenderLayer()
{
	if (is_for_color_blit_) {
		glDeleteTextures(1, &color_buf_);
	}
	else {
		glDeleteRenderbuffers(1, &color_buf_);
		glDeleteRenderbuffers(1, &depth_buf_);
	}
	glDeleteFramebuffers(1, &handle_);
}