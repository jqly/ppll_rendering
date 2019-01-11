#ifndef XY_CAMERA
#define XY_CAMERA


#include "xy_calc.h"


class Camera {
public:
	virtual void Init(xy::vec3 position, xy::vec3 target, int width, int height, float FoVy) = 0;
	virtual xy::vec3 Pos() const = 0;
	virtual xy::mat4 Proj() const = 0;
	virtual xy::mat4 View() const = 0;
	virtual void Zoom(float dist) = 0;
	virtual void Track(float x, float y, float z = 0) = 0;
};

class ArcballCamera : public Camera {
public:
	ArcballCamera();

	void Init(xy::vec3 position, xy::vec3 target, int width, int height, float FoVy) override;

	int Width() const { return width_; }
	int Height() const { return height_; }
	float Near() const { return 1.f; }
	float Far() const { return 100.0f; }
	void Zoom(float value) override;
	xy::vec3 Pos() const override;
	xy::mat4 Proj() const override;
	xy::mat4 View() const override;

	void Track(float mousex, float mousey, float z = 0) override;

private:
	bool ISect(float mousex, float mousey, xy::vec3 *nhit) const;

	float aspect_, FoVy_;
	int width_, height_;
	bool is_tracking_;
	xy::quat rot_, rot_prev_;
	xy::vec3 nhit_prev_;
	xy::vec3 pos_, tgt_;
};


class WanderCamera : public Camera {
public:
	WanderCamera();

	void Init(xy::vec3 position, xy::vec3 spot, int width, int height, float FoVy) override;

	void Zoom(float dist) override;
	int Width() const { return width_; }
	int Height() const { return height_; }
	xy::vec3 Pos() const override { return pos_; }
	xy::mat4 Proj() const override;
	xy::mat4 View() const override;
	float Near() const { return .1f; }
	float Far() const { return 100.f; }

	void Track(float mousex, float mousey, float z = 0) override;

private:

	bool ISect(float mousex, float mousey, xy::vec3 *nhit) const;

	float aspect_, FoVy_;
	int width_, height_;
	bool is_tracking_;
	xy::quat rot_, rot_prev_;
	xy::vec3 nhit_prev_;
	xy::vec3 pos_, ndir_, up_;
};


#endif // !XY_CAMERA