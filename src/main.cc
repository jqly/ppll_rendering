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

#include "shader.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


void HandleInput(GLFWWindowDesc &window, Camera &camera);

void ImguiInit(GLFWwindow *window);
void ImguiOverlay(DrawShaderParams &draw_shader_params);
void ImguiExit();

int main()
{
	GLFWWindowDesc window(xy_config::screen_width,xy_config::screen_height, "c01dbeef");
	ArcballCamera camera;
	camera.Init({ 3.71f,2.25f,2.47f }, { 0.f }, xy_config::screen_width, xy_config::screen_height, xy::DegreeToRadian(60.f));

	HandleInput(window, camera);

	AABB fiber_bound({ {1,1,-1}, {-1,0,1} });
	FiberAsset fiber_asset;
	fiber_asset.LoadFromFile(
		xy_config::GetAssetPath("fibers_on_plane.ind"),
		xy_config::GetAssetPath("hair_base_color.jpg"),
		xy_config::GetAssetPath("hair_spec_offset.jpg"));
	//xy::Print("#fibers={}\n", fiber_asset.num_verts_per_fiber.size());
	fiber_asset.CreateGpuRes();
	fiber_asset.model_matrix = xy::mat4(1.f);

	Draw draw;
	draw.Init(xy_config::screen_width, xy_config::screen_height, 0);

	// FPS counter.
	int frame_cnt = 0;
	xy::Catcher c{ 5000 };

	// Background of our composite layer.
	xy::vec4 bg(1, 1, 1, 1);

	// Setting Dear ImGUI.
	ImguiInit(window.wptr);

	DrawShaderParams draw_shader_params;
	draw_shader_params.g_HairRadius = 1.f;
	while (!glfwWindowShouldClose(window.wptr) && window.alive) {
		glfwPollEvents();

		draw.Render(fiber_asset, camera, bg, draw_shader_params);

		draw.OutputFrame();

		ImguiOverlay(draw_shader_params);

		glfwSwapBuffers(window.wptr);
	}

	ImguiExit();

	return 0;
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
	DrawShaderParams &draw_shader_params
)
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Tweak");
	ImGui::Text("(%.2fms,%.0ffps)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::SliderFloat("Hair radius", &draw_shader_params.g_HairRadius, 0.f, 5.f);

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