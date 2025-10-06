#include "test/Test.h"
#include <vector>
#include <iostream>
#include <mema\Alloc_typedef.h>

void tst::TestAllocatorsWithVector()
{
	std::cout << "\n\n=====Testing Using Vectors=====\n";
	
	// Vector and System Allocator
	std::vector<int, mema::SystemAllocatorSTL<int>> vec_sys;
	vec_sys.push_back(10);
	vec_sys.push_back(20);

	// Vector and Small Object Allocator
	std::vector<int, mema::SoaAllocatorSTL<int>> vec_soa;

	for (int i = 0; i < 260; ++i)
	{
		vec_soa.push_back(i);
	}
}
