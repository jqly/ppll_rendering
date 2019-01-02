#ifndef XY_EXT
#define XY_EXT


#include <iostream>
#include <string>
#include <chrono>
#include <fstream>
#include <memory>
#include <numeric>
#include <cassert>
#include <type_traits>
#include <array>

#include "xy_calc.h"


#define XY_Die(reason)  xy::__DieImpl(reason,__FILE__,__LINE__)

namespace xy
{


inline std::string ReadFile(std::string path)
{
	std::ifstream fin(path);

	assert(!fin.fail());

	fin.ignore(std::numeric_limits<std::streamsize>::max());
	auto size = fin.gcount();
	fin.clear();

	fin.seekg(0, std::ios_base::beg);
	auto source = std::unique_ptr<char>(new char[size]);
	fin.read(source.get(), size);

	return std::string(source.get(), static_cast<std::string::size_type>(size));
}

class Catcher {
public:
	Catcher(unsigned milli)
	{
		update_interval_ = std::chrono::milliseconds(milli);
		next_update_time_ = std::chrono::system_clock::now();
	}

	template<typename FN>
	void Sync(FN fn)
	{
		while (std::chrono::system_clock::now() > next_update_time_) {
			fn();
			next_update_time_ += update_interval_;
		}
	}

	void SyncNoOp()
	{
		next_update_time_ = std::chrono::system_clock::now();
	}

private:
	std::chrono::time_point<std::chrono::system_clock> next_update_time_;
	std::chrono::milliseconds update_interval_;
};

inline void Print(std::string format)
{
	std::cout << format;
}

template<typename T>
inline void Print(std::string format, const T &value)
{
	auto op = format.find('{', 0);
	assert(op != std::string::npos);
	auto ed = format.find('}', op + 1);
	assert(ed != std::string::npos);

	std::cout << format.substr(0, op);
	if (ed - op != 1) {
		std::ios_base::fmtflags fmt_old_flags{ std::cout.flags() };

		auto info = format.substr(op + 1, ed - op - 1);
		if (info == "hex") {
			if (std::is_integral<T>::value)
				std::cout << std::hex << value;
			else if (std::is_floating_point<T>::value)
				std::cout << std::hexfloat << value;
			else
				std::cout << value;
		}

		std::cout.flags(fmt_old_flags);
	}
	else {
		std::cout << value;
	}

	std::cout << format.substr(ed + 1, format.size() - ed - 1);
}

template<typename T, typename... Args>
inline void Print(std::string format, const T &value, const Args &...args)
{
	auto pos = format.find('}', 0);
	assert(pos != std::string::npos);
	Print(format.substr(0, pos + 1), value);
	Print(format.substr(pos + 1, format.size() - pos - 1), args...);
}

inline void __DieImpl(std::string reason, std::string file_path, int line_number)
{
	Print("ERROR({})\n", reason);
	Print("File({})\nLine({})\n", file_path, line_number);
	std::exit(1);
}

template<typename FN>
inline void TimeProfile(FN fn, unsigned num_iters, std::string tag)
{
	auto op_time = std::chrono::system_clock::now();
	for (unsigned i = 0; i < num_iters; ++i)
		fn();
	auto ed_time = std::chrono::system_clock::now();

	auto elapse = std::chrono::duration_cast<std::chrono::milliseconds>(ed_time - op_time).count();

	Print("{}:{}ms/{} loops\n", tag, elapse, num_iters);
}

template<typename FN>
inline long long TimeProfile(FN fn, unsigned num_iters)
{
	auto op_time = std::chrono::system_clock::now();
	for (unsigned i = 0; i < num_iters; ++i)
		fn();
	auto ed_time = std::chrono::system_clock::now();

	auto elapse = std::chrono::duration_cast<std::chrono::milliseconds>(ed_time - op_time).count();

	return elapse;
}

template<std::size_t N1, std::size_t N2, std::size_t... Idxs1, std::size_t... Idxs2>
constexpr std::array<char, N1 + N2 - 2> __ConcatImpl(
	const char(&a)[N1],
	const char(&b)[N2],
	std::index_sequence<Idxs1...>,
	std::index_sequence<Idxs2...>)
{
	return { a[Idxs1]...,b[Idxs2]... };
}

template<std::size_t N1, std::size_t N2>
constexpr auto __ConcatImpl(const char(&a)[N1], const char(&b)[N2])
{
	return __ConcatImpl(a, b, std::make_index_sequence<N1 - 1>(), std::make_index_sequence<N2 - 1>());
}

template<std::size_t N1, std::size_t N2, std::size_t... Idxs1, std::size_t... Idxs2>
constexpr std::array<char, N1 + N2 - 1> __ConcatImpl(
	const char(&a)[N1],
	const std::array<char, N2>(&b),
	std::index_sequence<Idxs1...>,
	std::index_sequence<Idxs2...>)
{
	return { a[Idxs1]...,b[Idxs2]... };
}

template<std::size_t N1, std::size_t N2>
constexpr auto __ConcatImpl(const char(&a)[N1], const std::array<char, N2>(&b))
{
	return __ConcatImpl(a, b, std::make_index_sequence<N1 - 1>(), std::make_index_sequence<N2>());
}

template<std::size_t N1, std::size_t N2, typename... Rests>
constexpr auto __ConcatImpl(const char(&a)[N1], const char(&b)[N2], const Rests&... args)
{
	return __ConcatImpl(a, __ConcatImpl(b, args...));
}

template<std::size_t N1, std::size_t N2>
inline std::string Concat(const char(&a)[N1], const std::array<char, N2>(&b))
{
	auto res = __ConcatImpl(a, b);
	return { res.data(),res.size() };
}

template<std::size_t N1, std::size_t N2, typename... Rests>
inline std::string Concat(const char(&a)[N1], const char(&b)[N2], const Rests&... args)
{
	auto res = __ConcatImpl(a, b, args...);
	return { res.data(),res.size() };
}


inline std::ostream &operator<<(std::ostream &os, const vec2 &v)
{
	return os <<
		std::string("(") +
		std::to_string(v.x) +
		"," +
		std::to_string(v.y) +
		")";
}

inline std::ostream &operator<<(std::ostream &os, const vec3 &v)
{
	return os <<
		std::string("(") +
		std::to_string(v.x) +
		"," +
		std::to_string(v.y) +
		"," +
		std::to_string(v.z) +
		")";
}

inline std::ostream &operator<<(std::ostream &os, const vec4 &v)
{
	return os <<
		std::string("(") +
		std::to_string(v.x) +
		"," +
		std::to_string(v.y) +
		"," +
		std::to_string(v.z) +
		"," +
		std::to_string(v.w) +
		")";
}

inline std::ostream &operator<<(std::ostream &os, const mat3 &m)
{
	return os <<
		"((" + std::to_string(m[0][0]) + "," + std::to_string(m[1][0]) + "," + std::to_string(m[2][0]) + "),\n" +
		" (" + std::to_string(m[0][1]) + "," + std::to_string(m[1][1]) + "," + std::to_string(m[2][1]) + "),\n" +
		" (" + std::to_string(m[0][2]) + "," + std::to_string(m[1][2]) + "," + std::to_string(m[2][2]) + "))\n";
}

inline std::ostream &operator<<(std::ostream &os, const mat4 &m)
{
	return os <<
		"((" + std::to_string(m[0][0]) + "," + std::to_string(m[1][0]) + "," + std::to_string(m[2][0]) + "," + std::to_string(m[3][0]) + "),\n" +
		" (" + std::to_string(m[0][1]) + "," + std::to_string(m[1][1]) + "," + std::to_string(m[2][1]) + "," + std::to_string(m[3][1]) + "),\n" +
		" (" + std::to_string(m[0][2]) + "," + std::to_string(m[1][2]) + "," + std::to_string(m[2][2]) + "," + std::to_string(m[3][2]) + "),\n" +
		" (" + std::to_string(m[0][3]) + "," + std::to_string(m[1][3]) + "," + std::to_string(m[2][3]) + "," + std::to_string(m[3][3]) + "))";
}

inline std::ostream &operator<<(std::ostream &os, const quat &q)
{
	return os <<
		std::string("(") +
		std::to_string(q.w) +
		"," +
		std::to_string(q.x) +
		"," +
		std::to_string(q.y) +
		"," +
		std::to_string(q.z) +
		")";
}


}


#endif // !XY_EXT

