#ifndef XY_ASSET
#define XY_ASSET

#include <vector>
#include <fstream>
#include "glad/glad.h"

#include "xy_ext.h"
#include "xy_calc.h"
#include "gpu_array.h"


struct FiberAsset {
	std::vector<xy::vec3> positions;
	std::vector<xy::vec3> tangents;
	std::vector<float> scales;
	std::vector<int> num_verts_per_fiber;

	std::string map_bc_path;  // base color
	std::string map_sro_path;  // specular random offset

	GLuint map_base_color;
	GLuint map_spec_offset;
	GpuArray vao;

	void LoadFromFile(
		std::string model_path, 
		std::string base_color_texture_path,
		std::string specular_random_offset_texture_path);
	void CreateGpuRes();

	xy::mat4 model_matrix;
	std::string description;
};
 
struct ObjShape {
	struct Blob {
		std::vector<xy::vec3> positions;
		std::vector<xy::vec3> normals;
		std::vector<xy::vec2> texcoords;
	};

	std::vector<Blob> blobs;

	std::vector<xy::vec3> Ka;
	std::vector<xy::vec3> Kd;
	std::vector<xy::vec3> Ks;

	std::vector<std::string> map_Ka_image_paths;
	std::vector<std::string> map_Kd_image_paths;
	std::vector<std::string> map_Ks_image_paths;
	std::vector<std::string> map_d_image_paths;

	std::vector<GLuint> map_Ka_textures;
	std::vector<GLuint> map_Kd_textures;
	std::vector<GLuint> map_Ks_textures;
	std::vector<GLuint> map_d_textures;

	std::vector<GpuArray> vaos;

	std::vector<std::string> mtl_desc;
};

struct ObjAsset {
	std::vector<ObjShape> shapes;

	void LoadFromFile(std::string obj_path, std::string mtl_dirpath);
	void CreateGpuRes();

	xy::mat4 model_matrix;
	std::string description;
};



#endif // !XY_ASSET
