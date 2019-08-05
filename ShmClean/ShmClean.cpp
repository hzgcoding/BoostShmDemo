// ShmClean.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <boost/interprocess/shared_memory_object.hpp>


int main()
{
	using namespace boost::interprocess;
	shared_memory_object::remove("MySharedMemory");
	return 0;
}
