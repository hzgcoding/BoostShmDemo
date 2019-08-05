// ShmGrow.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/managed_mapped_file.hpp>
#include <cassert>

class MyClass
{
	//...
};

int main1()
{
	using namespace boost::interprocess;
	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	{
		//Create a managed shared memory
		managed_shared_memory shm(create_only, "MySharedMemory", 1000);

		//Check size
		assert(shm.get_size() == 1000);
		//Construct a named object
		MyClass *myclass = shm.construct<MyClass>("MyClass")();
		//The managed segment is unmapped here
	}
	{
		//Now that the segment is not mapped grow it adding extra 500 bytes
		managed_shared_memory::grow("MySharedMemory", 500);

		//Map it again
		managed_shared_memory shm(open_only, "MySharedMemory");
		//Check size
		assert(shm.get_size() == 1500);
		//Check "MyClass" is still there
		MyClass *myclass = shm.find<MyClass>("MyClass").first;
		assert(myclass != 0);
		//The managed segment is unmapped here
	}
	{
		//Now minimize the size of the segment
		managed_shared_memory::shrink_to_fit("MySharedMemory");

		//Map it again
		managed_shared_memory shm(open_only, "MySharedMemory");
		//Check size
		assert(shm.get_size() < 1000);
		//Check "MyClass" is still there
		MyClass *myclass = shm.find<MyClass>("MyClass").first;
		assert(myclass != 0);
		//The managed segment is unmapped here
	}
	return 0;
}

#include <boost/interprocess/managed_shared_memory.hpp>
#include <cassert>

int main()
{
	using namespace boost::interprocess;

	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Managed memory segment that allocates portions of a shared memory
	//segment with the default management algorithm
	managed_shared_memory managed_shm(create_only, "MySharedMemory", 65536);

	const std::size_t Alignment = 128;

	//Allocate 100 bytes aligned to Alignment from segment, throwing version
	void *ptr = managed_shm.allocate_aligned(100, Alignment);

	//Check alignment
	assert((static_cast<char*>(ptr) - static_cast<char*>(0)) % Alignment == 0);

	//Deallocate it
	managed_shm.deallocate(ptr);

	//Non throwing version
	ptr = managed_shm.allocate_aligned(100, Alignment, std::nothrow);

	//Check alignment
	assert((static_cast<char*>(ptr) - static_cast<char*>(0)) % Alignment == 0);

	//Deallocate it
	managed_shm.deallocate(ptr);

	//If we want to efficiently allocate aligned blocks of memory
	//use managed_shared_memory::PayloadPerAllocation value
	assert(Alignment > managed_shared_memory::PayloadPerAllocation);

	//This allocation will maximize the size of the aligned memory
	//and will increase the possibility of finding more aligned memory
	ptr = managed_shm.allocate_aligned
	(3 * Alignment - managed_shared_memory::PayloadPerAllocation, Alignment);

	//Check alignment
	assert((static_cast<char*>(ptr) - static_cast<char*>(0)) % Alignment == 0);

	//Deallocate it
	managed_shm.deallocate(ptr);

	return 0;
}