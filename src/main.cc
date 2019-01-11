#include "xy/xy_calc.h"
#include "xy/xy_ext.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "xy/camera.h"
#include "xy/window.h"
#include "xy_config.h"
#include "xy/asset.h"
#include "xy/aabb.h"
#include "xy/xy_calc.h"

#include "shader.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


void HandleInput(GLFWWindowDesc &window, Camera &camera);

struct GameParams {
	float ppll_hair_radius;
	float ppll_hair_transparency;
	float msm_moments_offset;
	float msm_depth_offset;
};

void ImguiInit(GLFWwindow *window);
void ImguiOverlay(GameParams &params);
void ImguiExit();

int GameALL();

float MSMComputeShadow(
	xy::vec4 moments,
	float fragment_depth,
	float depth_bias,
	float moment_bias)
{
	xy::vec4 b = xy::Lerp(moments, xy::vec4(.5, .5, .5, .5), moment_bias);

	// OpenGL 4 only - fma has higher precision:
	float l32_d22 = -b.x * b.y + b.z; // a * b + c
	float d22 = -b.x * b.x + b.y;     // a * b + c
	float squared_depth_variance = -b.x* b.y + b.z; // a * b + c

	float d33_d22 = xy::Dot(
		xy::vec2(squared_depth_variance, -l32_d22), xy::vec2(d22, l32_d22));
	float inv_d22 = 1. - d22;
	float l32 = l32_d22 * inv_d22;

	xy::vec3 z;
	z[0] = fragment_depth - depth_bias;

	xy::vec3 c = xy::vec3(1., z[0], z[0] * z[0]);
	c.y -= b.x;
	c.z -= b.y + l32 * c.y;
	c.y *= inv_d22;
	c.z *= d22 / d33_d22;
	c.y -= l32 * c.z;
	c.x -= xy::Dot(xy::vec2(c.y, c.z), xy::vec2(b.x, b.y));

	float inv_c2 = 1. / c.z;
	float p = c.y * inv_c2;
	float q = c.x * inv_c2;
	float r = sqrt((p * p * .25) - q);

	z[1] = -p * .5 - r;
	z[2] = -p * .5 + r;

	xy::vec4 tmp =
		(z[2] < z[0]) ? xy::vec4(z[1], z[0], 1., 1.) :
		((z[1] < z[0]) ? xy::vec4(z[0], z[1], 0., 1.) :
			xy::vec4(0., 0., 0., 0.));
	float quotient = (
		tmp[0] * z[2] - b[0] * (tmp[0] + z[2]) + b[1]) / ((z[2] - tmp[1])*(z[0] - z[1]));

	return tmp[2] + tmp[3] * quotient;
}

xy::vec4 MSM_OptimizedMoments(float depth)
{
	float depth_sq = depth * depth;
	xy::vec4 moments = xy::vec4(depth, depth_sq, depth_sq*depth, depth_sq*depth_sq);

	//xy::mat4 T = xy::mat4(
	//	{ 1.5, 0, 0.86602540378443864676372317075294, 0 },
	//	{ 0, 4, 0, .5 },
	//	{ -2, 0, -0.38490017945975050967276585366797, 0 },
	//	{ 0, -4, 0, .5 });

	xy::mat4 T = xy::mat4(
		{ -2.07224649f,    13.7948857237f,  0.105877704f,   9.7924062118f },
		{ 32.23703778f,  -59.4683975703f, -1.9077466311f, -33.7652110555f },
		{ -68.571074599f,  82.0359750338f,  9.3496555107f,  47.9456096605f },
		{ 39.3703274134f,-35.364903257f,  -6.6543490743f, -23.9728048165f });
	//T = T.T();
	//xy::vec4 opt_moments = T * moments + xy::vec4(.5f, 0, .5f, 0);

	auto opt_moments = T * moments;
	opt_moments[0] += 0.035955884801f;

	return opt_moments;
}

xy::vec4 MSM_ConvertOptimizedMoments(xy::vec4 opt_moments)
{
	//opt_moments -= xy::vec4(.5f, 0, .5f, 0);
	//xy::mat4 T = xy::mat4(
	//	{ -0.33333333333333333333333333333333, 0, -.75, 0 },
	//	{ 0, .125, 0, -.125 },
	//	{ 1.7320508075688772935274463415059, 0, 1.2990381056766579701455847561294, 0 },
	//	{ 0, 1, 0, 1 });

	opt_moments[0] -= 0.035955884801f;
	xy::mat4 T = xy::mat4(
		{ 0.2227744146f, 0.1549679261f, 0.1451988946f, 0.163127443f },
		{ 0.0771972861f, 0.1394629426f, 0.2120202157f, 0.2591432266f },
		{ 0.7926986636f, 0.7963415838f, 0.7258694464f, 0.6539092497f },
		{ 0.0319417555f,-0.1722823173f,-0.2758014811f,-0.3376131734f });

	return T * opt_moments;
}

