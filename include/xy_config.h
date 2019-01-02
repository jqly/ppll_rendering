#ifndef XY_CONFIG
#define XY_CONFIG


#include <string>
#include <filesystem>

#include "xy/xy_ext.h"


namespace xy_config
{


template<std::size_t N>
inline std::string GetShaderPath(const char(&a)[N])
{
	auto path = xy::Concat("D:/jqlyg/ppll_rendering/shader/", a);

	if (!std::experimental::filesystem::exists(path))
		XY_Die(path + " does not exist");

	return path;
}

template<std::size_t N>
inline std::string GetAssetPath(const char(&a)[N])
{
	auto path = xy::Concat("D:/jqlyg/ppll_rendering/asset/", a);

	if (!std::experimental::filesystem::exists(path))
		XY_Die(path + " does not exist");

	return path;
}

static int screen_width = 256, screen_height = 256;


}



#endif // !XY_CONFIG
