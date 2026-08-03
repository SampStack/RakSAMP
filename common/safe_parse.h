#pragma once

#include <cerrno>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

namespace raksamp::parse
{
template <std::size_t Size>
bool Copy(char (&destination)[Size], const char *source)
{
	static_assert(Size > 0, "destination must not be empty");
	if(source == nullptr)
		return false;
	const std::size_t length = std::strlen(source);
	if(length >= Size)
		return false;
	std::memcpy(destination, source, length + 1);
	return true;
}

template <typename Integer>
bool IntegerValue(
	const char *value,
	Integer &result,
	Integer minimum = std::numeric_limits<Integer>::lowest(),
	Integer maximum = (std::numeric_limits<Integer>::max)())
{
	static_assert(std::is_integral_v<Integer>, "IntegerValue requires an integer");
	if(value == nullptr || value[0] == '\0')
		return false;

	char *end = nullptr;
	errno = 0;
	if constexpr(std::is_signed_v<Integer>)
	{
		const long long parsed = std::strtoll(value, &end, 10);
		if(errno != 0 || end == value || *end != '\0' ||
			parsed < static_cast<long long>(minimum) ||
			parsed > static_cast<long long>(maximum))
			return false;
		result = static_cast<Integer>(parsed);
	}
	else
	{
		if(value[0] == '-')
			return false;
		const unsigned long long parsed = std::strtoull(value, &end, 10);
		if(errno != 0 || end == value || *end != '\0' ||
			parsed < static_cast<unsigned long long>(minimum) ||
			parsed > static_cast<unsigned long long>(maximum))
			return false;
		result = static_cast<Integer>(parsed);
	}
	return true;
}

inline bool FloatValue(
	const char *value,
	float &result,
	float minimum = -(std::numeric_limits<float>::max)(),
	float maximum = (std::numeric_limits<float>::max)())
{
	if(value == nullptr || value[0] == '\0')
		return false;
	char *end = nullptr;
	errno = 0;
	const float parsed = std::strtof(value, &end);
	if(errno != 0 || end == value || *end != '\0' ||
		!std::isfinite(parsed) || parsed < minimum || parsed > maximum)
		return false;
	result = parsed;
	return true;
}

inline bool HostAndPort(
	std::string_view value,
	std::string &host,
	unsigned short &port)
{
	const std::size_t separator = value.rfind(':');
	if(separator == std::string_view::npos || separator == 0 ||
		separator + 1 >= value.size())
		return false;
	const std::string hostValue(value.substr(0, separator));
	const std::string portValue(value.substr(separator + 1));
	unsigned int parsedPort = 0;
	if(hostValue.size() > 255 ||
		!IntegerValue<unsigned int>(portValue.c_str(), parsedPort, 1, 65535))
		return false;
	host = hostValue;
	port = static_cast<unsigned short>(parsedPort);
	return true;
}
}
