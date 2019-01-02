#include <string>

#include "xy/shader.h"
#include "xy/asset.h"
#include "xy/render_layer.h"
#include "xy/xy_ext.h"
#include "xy_config.h"
#include "xy/camera.h"


struct PPLLShaderParams {
	xy::mat4 g_ViewProj;
	xy::mat4 g_Model;
	xy::vec3 g_Eye;
	xy::vec2 g_WinSize;
	float g_HairRadius;
};

class PPLLForHair {
public:

	PPLLForHair()
		:
		screen_width_{ 0 },
		screen_height_{ 0 },
		num_link_list_nodes_{ 0 },
		counter_buf_{ 0 },
		ll_head_tex_{ 0 },
		ll_head_clear_buf_{ 0 },
		ll_buf_{ 0 }
	{}

	void Init(int screen_width, int screen_height, int num_link_list_nodes)
	{
		//////
		//// Delete previously allocated buffers.
		//////

		//glDeleteBuffers(1, &counter_buf_);
		//glDeleteBuffers(1, &link_list_head_buf_);
		//glDeleteBuffers(1, &ll_buf_);

		////
		// Init variables.
		////

		screen_width_ = screen_width;
		screen_height_ = screen_height;
		num_link_list_nodes_ = num_link_list_nodes;

		////
		// Init buffers.
		////

		// Atomic counter buffer.
		glGenBuffers(1, &counter_buf_);
		glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, counter_buf_);
		GLuint counter_zero_state = 0;
		glBufferStorage(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &counter_zero_state, GL_DYNAMIC_STORAGE_BIT);

		// Link-list head buffer is an image.
		glGenTextures(1, &ll_head_tex_);
		glBindTexture(GL_TEXTURE_2D, ll_head_tex_);
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, screen_width_, screen_height_);

		// We need another texture to clear the head buffer.
		std::vector<GLuint> clear_(screen_width_*screen_height_, 0x3fffffff);
		glGenBuffers(1, &ll_head_clear_buf_);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, ll_head_clear_buf_);
		glBufferData(GL_PIXEL_UNPACK_BUFFER, clear_.size() * sizeof(GLuint), clear_.data(), GL_STATIC_COPY);
		// ImGUI will be affected had we not set the following.
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		// Link-list arena buffer.
		glGenBuffers(1, &ll_buf_);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ll_buf_);
		glBufferStorage(GL_SHADER_STORAGE_BUFFER, num_link_list_nodes_ * sizeof(PPLLNode), nullptr, GL_NONE);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		store_pass_.Init(
			xy::ReadFile(xy_config::GetShaderPath("ppll_store.vert")),
			xy::ReadFile(xy_config::GetShaderPath("ppll_store.geom")),
			xy::ReadFile(xy_config::GetShaderPath("ppll_store.frag"))
		);

		blend_pass_.Init(
			xy::ReadFile(xy_config::GetShaderPath("ppll_blend.vert")),
			xy::ReadFile(xy_config::GetShaderPath("ppll_blend.frag"))
		);

		glUseProgram(store_pass_.Get());
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buf_);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ll_buf_);
		glBindImageTexture(0, ll_head_tex_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glUseProgram(0);

		glUseProgram(blend_pass_.Get());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ll_buf_);
		glBindImageTexture(0, ll_head_tex_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
		glUseProgram(0);
	}

	void BindStorePass(PPLLShaderParams &g)
	{
		// Clear linked list heads.
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, ll_head_clear_buf_);
		glBindTexture(GL_TEXTURE_2D, ll_head_tex_);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screen_width_, screen_height_,
			GL_RED_INTEGER, GL_UNSIGNED_INT, NULL);
		glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

		// Counter reset to 0.
		GLuint tmp = 0;
		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buf_);
		glBufferSubData(GL_ATOMIC_COUNTER_BUFFER, 0, sizeof(GLuint), &tmp);

		// Assign uniforms.
		glUseProgram(store_pass_.Get());

		store_pass_.Assign("g_Model", g.g_Model);
		store_pass_.Assign("g_HairRadius", g.g_HairRadius);
		store_pass_.Assign("g_Eye", g.g_Eye);
		store_pass_.Assign("g_ViewProj", g.g_ViewProj);
		store_pass_.Assign("g_WinSize", g.g_WinSize);
		store_pass_.Assign("g_NumNodes", static_cast<GLuint>(num_link_list_nodes_));
	}

	void BindBlendPass(PPLLShaderParams &g)
	{
		glUseProgram(blend_pass_.Get());

		blend_pass_.Assign("g_WinSize", g.g_WinSize);
		blend_pass_.Assign("g_NumNodes", static_cast<GLuint>(num_link_list_nodes_));
	}

private:
	int screen_width_, screen_height_;
	int num_link_list_nodes_;
	GLuint counter_buf_, ll_head_tex_, ll_head_clear_buf_, ll_buf_;

	struct PPLLNode {
		GLuint depth;
		GLuint data;
		GLuint color;
		GLuint next;
	};

	Shader store_pass_, blend_pass_;
};

struct DrawShaderParams {
	float g_HairRadius;
};

class Draw {
public:

	Draw()
	{}

	void Init(int screen_width, int screen_height, int msaa_level)
	{

		screen_width_ = screen_width;
		screen_height_ = screen_height;

		////
		// Main frame settings.
		////

		composite_layer_.Init(GL_RGBA8, GL_DEPTH_COMPONENT24, msaa_level, screen_width_, screen_height_);

		////
		// PPLLForHair settings.
		////

		int num_link_list_nodes = screen_width_ * screen_height_ * 500;
		ppll_.Init(screen_width_, screen_height_, num_link_list_nodes);

		// Screen quad for PPLLForHair second pass.
		std::vector<xy::vec3> quad{ {-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0} };
		screen_quad_vao_.SubmitBuf(quad, { 3 });

	}

	void Render(
		FiberAsset &fiber_asset,
		Camera &camera,
		xy::vec4 bg,
		DrawShaderParams &shader_params
	)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, composite_layer_.Get());
		glClearColor(bg.r, bg.g, bg.b, bg.a);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		////
		// PPLL passes.
		////

		// Gather all PPLL-store-pass uniforms.
		PPLLShaderParams desc;
		desc.g_Eye = camera.Pos();
		desc.g_HairRadius = shader_params.g_HairRadius;
		desc.g_Model = fiber_asset.model_matrix;
		desc.g_ViewProj = camera.Proj()*camera.View();
		desc.g_WinSize = xy::vec2(screen_width_, screen_height_);

		// Store pass.
		ppll_.BindStorePass(desc);
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		//glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		fiber_asset.vao.DrawLineStrips({ 0,1,2 });
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		//glClearColor(bg.r, bg.g, bg.b, bg.a);
		//glClear(GL_COLOR_BUFFER_BIT);

		// Two-pass sync.
		glFinish();

		glDisable(GL_DEPTH_TEST);
		ppll_.BindBlendPass(desc);
		//screen_quad_vao_.Draw(GL_TRIANGLE_FAN, { 0 });
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}

	void OutputFrame()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1,1,1,1);
		glClear(GL_COLOR_BUFFER_BIT);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, composite_layer_.Get());
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(
			0, 0,
			screen_width_, screen_height_,
			0, 0,
			screen_width_, screen_height_,
			GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

private:

	int screen_width_, screen_height_;

	PPLLForHair ppll_;
	RenderLayer composite_layer_;
	Shader ppll_store_, ppll_blend_;
	GpuArray screen_quad_vao_;
};
