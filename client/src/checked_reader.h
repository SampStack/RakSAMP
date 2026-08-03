#pragma once

#include <cmath>
#include <cstddef>
#include <string>

namespace raksamp::protocol
{
template <typename Stream>
class CheckedReader
{
public:
	explicit CheckedReader(Stream &stream) : stream_(stream) {}

	template <typename... Values>
	bool Read(Values &...values)
	{
		return (stream_.Read(values) && ...);
	}

	bool Bytes(char *destination, std::size_t length)
	{
		return destination != nullptr &&
			stream_.Read(destination, static_cast<int>(length));
	}

	bool String8(std::string &result, std::size_t maximumLength)
	{
		unsigned char length = 0;
		if(!stream_.Read(length) || length > maximumLength)
			return false;
		std::string value(length, '\0');
		if(length > 0 &&
			!stream_.Read(value.data(), static_cast<int>(length)))
			return false;
		result = std::move(value);
		return true;
	}

	bool Finite(float &value)
	{
		float parsed = 0.0f;
		if(!stream_.Read(parsed) || !std::isfinite(parsed))
			return false;
		value = parsed;
		return true;
	}

	bool Finite3(float (&values)[3])
	{
		float parsed[3] = {};
		if(!Finite(parsed[0]) || !Finite(parsed[1]) || !Finite(parsed[2]))
			return false;
		for(std::size_t index = 0; index < 3; ++index)
			values[index] = parsed[index];
		return true;
	}

private:
	Stream &stream_;
};
}
