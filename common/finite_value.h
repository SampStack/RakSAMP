#pragma once

#if defined(_MSC_VER)
#include <float.h>
#else
#include <cmath>
#endif

namespace raksamp::numeric
{
inline bool IsFinite(float value)
{
#if defined(_MSC_VER)
	return _finite(value) != 0;
#else
	return std::isfinite(value);
#endif
}
}
