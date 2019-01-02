#ifndef XY_AABB
#define XY_AABB


#include "xy_calc.h"
#include <vector>


struct AABB {
	xy::vec3 inf, sup;

	AABB();
	AABB(const std::vector<xy::vec3> &ps);

	void Extend(const xy::vec3 &p);
	void Extend(const std::vector<xy::vec3> &ps);

	bool IsInside(const xy::vec3 &p) const;

	xy::vec3 Center() const;
	xy::vec3 Lengths() const;
};


#endif // !XY_AABB