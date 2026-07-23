#include "DS_List.h"

#include <cassert>

int main()
{
	DataStructures::List<int> values;
	values.Insert(42);
	values.Insert(7, 0);

	assert(values.Size() == 2);
	assert(values[0] == 7);
	assert(values[1] == 42);

	values.RemoveAtIndex(0);
	assert(values.Size() == 1);
	assert(values[0] == 42);
	return 0;
}
