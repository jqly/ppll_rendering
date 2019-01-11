#include "asset.h"

#include <map>
#include "glad/glad.h"
#include "tiny_obj_loader.h"
#include "stb_image.h"
#include "xy_ext.h"
#include "gpu_array.h"


void ObjAsset::LoadFromFile(std::string obj_path, std::string mtl_dir)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> raw_shapes;
	std::vector<tinyobj::material_t> materials;
	std::string errmsg;

	bool is_success = tinyobj::LoadObj(&attrib, &raw_shapes, &materials, &errmsg, obj_path.c_str(), mtl_dir.c_str(), true);
	if (!is_success) {
		XY_Die(errmsg);
	}

	if (attrib.vertices.empty())
		xy::Print("obj empty vertex list");
	if (attrib.normals.empty())
		xy::Print("obj empty normal list");
	if (attrib.texcoords.empty())
		xy::Print("obj empty texcoord list");

	shapes.clear();
	shapes.resize(raw_shapes.size());

	for (int kthshape = 0; kthshape < raw_shapes.size(); ++kthshape) {

		auto &raw_shape = raw_shapes[kthshape];
		auto &shape = shapes[kthshape];

		std::vector<int> mtl_ids = raw_shape.mesh.material_ids;
		{
			std::sort(mtl_ids.begin(), mtl_ids.end());
			auto last = std::unique(mtl_ids.begin(), mtl_ids.end());
			mtl_ids.erase(last, mtl_ids.end());

			auto num_blobs = mtl_ids.size();

			std::vector<ObjShape::Blob>(num_blobs).swap(shape.blobs);
			std::vector<xy::vec3>(num_blobs).swap(shape.Ka);
			std::vector<xy::vec3>(num_blobs).swap(shape.Kd);
			std::vector<xy::vec3>(num_blobs).swap(shape.Ks);
			std::vector<std::string>(num_blobs, "").swap(shape.map_Ka_image_paths);
			std::vector<std::string>(num_blobs, "").swap(shape.map_Kd_image_paths);
			std::vector<std::string>(num_blobs, "").swap(shape.map_Ks_image_paths);
			std::vector<std::string>(num_blobs, "").swap(shape.map_d_image_paths);
			std::vector<std::string>(num_blobs, "").swap(shape.mtl_desc);

			for (int i = 0; i < num_blobs; ++i) {
				auto &mtl = materials[mtl_ids[i]];

				shape.Ka[i] = xy::FloatArrayToVec(mtl.ambient);
				shape.Kd[i] = xy::FloatArrayToVec(mtl.diffuse);
				shape.Ks[i] = xy::FloatArrayToVec(mtl.specular);
				if (mtl.ambient_texname != "")
					shape.map_Ka_image_paths[i] = mtl_dir + mtl.ambient_texname;
				if (mtl.diffuse_texname != "")
					shape.map_Kd_image_paths[i] = mtl_dir + mtl.diffuse_texname;
				if (mtl.specular_texname != "")
					shape.map_Ks_image_paths[i] = mtl_dir + mtl.specular_texname;
				if (mtl.alpha_texname != "")
					shape.map_d_image_paths[i] = mtl_dir + mtl.alpha_texname;
				shape.mtl_desc[i] = mtl.name;
			}

			std::vector<int> blob_lookup(materials.size(), -1);
			{
				for (int i = 0; i < mtl_ids.size(); ++i)
					blob_lookup[mtl_ids[i]] = i;
			}

			auto num_faces = raw_shape.mesh.num_face_vertices.size();
			int index_offset = 0;

			for (int kthface = 0; kthface < num_faces; ++kthface) {
				auto mtl_id = raw_shape.mesh.material_ids[kthface];
				auto &blob = shape.blobs[blob_lookup[mtl_id]];

				for (int kthvert = 0; kthvert < 3; ++kthvert) {
					auto &idxset = raw_shape.mesh.indices[index_offset + kthvert];

					if (!attrib.vertices.empty()) {
						float x = attrib.vertices[3 * idxset.vertex_index + 0];
						float y = attrib.vertices[3 * idxset.vertex_index + 1];
						float z = attrib.vertices[3 * idxset.vertex_index + 2];
						blob.positions.emplace_back(x, y, z);
					}

					if (!attrib.normals.empty()) {
						float nx = attrib.normals[3 * idxset.normal_index + 0];
						float ny = attrib.normals[3 * idxset.normal_index + 1];
						float nz = attrib.normals[3 * idxset.normal_index + 2];
						blob.normals.emplace_back(nx, ny, nz);
					}

					if (!attrib.texcoords.empty()) {
						float tx = attrib.texcoords[2 * idxset.texcoord_index + 0];
						float ty = attrib.texcoords[2 * idxset.texcoord_index + 1];
						blob.texcoords.emplace_back(tx, ty);
					}
				}
				index_offset += 3;
			}
		}
	}
}

