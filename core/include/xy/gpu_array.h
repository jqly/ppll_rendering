#ifndef XY_GPU_ARRAY
#define XY_GPU_ARRAY


#include <vector>
#include "glad/glad.h"


class GpuArray {
public:
	GpuArray();
	GpuArray(const GpuArray &) = delete;
	GpuArray& operator= (const GpuArray&) = delete;
	~GpuArray();

	template <typename T>
	void SubmitBuf(const std::vector<T> &buf, const std::vector<int> &&attrib_sizes)
	{
		if (!initialized) {
			Init();
			initialized = true;
		}

		if (vertex_count_ == 0)
			vertex_count_ = static_cast<int>(buf.size());
		if (vertex_count_ != buf.size())
			XY_Die("buffer has unequal length");

		if (cur_buf_binding_ >= 16)
			XY_Die("too many buffers");

		glGenBuffers(1, &bufs_[cur_buf_binding_]);

		glBindVertexArray(vao_);
		glBindBuffer(GL_ARRAY_BUFFER, bufs_[cur_buf_binding_]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(T)*buf.size(), buf.data(), GL_STATIC_DRAW);

		int offset = 0;
		for (auto attrib_size : attrib_sizes) {
			glVertexAttribPointer(cur_attrib_binding_, attrib_size, GL_FLOAT, GL_FALSE, sizeof(T), (void*)(offset));
			++cur_attrib_binding_;
			offset += attrib_size * sizeof(float);
		}
		++cur_buf_binding_;
	}

	void SetAsLineStrips(std::vector<int> num_lsverts);

	void Draw(GLenum mode, const std::vector<int> &&attribs) const;
	void DrawLineStrips(const std::vector<int> &&attribs, float keep_ratio) const;

private:
	bool initialized;
	GLuint vao_, bufs_[16];
	int cur_buf_binding_, cur_attrib_binding_;
	int vertex_count_;
	std::vector<int> num_lsverts_;

	void Init();
};



#endif // !XY_GPU_ARRAY