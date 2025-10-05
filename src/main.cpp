#include "bmk\Benchmark.h"
#include "mema\Alloc_typedef.h"
#include "mema\CtmSOABackend.h"

#include <iostream>

int main()
{
	constexpr std::size_t numOps = 4000000;

	bmk::BmkAllocator<SystemBackend> sysAlloc;
	bmk::BmkAllocator<mema::SOABackend> soaAlloc;
	bmk::BmkAllocator<mema::CtmSOABackend> ctmAlloc;

	bmk::Benchmark bench(numOps);

	std::size_t size = 16;

	std::cout << "=====SYSTEM ALLOCATOR=====";
	//bench.BenchBulk(sysAlloc, size);
	//bench.BenchSameOrder(sysAlloc, size);
	//bench.BenchReverseOrder(sysAlloc, size);
	bench.BenchButterfly(sysAlloc, size);

	std::cout << "\n\n=====SMALL OBJ ALLOCATOR=====";
	//bench.BenchBulk(soaAlloc, size);
	//bench.BenchSameOrder(soaAlloc, size);
	//bench.BenchReverseOrder(soaAlloc, size);
	bench.BenchButterfly(soaAlloc, size);

	std::cout << "\n\n=====CUSTOM SMALL OBJ ALLOCATOR=====";
	//bench.BenchBulk(ctmAlloc, size);
	//bench.BenchSameOrder(ctmAlloc, size);
	//bench.BenchReverseOrder(ctmAlloc, size);
	bench.BenchButterfly(ctmAlloc, size);



	//std::cout << "\n\n=====Testing Using Vectors=====\n";
	//// std::vector con SystemAllocator
	//std::vector<int, mema::SystemAllocatorSTL<int>> vec_sys;
	//vec_sys.push_back(10);
	//vec_sys.push_back(20);

	//// std::vector con SOA allocator
	//std::vector<int, mema::SoaAllocatorSTL<int>> vec_soa;

	//for (int i = 0; i < 260; ++i)
	//{
	//	vec_soa.push_back(i);
	//}

	return 0;
}