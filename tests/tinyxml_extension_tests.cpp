#include <cassert>

#include "tinyxml.h"

int main()
{
	TiXmlElement valid("color");
	valid.SetAttribute("rgb", "0 127 255");
	unsigned char red = 9;
	unsigned char green = 9;
	unsigned char blue = 9;
	assert(valid.QueryColorAttribute("rgb", &red, &green, &blue) == TIXML_SUCCESS);
	assert(red == 0 && green == 127 && blue == 255);

	TiXmlElement invalid("color");
	invalid.SetAttribute("rgb", "256 2 3");
	assert(invalid.QueryColorAttribute("rgb", &red, &green, &blue) == TIXML_WRONG_TYPE);
	assert(red == 0 && green == 127 && blue == 255);

	return 0;
}
