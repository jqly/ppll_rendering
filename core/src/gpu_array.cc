#include "gpu_array.h"

#include <vector>
#include "glad/glad.h"
#include "xy_ext.h"


GpuArray::GpuArray()
	:
	vao_{ 0 },
	bufs_{0},
	cur_buf_binding_{ 0 }, 
	cur_attrib_binding_{ 0 },
	vertex_count_{ 0 },
	initialized{ false }
{}

GpuArray::~GpuArray()
{
	glDeleteVertexArrays(1, &vao_);
	glDeleteBuffers(16, bufs_);
}

void GpuArray::SetAsLineStrips(std::vector<int> num_lsverts)
{
	num_lsverts_.swap(num_lsverts);
}

void GpuArray::Draw(GLenum mode, const std::vector<int> &&attribs) const
{
	glBindVertexArray(vao_);
	for (auto attrib : attribs)
		glEnableVertexAttribArray(attrib);

	glDrawArrays(mode, 0, vertex_count_);

	for (auto attrib : attribs)
		glDisableVertexAttribArray(attrib);
	glBindVertexArray(0);
}

void GpuArray::DrawLineStrips(const std::vector<int> &&attribs) const
{
	glBindVertexArray(vao_);
	for (auto attrib : attribs)
		glEnableVertexAttribArray(attrib);

	int accsum = 0;
	for (auto nverts : num_lsverts_) {
		glDrawArrays(GL_LINE_STRIP, accsum, nverts);
		accsum += nverts;
	}

	for (auto attrib : attribs)
		glDisableVertexAttribArray(attrib);
	glBindVertexArray(0);
}

void GpuArray::Init()
{
	for (int i = 0; i < 16; ++i)
		bufs_[i] = 0;
	glGenVertexArrays(1, &vao_);
	if (vao_ == 0)
		XY_Die("glGenVertexArrays failed");
}