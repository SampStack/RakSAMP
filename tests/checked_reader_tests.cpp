#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "checked_reader.h"

namespace
{
class FakeStream
{
public:
	explicit FakeStream(std::vector<unsigned char> bytes) : bytes_(std::move(bytes)) {}

	template <typename Value>
	bool Read(Value &value)
	{
		return Read(reinterpret_cast<char *>(&value), sizeof(value));
	}

	bool Read(char *destination, int length)
	{
		if(length < 0 || offset_ + static_cast<std::size_t>(length) > bytes_.size())
			return false;
		std::memcpy(destination, bytes_.data() + offset_, length);
		offset_ += static_cast<std::size_t>(length);
		return true;
	}

private:
	std::vector<unsigned char> bytes_;
	std::size_t offset_ = 0;
};
}

int main()
{
	FakeStream valid({3, 'a', 'b', 'c'});
	raksamp::protocol::CheckedReader<FakeStream> validReader(valid);
	std::string text;
	assert(validReader.String8(text, 3));
	assert(text == "abc");

	FakeStream oversized({4, 'a', 'b', 'c', 'd'});
	raksamp::protocol::CheckedReader<FakeStream> oversizedReader(oversized);
	assert(!oversizedReader.String8(text, 3));

	FakeStream truncated({3, 'a'});
	raksamp::protocol::CheckedReader<FakeStream> truncatedReader(truncated);
	assert(!truncatedReader.String8(text, 3));

	return 0;
}
