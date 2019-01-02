#ifndef XY_RENDER_LAYER
#define XY_RENDER_LAYER


#include "glad/glad.h"


class RenderLayer {

public:
	RenderLayer() = default;
	RenderLayer(RenderLayer const&) = delete;
	RenderLayer& operator=(RenderLayer const&) = delete;

	void Init(GLenum color_format, GLenum depth_format, GLsizei msaa_level, int width, int height);
	void Init(GLenum color_format, GLenum depth_format, int width, int height);
	void InitForColorBlit(GLenum color_format, int width, int height);

	void BlitColor(const RenderLayer &dest) const;
	void BlitDepth(const RenderLayer &dest) const;

	int Width() const {return width_;}
	int Height() const {return height_;}
	GLuint Get() const {return handle_;}

	~RenderLayer();

private:
	int width_, height_, msaa_level_;
	GLuint handle_, color_buf_, depth_buf_;
	bool is_for_color_blit_;

};



#endif // !XY_RENDER_LAYER