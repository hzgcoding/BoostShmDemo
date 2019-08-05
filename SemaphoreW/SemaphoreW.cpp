// SemaphoreW.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/interprocess/sync/interprocess_semaphore.hpp>

struct shared_memory_buffer
{
	enum { NumItems = 10 };

	shared_memory_buffer()
		: mutex(1), nempty(NumItems), nstored(0)
	{}

	//Semaphores to protect and synchronize access
	boost::interprocess::interprocess_semaphore
		mutex, nempty, nstored;

	//Items to fill
	int items[NumItems];
};

#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <iostream>

using namespace boost::interprocess;

int main()
{
	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory1"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory1"); }
	} remover;

	//Create a shared memory object.
	shared_memory_object shm
	(create_only                  //only create
		, "MySharedMemory1"              //name
		, read_write  //read-write mode
	);

	//Set size
	shm.truncate(sizeof(shared_memory_buffer));

	//Map the whole shared memory in this process
	mapped_region region
	(shm                       //What to map
		, read_write //Map it as read-write
	);

	//Get the address of the mapped region
	void * addr = region.get_address();

	//Construct the shared structure in memory
	shared_memory_buffer * data = new (addr) shared_memory_buffer;

	const int NumMsg = 100;

	//Insert data in the array
	for (int i = 0; i < NumMsg; ++i) {
		data->nempty.wait();
		data->mutex.wait();
		data->items[i % shared_memory_buffer::NumItems] = i;
		data->mutex.post();
		data->nstored.post();
	}

	return 0;
}