GLuint LoadTextureFromFile(std::string tex_path)
{
	int width, height, num_channels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(tex_path.c_str(), &width, &height, &num_channels, 0);
	if (data == nullptr)
		XY_Die(std::string("failed to load texture(") + tex_path + ")");

	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (num_channels == 4) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	else if (num_channels == 3) {
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	else
		XY_Die("Unsupported texture format(#channels not 3 or 4)");

	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
	return tex;
}

void ObjAsset::CreateGpuRes()
{
	std::map<std::string, GLuint> map;
	map[""] = 0;
	for (int i = 0; i < shapes.size(); ++i) {
		for (auto &key : shapes[i].map_d_image_paths)
			if (map.count(key) == 0)
				map[key] = LoadTextureFromFile(key);
		for (auto &key : shapes[i].map_Ka_image_paths)
			if (map.count(key) == 0)
				map[key] = LoadTextureFromFile(key);
		for (auto &key : shapes[i].map_Kd_image_paths)
			if (map.count(key) == 0)
				map[key] = LoadTextureFromFile(key);
		for (auto &key : shapes[i].map_Ks_image_paths)
			if (map.count(key) == 0)
				map[key] = LoadTextureFromFile(key);
	}

	for (int i = 0; i < shapes.size(); ++i) {
		for (auto &key : shapes[i].map_d_image_paths)
			shapes[i].map_d_textures.push_back(map[key]);
		for (auto &key : shapes[i].map_Ka_image_paths)
			shapes[i].map_Ka_textures.push_back(map[key]);
		for (auto &key : shapes[i].map_Kd_image_paths)
			shapes[i].map_Kd_textures.push_back(map[key]);
		for (auto &key : shapes[i].map_Ks_image_paths)
			shapes[i].map_Ks_textures.push_back(map[key]);
	}

	for (auto &shape : shapes) {
		auto num_blobs = shape.blobs.size();
		shape.vaos.swap(std::vector<GpuArray>(num_blobs));
		for (int i = 0; i < num_blobs; ++i) {
			shape.vaos[i].SubmitBuf(shape.blobs[i].positions, { 3 });
			shape.vaos[i].SubmitBuf(shape.blobs[i].normals, { 3 });
			shape.vaos[i].SubmitBuf(shape.blobs[i].texcoords, { 2 });
		}
	}

}

void FiberAsset::LoadFromFile(
	std::string model_path,
	std::string base_color_texture_path,
	std::string specular_random_offset_texture_path)
{
	map_bc_path = base_color_texture_path;
	map_sro_path = specular_random_offset_texture_path;

	std::ifstream fp(model_path, std::ios::binary);

	char header[9];
	fp.read(header, 8);
	header[8] = '\0';
	if (strcmp(header, "IND_HAIR") != 0)
		XY_Die("Hair file's header not match!\n");

	auto read_unsigned = [&fp]()->unsigned {
		unsigned n;
		fp.read(reinterpret_cast<char*>(&n), sizeof(n));
		return n;
	};

	auto read_float = [&fp]()->float {
		float n;
		fp.read(reinterpret_cast<char*>(&n), sizeof(n));
		return n;
	};

	unsigned num_fibers = read_unsigned();
	unsigned num_total_verts = read_unsigned();

	for (unsigned kthfib = 1; kthfib <= num_fibers; ++kthfib) {
		auto vert_count = read_unsigned();

		//if (kthfib % 2 != 0) {
		//	fp.ignore(3 * sizeof(float) * vert_count);
		//	continue;
		//}

		num_verts_per_fiber.push_back(vert_count);

		for (unsigned kthp = 0; kthp < vert_count; ++kthp) {
			float x = read_float();
			float y = read_float();
			float z = read_float();
			// FLIP
			positions.emplace_back(x, y, z);
		}
	}

	tangents.resize(positions.size());
	for (std::size_t kthvert = 0,
		kthfib = 0; kthfib < num_verts_per_fiber.size(); ++kthfib) {
		for (std::size_t i = 0; i < num_verts_per_fiber[kthfib] - 1; ++i) {
			tangents[kthvert] = xy::Normalize(positions[kthvert + 1] - positions[kthvert]);
			++kthvert;
		}
		tangents[kthvert] = xy::Normalize(positions[kthvert] - positions[kthvert - 1]);
		++kthvert;
	}

	scales.reserve(positions.size());
	// scale reuse: An integer is attached to each fiber.
	xy::RandomEngine eng{ 0xc01dbeef };
	for (int kthfiber = 0; kthfiber < num_verts_per_fiber.size(); ++kthfiber) {
		int nvert = num_verts_per_fiber[kthfiber];
		int fibrandom = xy::Unif<1, 99>(eng);
		for (int i = 0; i < nvert; ++i) {
			//auto dist = static_cast<float>(i) / (nvert - 1.f);
			//scales.push_back(fibrandom + xy::Lerp(.95f, .05f, 2.f*(abs(.5 - dist))));
			if (i < nvert - 1)
				scales.push_back(fibrandom + .99f);
			else
				scales.push_back(fibrandom + .25f);
		}
	}

	vao.SetAsLineStrips(num_verts_per_fiber);
}

void FiberAsset::CreateGpuRes()
{
	if (positions.size() != tangents.size() || positions.size() != scales.size())
		XY_Die("Incomplete fiber asset!");
	vao.SubmitBuf(positions, { 3 });
	vao.SubmitBuf(tangents, { 3 });
	vao.SubmitBuf(scales, { 1 });

	// TODO: Add base color & specular random offset texture.
	map_base_color = LoadTextureFromFile(map_bc_path);
	map_spec_offset = LoadTextureFromFile(map_sro_path);
}
