#include <string>

#include "xy/shader.h"
#include "xy/asset.h"
#include "xy/render_layer.h"
#include "xy/xy_ext.h"
#include "xy_config.h"
#include "xy/camera.h"
#include "xy/aabb.h"


class MSM {

public:

	struct ParamsL {
		xy::mat4 g_LightModelViewProj;
	};

public:

	void Init(int width, int height)
	{
		width_ = width;
		height_ = height;

		rl_.Init(GL_RGBA16, GL_DEPTH_COMPONENT24, 1, width_, height_);
		sm_tmp_.Init(GL_RGBA16, width_, height_);

		render_.Init(
			xy::ReadFile(xy_config::GetShaderPath("msm_store.vert")),
			xy::ReadFile(xy_config::GetShaderPath("msm_store.frag")));

		filter_.Init(xy::ReadFile(xy_config::GetShaderPath("msm_filter.comp")));
	}

	void BindPass()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, rl_.Get());
		glUseProgram(render_.Get());
		glEnable(GL_DEPTH_TEST);

		glClearColor(1.f, 1.f, 1.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glViewport(0, 0, width_, height_);
	}

	void PassParams(ParamsL &params)
	{
		glUseProgram(render_.Get());
		render_.Assign("g_LightModelViewProj", params.g_LightModelViewProj);
	}

	void ProcessShadowMap()
	{
		glUseProgram(filter_.Get());

		filter_.Assign("g_KthPass", static_cast<int>(0));

		glBindImageTexture(0, rl_.GetColor(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16);
		glBindImageTexture(1, sm_tmp_.Get(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16);

		glDispatchCompute(
			width_ / 16,
			height_ / 16,
			1
		);

		glFinish();

		glUseProgram(filter_.Get());

		filter_.Assign("g_KthPass", static_cast<int>(1));

		glBindImageTexture(0, sm_tmp_.Get(), 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16);
		glBindImageTexture(1, rl_.GetColor(), 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA16);

		glDispatchCompute(
			width_ / 16,
			height_ / 16,
			1
		);

		glUseProgram(0);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	GLuint ShadowMap() const { return rl_.GetColor(); }

private:
	int width_, height_;
	FrameLayer rl_;
	TextureLayer sm_tmp_;
	Shader render_, filter_;
};

class Platte {

public:

	struct ParamsG {
		xy::mat4 g_LightViewProj;
		xy::mat4 g_ViewProj;
		xy::vec3 g_Eye;
		xy::vec3 g_SunLightDir;
		float g_DepthOffset;
		float g_MomentOffset;
		GLuint g_ShadowMap;
	};

	struct ParamsL {
		xy::mat4 g_Model;
		GLuint g_DiffuseMap;
		GLuint g_AlphaMap;
	};

public:

	Platte()
	{}

	void Init(int width, int height)
	{
		width_ = width;
		height_ = height;

		render_.Init(
			xy::ReadFile(xy_config::GetShaderPath("platte.vert")),
			xy::ReadFile(xy_config::GetShaderPath("platte.frag"))
		);
	}

	void BindPass()
	{
		glUseProgram(render_.Get());
		glEnable(GL_DEPTH_TEST);

		glViewport(0, 0, width_, height_);

		render_.Assign("g_ShadowMap", 0);
		render_.Assign("g_DiffuseMap", 1);
		render_.Assign("g_AlphaMap", 2);
	}

	void PassParams(ParamsG &params)
	{
		glUseProgram(render_.Get());

		render_.Assign("g_LightViewProj", params.g_LightViewProj);
		render_.Assign("g_ViewProj", params.g_ViewProj);
		render_.Assign("g_Eye", params.g_Eye);
		render_.Assign("g_SunLightDir", params.g_SunLightDir);
		render_.Assign("g_MomentOffset", params.g_MomentOffset);
		render_.Assign("g_DepthOffset", params.g_DepthOffset);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, params.g_ShadowMap);
	}

	void PassParams(ParamsL &params)
	{
		glUseProgram(render_.Get());

		render_.Assign("g_Model", params.g_Model);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, params.g_DiffuseMap);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, params.g_AlphaMap);

		if (params.g_AlphaMap == 0)
			render_.Assign("g_EnableAlphaToCoverage", static_cast<int>(-1));
		else
			render_.Assign("g_EnableAlphaToCoverage", static_cast<int>(1));
	}

private:
	int width_, height_;
	Shader render_;

};

