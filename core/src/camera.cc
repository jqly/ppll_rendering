#include "camera.h"

#include <string>
#include "xy_ext.h"
#include "camera.h"


ArcballCamera::ArcballCamera()
	:
	aspect_{ 0 },
	FoVy_{ 0 },
	width_{ 0 },
	height_{ 0 },
	rot_{ 1.f,0.f,0.f,0.f },
	rot_prev_{ 1.f,0.f,0.f,0.f },
	is_tracking_{ false },
	nhit_prev_{ 0 },
	pos_{ 0 },
	tgt_{ 0 }
{}

void ArcballCamera::Init(xy::vec3 position, xy::vec3 target, int width, int height, float FoVy)
{
	width_ = width;
	height_ = height;
	aspect_ = static_cast<float>(width) / height;
	FoVy_ = FoVy;
	pos_ = position;
	tgt_ = target;
}

void ArcballCamera::Zoom(float value)
{
	FoVy_ = xy::Clamp(FoVy_ + value, xy::DegreeToRadian(5.f), xy::DegreeToRadian(120.f));
}

xy::vec3 ArcballCamera::Pos() const
{
	auto pos = rot_.Conj()*(pos_ - tgt_) + tgt_;
	return pos;
}

xy::mat4 ArcballCamera::Proj() const
{
	return xy::Perspective(FoVy_, aspect_, Near(), Far());
}

xy::mat4 ArcballCamera::View() const
{
	auto trans = xy::Translation(-tgt_);
	auto transinv = xy::Translation(tgt_);
	auto view = xy::LookAt(pos_, tgt_, { .0f,1.f,.0f });
	return view * transinv*xy::QuatToMat4(xy::Normalize(rot_))*trans;
}

void ArcballCamera::Track(float mousex, float mousey, float z)
{
	if (mousex < 0.f || mousey < 0.f || mousex > Width() || mousey > Height()) {
		is_tracking_ = false;
		return;
	}

	xy::vec3 nhit;
	if (!ISect(mousex, mousey, &nhit))
		return;

	if (!is_tracking_) {
		nhit_prev_ = nhit;
		rot_prev_ = rot_;

		is_tracking_ = true;
	}

	if ((nhit - nhit_prev_).Norm() < xy::big_eps<float>)
		return;

	auto qrot_axis = Cross(nhit_prev_, nhit);
	qrot_axis = rot_prev_.Conj()*qrot_axis;

	float dw = acos(xy::Clamp(Dot(nhit_prev_, nhit), -1.f, 1.f));
	auto qrot = xy::AngleAxisToQuat(5.f*dw, Normalize(qrot_axis));
	rot_ = rot_prev_ * qrot;
}

bool ArcballCamera::ISect(float mousex, float mousey, xy::vec3 * nhit) const
{
	float y = height_ - 1 - mousey, x = mousex;
	xy::vec3 winpos{ x,y,0.f };
	xy::vec4 view_port{ 0,0,static_cast<float>(width_),static_cast<float>(height_) };

	auto unitpos_ = Normalize(pos_ - tgt_);

	auto view = LookAt(
		unitpos_,
		{ 0.f,0.f,0.f },
		xy::vec3{ .0f,1.f,.0f }
	);

	auto tmp11 = UnProject(
		winpos,
		view,
		xy::Perspective(FoVy_, aspect_, Near(), Far()),
		view_port
	);

	xy::vec3 ro = unitpos_;
	xy::vec3 rd = Normalize(tmp11 - ro);

	float r = .71f;  // Arcball radius.

	float a = 1.f;  // dot(rd,rd);
	float b = 2.f*Dot(ro, rd);
	float c = Dot(ro, ro) - r * r;

	float delta = b * b - 4 * a*c;
	if (delta < xy::big_eps<float>)
		return false;

	float tmp1 = -b / (2.f*a);
	float tmp2 = sqrt(delta) / (2.f*a);

	float ans1 = tmp1 + tmp2, ans2 = tmp1 - tmp2;
	float t = ans1 < ans2 ? ans1 : ans2;
	*nhit = Normalize(ro + t * rd);
	return true;
}

WanderCamera::WanderCamera()
	:
	aspect_{ 0 },
	FoVy_{ 0 },
	width_{ 0 },
	height_{ 0 },
	is_tracking_{ false },
	rot_{ 1.f,0.f,0.f,0.f },
	rot_prev_{ 1.f,0.f,0.f,0.f },
	nhit_prev_{ 0 },
	up_{ 0.f,1.f,0.f },
	pos_{ 0 },
	ndir_{0}
{}

void WanderCamera::Init(xy::vec3 position, xy::vec3 spot, int width, int height, float FoVy)
{
	width_ = width;
	height_ = height;
	aspect_ = static_cast<float>(width) / height;
	FoVy_ = FoVy;
	pos_ = position;
	ndir_ = Normalize(spot - position);
}

void WanderCamera::Zoom(float dist)
{
	pos_ += (rot_ * ndir_) * dist;
}

xy::mat4 WanderCamera::Proj() const
{
	return xy::Perspective(FoVy_, aspect_, Near(), Far());
}

xy::mat4 WanderCamera::View() const
{
	auto view = xy::LookAt(pos_, pos_ + xy::Normalize(rot_) * ndir_, up_);
	return view;
}

void WanderCamera::Track(float mousex, float mousey, float z)
{
	if (mousex < 0.f || mousey < 0.f || mousex > Width() || mousey > Height()) {
		rot_prev_ = rot_;
		is_tracking_ = false;
		return;
	}

	xy::vec3 nhit;
	if (!ISect(mousex, mousey, &nhit))
		return;

	if (!is_tracking_) {
		nhit_prev_ = nhit;
		is_tracking_ = true;
		return;
	}

	if ((nhit - nhit_prev_).Norm() < xy::big_eps<float>)
		return;

	auto qrot_axis = Cross(nhit, nhit_prev_);
	qrot_axis = rot_prev_.Conj()*qrot_axis;

	float dw = acos(xy::Clamp(Dot(nhit_prev_, nhit), -1.f, 1.f));
	auto qrot = xy::AngleAxisToQuat(dw, Normalize(qrot_axis));
	rot_ = rot_prev_ * qrot;
}

bool WanderCamera::ISect(float mousex, float mousey, xy::vec3 * nhit) const
{
	float y = height_ - 1 - mousey, x = mousex;
	xy::vec3 winpos{ x,y,0.f };
	xy::vec4 view_port{ 0,0,static_cast<float>(width_),static_cast<float>(height_) };

	auto unitpos_ = Normalize(pos_);

	auto view = xy::LookAt(pos_, pos_ + xy::Normalize(rot_prev_) * ndir_, up_);
	auto tmp11 = UnProject(
		winpos,
		view,
		Proj(),
		view_port
	);

	*nhit = Normalize(tmp11 - pos_);
	return true;
}