void TestMSM()
{
	for (int i = 0; i < 100; i += 1) {
		float depth = .01*i;
		auto moments = MSM_ConvertOptimizedMoments(MSM_OptimizedMoments(depth));
		xy::Print(
			"{},{},{},{},{}\n", depth, moments.x, moments.y, moments.z, moments.w
		);
	}
}

int main()
{
	GameALL();

}

void HandleInput(GLFWWindowDesc & window, Camera &camera)
{
	window.cursor_handler = [&window, &camera](float x, float y) {
		if (ImGui::GetIO().WantCaptureMouse) { return; }

		if (window.mouse_state == GLFWWindowDesc::MouseState::Press)
			camera.Track(x, y);

		if (window.mouse_state == GLFWWindowDesc::MouseState::Release)
			camera.Track(-1.f, -1.f);

	};

	window.scroll_handler = [&camera](float dx, float dy) {
		if (ImGui::GetIO().WantCaptureMouse) { return; }

		camera.Zoom(.1f*dy);
	};
}

void ImguiInit(GLFWwindow *window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init();
	ImGui::StyleColorsDark();
}

void ImguiOverlay(
	GameParams &params
)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Tweak");
	ImGui::Text("(%.2fms,%.0ffps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::SliderFloat("Hair radius", &params.ppll_hair_radius, 0.f, 5.f);
	ImGui::SliderFloat("Hair transparency", &params.ppll_hair_transparency, 0.f, 1.f);

	ImGui::SliderFloat("MSM moments offset", &params.msm_moments_offset, 0.f, 1.f);
	ImGui::SliderFloat("MSM depth offset", &params.msm_depth_offset, 0.f, 1.f);

	ImGui::End();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ImguiExit()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

int GameALL()
{
	GLFWWindowDesc window(xy_config::screen_width, xy_config::screen_height, "c01dbeef");
	WanderCamera camera;
	camera.Init({ 0,1.f,2.f }, { 0,1.f,0 }, xy_config::screen_width, xy_config::screen_height, xy::DegreeToRadian(45.f));

	HandleInput(window, camera);

	AABB world_bound({ {-4,-4,-4}, {4,4,4} });
	FiberAsset fiber_asset;
	//fiber_asset.LoadFromFile(
	//	xy_config::GetAssetPath("yuksel/curly.ind"),
	//	xy_config::GetAssetPath("hair/hair_base_color.jpg"),
	//	xy_config::GetAssetPath("hair/hair_spec_offset.jpg"));
	//xy::Print("#fibers={}\n", fiber_asset.num_verts_per_fiber.size());

	fiber_asset.LoadFromFile(
		xy_config::GetAssetPath("blender_girl/blender_girl_hair.ind"),
		//xy_config::GetAssetPath("fibers_on_plane.ind"),
		xy_config::GetAssetPath("hair/hair_base_color.jpg"),
		xy_config::GetAssetPath("hair/hair_spec_offset.jpg"));

	fiber_asset.CreateGpuRes();
	fiber_asset.model_matrix = xy::mat4(1.f);

	ObjAsset obj_asset;
	//obj_asset.LoadFromFile(xy_config::GetAssetPath("yuksel/woman.obj"), xy_config::GetAssetPath("yuksel/"));
	obj_asset.LoadFromFile(xy_config::GetAssetPath("blender_girl/blender_girl.obj"), xy_config::GetAssetPath("blender_girl/"));
	obj_asset.CreateGpuRes();
	obj_asset.model_matrix = xy::mat4(1.f);

	Draw draw;
	draw.Init(xy_config::screen_width, xy_config::screen_height, 0);

	// FPS counter.
	int frame_cnt = 0;
	xy::Catcher c{ 5000 };

	// Background of our composite layer.
	xy::vec4 bg(1, 1, 1, 1);

	// Setting Dear ImGUI.
	ImguiInit(window.wptr);

	GameParams game_params;
	game_params.ppll_hair_radius = 1.f;
	game_params.ppll_hair_transparency = .9f;
	game_params.msm_moments_offset = .0f;
	game_params.msm_depth_offset = .0f;

	xy::vec3 sun_light_dir(1.f, 1.f, 1.f);

	while (!glfwWindowShouldClose(window.wptr) && window.alive) {

		glfwPollEvents();

		draw.Render(
			world_bound,
			obj_asset,
			fiber_asset,
			camera,
			bg,
			sun_light_dir,
			game_params.msm_moments_offset,
			game_params.msm_depth_offset,
			game_params.ppll_hair_radius,
			game_params.ppll_hair_transparency
		);

		draw.OutputFrame();

		ImguiOverlay(game_params);

		glfwSwapBuffers(window.wptr);

	}

	ImguiExit();

	return 0;
}