class PPLLForHair {

public:

	struct ParamsG {
		xy::mat4 g_ViewProj;
		xy::vec3 g_Eye;
		xy::vec2 g_WinSize;
		xy::vec3 g_SunLightDir;
		float g_HairRadius;
		float g_HairTransparency;

		xy::mat4 g_LightViewProj;
		float g_DepthOffset;
		float g_MomentOffset;
		GLuint g_ShadowMap;

		GLuint g_HairBaseColorTex;
		GLuint g_HairSpecOffsetTex;
	};

	struct ParamsL {
		xy::mat4 g_Model;
	};

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
		std::vector<GLuint> clear_(screen_width_*screen_height_, 0xffffffff);
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
	}

	void BindStorePass()
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

		glUseProgram(store_pass_.Get());

		glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, 0, counter_buf_);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ll_buf_);
		glBindImageTexture(0, ll_head_tex_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);

		store_pass_.Assign("g_ShadowMap", 0);
		store_pass_.Assign("g_HairBaseColorTex", 1);
		store_pass_.Assign("g_HairSpecOffsetTex", 2);
	}

	void StorePassParams(ParamsG params)
	{
		glUseProgram(store_pass_.Get());
		store_pass_.Assign("g_HairRadius", params.g_HairRadius);
		store_pass_.Assign("g_HairTransparency", params.g_HairTransparency);
		store_pass_.Assign("g_Eye", params.g_Eye);
		store_pass_.Assign("g_ViewProj", params.g_ViewProj);
		store_pass_.Assign("g_WinSize", params.g_WinSize);
		store_pass_.Assign("g_NumNodes", static_cast<GLuint>(num_link_list_nodes_));

		store_pass_.Assign("g_LightViewProj", params.g_LightViewProj);
		store_pass_.Assign("g_SunLightDir", params.g_SunLightDir);
		store_pass_.Assign("g_MomentOffset", params.g_MomentOffset);
		store_pass_.Assign("g_DepthOffset", params.g_DepthOffset);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, params.g_ShadowMap);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, params.g_HairBaseColorTex);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, params.g_HairSpecOffsetTex);
	}

	void StorePassParams(ParamsL params)
	{
		glUseProgram(store_pass_.Get());
		store_pass_.Assign("g_Model", params.g_Model);
	}

	void BlendPassParams(ParamsG &params)
	{
		glUseProgram(blend_pass_.Get());

		blend_pass_.Assign("g_WinSize", params.g_WinSize);
		blend_pass_.Assign("g_NumNodes", static_cast<GLuint>(num_link_list_nodes_));
	}

	void BindBlendPass()
	{
		glUseProgram(blend_pass_.Get());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ll_buf_);
		glBindImageTexture(0, ll_head_tex_, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32UI);
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

		int num_link_list_nodes = screen_width_ * screen_height_ * 200;
		ppll_.Init(screen_width_, screen_height_, num_link_list_nodes);

		// Screen quad for PPLLForHair second pass.
		std::vector<xy::vec3> quad{ {-1,-1,0},{1,-1,0},{1,1,0},{-1,1,0} };
		screen_quad_vao_.SubmitBuf(quad, { 3 });

		////
		// Moment shadow map settings.
		////

		msm_.Init(screen_width_*2, screen_height_*2);

		////
		// Poly-rendering settings.
		////

		platte_.Init(screen_width_, screen_height_);
	}

	void Render(
		AABB &world_bounds,
		ObjAsset &obj_asset,
		FiberAsset &fiber_asset,
		Camera &camera,
		xy::vec4 background,
		xy::vec3 sun_light_dir,
		float msm_moment_offset,
		float msm_depth_offset,
		float ppll_HairRadius,
		float ppll_HairTransparency
	)
	{
		// Compute matrices.
		auto tgt = world_bounds.Center();
		auto radius = world_bounds.Lengths().Norm()*.25f;
		auto light_view_matrix = xy::LookAt(tgt + radius * sun_light_dir, tgt, { 0.f,1.f,0.f });
		auto light_proj_matrix = xy::Orthographic(-radius, radius, -radius, radius, 0.f, 4.f*radius);
		auto light_view_proj_matrix = light_proj_matrix * light_view_matrix;

		auto camera_view_proj_matrix = camera.Proj()*camera.View();

		//////
		//// Create moment shadow map.
		//////

		MSM::ParamsL msm_params;

		msm_.BindPass();

		msm_params.g_LightModelViewProj = light_view_proj_matrix * obj_asset.model_matrix;

		msm_.PassParams(msm_params);

		for (auto &shape : obj_asset.shapes)
			for (const auto &vao : shape.vaos)
				vao.Draw(GL_TRIANGLES, { 0 });

		glLineWidth(1.f);
		fiber_asset.vao.DrawLineStrips({ 0 },.1f);

		msm_.ProcessShadowMap();

		glBindFramebuffer(GL_FRAMEBUFFER, composite_layer_.Get());
		glClearColor(background.r, background.g, background.b, background.a);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

		////
		// Obj pass.
		////
		Platte::ParamsG platte_params_g;
		platte_params_g.g_ViewProj = camera_view_proj_matrix;
		platte_params_g.g_LightViewProj = light_view_proj_matrix;
		platte_params_g.g_Eye = camera.Pos();
		platte_params_g.g_SunLightDir = sun_light_dir;
		platte_params_g.g_MomentOffset = msm_moment_offset;
		platte_params_g.g_DepthOffset = msm_depth_offset;
		platte_params_g.g_ShadowMap = msm_.ShadowMap();

		platte_.BindPass();
		platte_.PassParams(platte_params_g);

		DrawObj(platte_, obj_asset);

		////
		// PPLL pass.
		////

		glBindFramebuffer(GL_FRAMEBUFFER, composite_layer_.Get());

		PPLLForHair::ParamsG ppll_params_g;
		ppll_params_g.g_Eye = camera.Pos();
		ppll_params_g.g_HairRadius = ppll_HairRadius;
		ppll_params_g.g_HairTransparency = ppll_HairTransparency;
		ppll_params_g.g_ViewProj = camera_view_proj_matrix;
		ppll_params_g.g_WinSize = xy::vec2(screen_width_, screen_height_);
		ppll_params_g.g_LightViewProj = light_view_proj_matrix;
		ppll_params_g.g_SunLightDir = sun_light_dir;
		ppll_params_g.g_DepthOffset = msm_depth_offset;
		ppll_params_g.g_MomentOffset = msm_moment_offset;
		ppll_params_g.g_ShadowMap = msm_.ShadowMap();
		ppll_params_g.g_HairBaseColorTex = fiber_asset.map_base_color;
		ppll_params_g.g_HairSpecOffsetTex = fiber_asset.map_spec_offset;

		PPLLForHair::ParamsL ppll_params_l;

		// Store pass.
		ppll_.BindStorePass();

		ppll_.StorePassParams(ppll_params_g);

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		ppll_params_l.g_Model = fiber_asset.model_matrix;
		ppll_.StorePassParams(ppll_params_l);

		fiber_asset.vao.DrawLineStrips({ 0,1,2 },1.f);

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		//// Two-pass sync.
		glFinish();

		glDisable(GL_DEPTH_TEST);
		ppll_.BindBlendPass();
		ppll_.BlendPassParams(ppll_params_g);

		glEnable(GL_BLEND);
		glBlendFuncSeparate(GL_ONE, GL_SRC_ALPHA, GL_ONE, GL_ONE);

		screen_quad_vao_.Draw(GL_TRIANGLE_FAN, { 0 });

		glDisable(GL_BLEND);

		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_TRUE);
	}

	void OutputFrame()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1, 1, 1, 1);
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

	void DrawObj(Platte &platte, ObjAsset &asset)
	{

		Platte::ParamsL platte_params_l;

		platte_params_l.g_Model = asset.model_matrix;

		for (auto &shape : asset.shapes) {
			int num_blobs = shape.blobs.size();
			for (int i = 0; i < num_blobs; ++i) {

				platte_params_l.g_AlphaMap = shape.map_d_textures[i];
				platte_params_l.g_DiffuseMap = shape.map_Kd_textures[i];

				platte.PassParams(platte_params_l);

				bool enable_alpha_to_coverage = (shape.map_d_textures[i] != 0);

				if (enable_alpha_to_coverage) {
					glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
					shape.vaos[i].Draw(GL_TRIANGLES, { 0,1,2 });
					glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
				}
				else {
					shape.vaos[i].Draw(GL_TRIANGLES, { 0,1,2 });
				}

			}
		}
	}

	int screen_width_, screen_height_;

	MSM msm_;
	Platte platte_;
	PPLLForHair ppll_;
	FrameLayer composite_layer_;

	GpuArray screen_quad_vao_;
};
