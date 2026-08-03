#include <cassert>
#include <cmath>
#include <string>

#define max(a, b) windows_max_macro(a, b)
#include "safe_parse.h"
#undef max

int main()
{
	char small[5] = {};
	assert(raksamp::parse::Copy(small, "test"));
	assert(std::string(small) == "test");
	assert(!raksamp::parse::Copy(small, "large"));
	assert(!raksamp::parse::Copy(small, nullptr));

	int integer = 0;
	assert(raksamp::parse::IntegerValue("42", integer, 0, 100));
	assert(integer == 42);
	assert(!raksamp::parse::IntegerValue("42x", integer, 0, 100));
	assert(!raksamp::parse::IntegerValue("-1", integer, 0, 100));
	assert(!raksamp::parse::IntegerValue("101", integer, 0, 100));

	float decimal = 0.0f;
	assert(raksamp::parse::FloatValue("1.25", decimal, -2.0f, 2.0f));
	assert(decimal == 1.25f);
	assert(!raksamp::parse::FloatValue("nan", decimal));
	assert(!raksamp::parse::FloatValue("inf", decimal));

	std::string host;
	unsigned short port = 0;
	assert(raksamp::parse::HostAndPort("127.0.0.1:7777", host, port));
	assert(host == "127.0.0.1");
	assert(port == 7777);
	assert(!raksamp::parse::HostAndPort("127.0.0.1", host, port));
	assert(!raksamp::parse::HostAndPort(":7777", host, port));
	assert(!raksamp::parse::HostAndPort("host:65536", host, port));

	return 0;
}
