#include "aabb.h"

AABB::AABB()
	:
	inf{ std::numeric_limits<float>::max() },
	sup{ std::numeric_limits<float>::lowest() }
{}

AABB::AABB(const std::vector<xy::vec3>& ps)
	: AABB()
{
	for (const auto &p : ps)
		Extend(p);
}

void AABB::Extend(const xy::vec3 & p)
{
	inf = CompMin(inf, p);
	sup = CompMax(sup, p);
}

void AABB::Extend(const std::vector<xy::vec3>& ps)
{
	for (auto &p : ps)
		Extend(p);
}

bool AABB::IsInside(const xy::vec3 & p) const
{
	return p.x <= sup.x && p.y <= sup.y && p.z <= sup.z &&
		p.x >= inf.x && p.y >= inf.y && p.z >= inf.z;
}

xy::vec3 AABB::Center() const
{
	return (inf + sup) / 2.f;
}

xy::vec3 AABB::Lengths() const
{
	return sup - inf;
}
