// ShmDcoc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <sstream>
#include <iostream>

int main1(int argc, char *argv[])
{
	using namespace boost::interprocess;
	if (argc == 1) {  //Parent process
					  //Remove shared memory on construction and destruction
		struct shm_remove
		{
			shm_remove() { shared_memory_object::remove("MySharedMemory"); }
			~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		} remover;

		//Create a managed shared memory segment
		managed_shared_memory segment(create_only, "MySharedMemory", 65536);

		//Allocate a portion of the segment (raw memory)
		managed_shared_memory::size_type free_memory = segment.get_free_memory();
		void * shptr = segment.allocate(1024/*bytes to allocate*/);

		//Check invariant
		if (free_memory <= segment.get_free_memory())
			return 1;

		//An handle from the base address can identify any byte of the shared
		//memory segment even if it is mapped in different base addresses
		managed_shared_memory::handle_t handle = segment.get_handle_from_address(shptr);
		std::stringstream s;
		s << argv[0] << " " << handle;
		s << std::ends;
		//Launch child process
		if (0 != std::system(s.str().c_str()))
			return 1;
		//Check memory has been freed
		if (free_memory != segment.get_free_memory())
			return 1;
	}
	else {
		//Open managed segment
		managed_shared_memory segment(open_only, "MySharedMemory");

		//An handle from the base address can identify any byte of the shared
		//memory segment even if it is mapped in different base addresses
		managed_shared_memory::handle_t handle = 0;

		//Obtain handle value
		std::stringstream s; s << argv[1]; s >> handle;

		//Get buffer local address from handle
		void *msg = segment.get_address_from_handle(handle);

		//Deallocate previously allocated memory
		segment.deallocate(msg);
	}
	std::cin.get();
	return 0;
}

#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <cstddef>
#include <cassert>
#include <utility>

int main2(int argc, char *argv[])
{
	using namespace boost::interprocess;
	typedef std::pair<double, int> MyType;

	if (argc == 1) {  //Parent process
					  //Remove shared memory on construction and destruction
		struct shm_remove
		{
			shm_remove() { shared_memory_object::remove("MySharedMemory"); }
			~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		} remover;

		//Construct managed shared memory
		managed_shared_memory segment(create_only, "MySharedMemory", 65536);

		//Create an object of MyType initialized to {0.0, 0}
		MyType *instance = segment.construct<MyType>
			("MyType instance")  //name of the object
			(0.0, 0);            //ctor first argument

								 //Create an array of 10 elements of MyType initialized to {0.0, 0}
		MyType *array = segment.construct<MyType>
			("MyType array")     //name of the object
			[10]                 //number of elements
		(0.0, 0);            //Same two ctor arguments for all objects

							 //Create an array of 3 elements of MyType initializing each one
							 //to a different value {0.0, 0}, {1.0, 1}, {2.0, 2}...
		float float_initializer[3] = { 0.0, 1.0, 2.0 };
		int   int_initializer[3] = { 0, 1, 2 };

		MyType *array_it = segment.construct_it<MyType>
			("MyType array from it")   //name of the object
			[3]                        //number of elements
		(&float_initializer[0]    //Iterator for the 1st ctor argument
			, &int_initializer[0]);    //Iterator for the 2nd ctor argument

									   //Launch child process
		std::string s(argv[0]); s += " child ";
		if (0 != std::system(s.c_str()))
			return 1;


		//Check child has destroyed all objects
		if (segment.find<MyType>("MyType array").first ||
			segment.find<MyType>("MyType instance").first ||
			segment.find<MyType>("MyType array from it").first)
			return 1;
	}
	else {
		//Open managed shared memory
		managed_shared_memory segment(open_only, "MySharedMemory");

		std::pair<MyType*, managed_shared_memory::size_type> res;

		//Find the array
		res = segment.find<MyType>("MyType array");
		//Length should be 10
		if (res.second != 10) return 1;

		//Find the object
		res = segment.find<MyType>("MyType instance");
		//Length should be 1
		if (res.second != 1) return 1;

		//Find the array constructed from iterators
		res = segment.find<MyType>("MyType array from it");
		//Length should be 3
		if (res.second != 3) return 1;

		//We're done, delete all the objects
		segment.destroy<MyType>("MyType array");
		segment.destroy<MyType>("MyType instance");
		segment.destroy<MyType>("MyType array from it");
	}
	return 0;
}


//Demo_List Manual
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/offset_ptr.hpp>

using namespace boost::interprocess;

//Shared memory linked list node
struct list_node
{
	offset_ptr<list_node> next;
	int                   value;
};

int main3()
{
	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Create shared memory
	managed_shared_memory segment(create_only,
		"MySharedMemory",  //segment name
		65536);

	//Create linked list with 10 nodes in shared memory
	offset_ptr<list_node> prev = 0, current, first;

	int i;
	for (i = 0; i < 10; ++i, prev = current) {
		current = static_cast<list_node*>(segment.allocate(sizeof(list_node)));
		current->value = i;
		current->next = 0;

		if (!prev)
			first = current;
		else
			prev->next = current;
	}

	//Communicate list to other processes
	//. . .
	//When done, destroy list
	for (current = first; current; /**/) {
		prev = current;
		current = current->next;
		segment.deallocate(prev.get());
	}
	return 0;
}


//Demo_Vector
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <string>
#include <cstdlib> //std::system

using namespace boost::interprocess;

//Define an STL compatible allocator of ints that allocates from the managed_shared_memory.
//This allocator will allow placing containers in the segment
typedef allocator<int, managed_shared_memory::segment_manager>  ShmemAllocator;

//Alias a vector that uses the previous STL-like allocator so that allocates
//its values from the segment
typedef vector<int, ShmemAllocator> MyVector;

//Main function. For parent process argc == 1, for child process argc == 2
int main4(int argc, char *argv[])
{
	if (argc == 1) { //Parent process
					 //Remove shared memory on construction and destruction
		struct shm_remove
		{
			shm_remove() { shared_memory_object::remove("MySharedMemory"); }
			~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		} remover;

		//Create a new segment with given name and size
		managed_shared_memory segment(create_only, "MySharedMemory", 65536);

		//Initialize shared memory STL-compatible allocator
		const ShmemAllocator alloc_inst(segment.get_segment_manager());

		//Construct a vector named "MyVector" in shared memory with argument alloc_inst
		MyVector *myvector = segment.construct<MyVector>("MyVector")(alloc_inst);

		for (int i = 0; i < 100; ++i)  //Insert data in the vector
			myvector->push_back(i);

		//Launch child process
		std::string s(argv[0]); s += " child ";
		if (0 != std::system(s.c_str()))
			return 1;

		//Check child has destroyed the vector
		if (segment.find<MyVector>("MyVector").first)
			return 1;
	}
	else { //Child process
		   //Open the managed segment
		managed_shared_memory segment(open_only, "MySharedMemory");

		//Find the vector using the c-string name
		MyVector *myvector = segment.find<MyVector>("MyVector").first;

		for (auto & ele : *myvector)
		{
			std::cout << ele << std::endl;
		}

		//Use vector in reverse order
		std::sort(myvector->rbegin(), myvector->rend());

		for (auto & ele : *myvector)
		{
			std::cout << ele << std::endl;
		}

		//When done, destroy the vector from the segment
		segment.destroy<MyVector>("MyVector");
	}

	return 0;
}


//Demo_Map
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>

int main5()
{
	using namespace boost::interprocess;

	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Shared memory front-end that is able to construct objects
	//associated with a c-string. Erase previous shared memory with the name
	//to be used and create the memory segment at the specified address and initialize resources
	managed_shared_memory segment
	(create_only
		, "MySharedMemory" //segment name
		, 65536);          //segment size in bytes

						   //Note that map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
						   //so the allocator must allocate that pair.
	typedef int    KeyType;
	typedef float  MappedType;
	typedef std::pair<const int, float> ValueType;

	//Alias an STL compatible allocator of for the map.
	//This allocator will allow to place containers
	//in managed shared memory segments
	typedef allocator<ValueType, managed_shared_memory::segment_manager>
		ShmemAllocator;

	//Alias a map of ints that uses the previous STL-like allocator.
	//Note that the third parameter argument is the ordering function
	//of the map, just like with std::map, used to compare the keys.
	typedef map<KeyType, MappedType, std::less<KeyType>, ShmemAllocator> MyMap;

	//Initialize the shared memory STL-compatible allocator
	ShmemAllocator alloc_inst(segment.get_segment_manager());

	//Construct a shared memory map.
	//Note that the first parameter is the comparison function,
	//and the second one the allocator.
	//This the same signature as std::map's constructor taking an allocator
	MyMap *mymap =
		segment.construct<MyMap>("MyMap")      //object name
		(std::less<int>() //first  ctor parameter
			, alloc_inst);     //second ctor parameter

							   //Insert data in the map
	for (int i = 0; i < 100; ++i) {
		mymap->insert(std::pair<const int, float>(i, (float)i));
	}

	for (auto& p : * mymap)
	{
		std::cout << "Key : " << p.first << " Value : " << p.second << std::endl;
	}

	std::cin.get();
	return 0;
}

//Demo_Map_String
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>

int main6()
{
	using namespace boost::interprocess;

	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Shared memory front-end that is able to construct objects
	//associated with a c-string. Erase previous shared memory with the name
	//to be used and create the memory segment at the specified address and initialize resources
	managed_shared_memory segment
	(create_only
		, "MySharedMemory" //segment name
		, 65536);          //segment size in bytes

						   //Note that map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
						   //so the allocator must allocate that pair.
	typedef int    KeyType;
	typedef std::string  MappedType;
	typedef std::pair<const int, std::string> ValueType;

	//Alias an STL compatible allocator of for the map.
	//This allocator will allow to place containers
	//in managed shared memory segments
	typedef allocator<ValueType, managed_shared_memory::segment_manager>
		ShmemAllocator;

	//Alias a map of ints that uses the previous STL-like allocator.
	//Note that the third parameter argument is the ordering function
	//of the map, just like with std::map, used to compare the keys.
	typedef map<KeyType, MappedType, std::less<KeyType>, ShmemAllocator> MyMap;

	//Initialize the shared memory STL-compatible allocator
	ShmemAllocator alloc_inst(segment.get_segment_manager());

	//Construct a shared memory map.
	//Note that the first parameter is the comparison function,
	//and the second one the allocator.
	//This the same signature as std::map's constructor taking an allocator
	MyMap *mymap =
		segment.construct<MyMap>("MyMap")      //object name
		(std::less<int>() //first  ctor parameter
			, alloc_inst);     //second ctor parameter

							   //Insert data in the map
	for (int i = 0; i < 100; ++i) {
		mymap->insert(std::pair<const int, std::string>(i, std::string("huzhigen")));
	}

	for (auto& p : *mymap)
	{
		std::cout << "Key : " << p.first << " Value : " << p.second << std::endl;
	}

	std::cin.get();
	return 0;
}

//Demo_Basic_string
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>

int main7()
{
	using namespace boost::interprocess;
	//Typedefs
	typedef allocator<char, managed_shared_memory::segment_manager>
		CharAllocator;
	typedef basic_string<char, std::char_traits<char>, CharAllocator>
		MyShmString;
	typedef allocator<MyShmString, managed_shared_memory::segment_manager>
		StringAllocator;
	typedef vector<MyShmString, StringAllocator>
		MyShmStringVector;

	//Open shared memory
	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	managed_shared_memory shm(create_only, "MySharedMemory", 10000);

	//Create allocators
	CharAllocator     charallocator(shm.get_segment_manager());
	StringAllocator   stringallocator(shm.get_segment_manager());

	//This string is in only in this process (the pointer pointing to the
	//buffer that will hold the text is not in shared memory).
	//But the buffer that will hold "this is my text" is allocated from
	//shared memory
	MyShmString mystring(charallocator);
	mystring = "this is my text";

	//This vector is only in this process (the pointer pointing to the
	//buffer that will hold the MyShmString-s is not in shared memory).
	//But the buffer that will hold 10 MyShmString-s is allocated from
	//shared memory using StringAllocator. Since strings use a shared
	//memory allocator (CharAllocator) the 10 buffers that hold
	//"this is my text" text are also in shared memory.
	MyShmStringVector myvector(stringallocator);
	myvector.insert(myvector.begin(), 10, mystring);

	//This vector is fully constructed in shared memory. All pointers
	//buffers are constructed in the same shared memory segment
	//This vector can be safely accessed from other processes.
	MyShmStringVector *myshmvector =
		shm.construct<MyShmStringVector>("myshmvector")(stringallocator);
	myshmvector->insert(myshmvector->begin(), 10, mystring);

	//Destroy vector. This will free all strings that the vector contains
	shm.destroy_ptr(myshmvector);
	return 0;
}


//Demo_Shm_Move
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <cassert>

int main8()
{
	using namespace boost::interprocess;

	//Typedefs
	typedef managed_shared_memory::segment_manager     SegmentManager;
	typedef allocator<char, SegmentManager>            CharAllocator;
	typedef basic_string<char, std::char_traits<char>
		, CharAllocator>                MyShmString;
	typedef allocator<MyShmString, SegmentManager>     StringAllocator;
	typedef vector<MyShmString, StringAllocator>       MyShmStringVector;

	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	managed_shared_memory shm(create_only, "MySharedMemory", 10000);

	//Create allocators
	CharAllocator     charallocator(shm.get_segment_manager());
	StringAllocator   stringallocator(shm.get_segment_manager());

	//Create a vector of strings in shared memory.
	MyShmStringVector *myshmvector =
		shm.construct<MyShmStringVector>("myshmvector")(stringallocator);

	//Insert 50 strings in shared memory. The strings will be allocated
	//only once and no string copy-constructor will be called when inserting
	//strings, leading to a great performance.
	MyShmString string_to_compare(charallocator);
	string_to_compare = "this is a long, long, long, long, long, long, string...";

	myshmvector->reserve(50);
	for (int i = 0; i < 50; ++i) {
		MyShmString move_me(string_to_compare);
		//In the following line, no string copy-constructor will be called.
		//"move_me"'s contents will be transferred to the string created in
		//the vector
		myshmvector->push_back(boost::move(move_me));

		//The source string is in default constructed state
		assert(move_me.empty());

		//The newly created string will be equal to the "move_me"'s old contents
		assert(myshmvector->back() == string_to_compare);
	}

	//Now erase a string...
	myshmvector->pop_back();

	//...And insert one in the first position.
	//No string copy-constructor or assignments will be called, but
	//move constructors and move-assignments. No memory allocation
	//function will be called in this operations!!
	myshmvector->insert(myshmvector->begin(), boost::move(string_to_compare));

	//Destroy vector. This will free all strings that the vector contains
	shm.destroy_ptr(myshmvector);

	std::cin.get();
	return 0;
}

//Demo_Containers of containers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>

int main9()
{
	using namespace boost::interprocess;

	//Typedefs of allocators and containers
	typedef managed_shared_memory::segment_manager                       segment_manager_t;
	typedef allocator<void, segment_manager_t>                           void_allocator;
	typedef allocator<int, segment_manager_t>                            int_allocator;
	typedef vector<int, int_allocator>                                   int_vector;
	typedef allocator<int_vector, segment_manager_t>                     int_vector_allocator;
	typedef vector<int_vector, int_vector_allocator>                     int_vector_vector;
	typedef allocator<char, segment_manager_t>                           char_allocator;
	typedef basic_string<char, std::char_traits<char>, char_allocator>   char_string;

	class complex_data
	{

	public:
		//Since void_allocator is convertible to any other allocator<T>, we can simplify
		//the initialization taking just one allocator for all inner containers.
		complex_data(int id, const char *name, const void_allocator &void_alloc)
			: id_(id), char_string_(name, void_alloc), int_vector_vector_(void_alloc)
		{}
		//Other members...
		int               id_;
		char_string       char_string_;
		int_vector_vector int_vector_vector_;
	};

	//Definition of the map holding a string as key and complex_data as mapped type
	typedef std::pair<const char_string, complex_data>                      map_value_type;
	typedef std::pair<char_string, complex_data>                            movable_to_map_value_type;
	typedef allocator<map_value_type, segment_manager_t>                    map_value_type_allocator;
	typedef map< char_string, complex_data
		, std::less<char_string>, map_value_type_allocator>          complex_map_type;


	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Create shared memory
	managed_shared_memory segment(create_only, "MySharedMemory", 65536);

	//An allocator convertible to any allocator<T, segment_manager_t> type
	void_allocator alloc_inst(segment.get_segment_manager());

	//Construct the shared memory map and fill it
	complex_map_type *mymap = segment.construct<complex_map_type>
		//(object name), (first ctor parameter, second ctor parameter)
		("MyMap")(std::less<char_string>(), alloc_inst);

	for (int i = 0; i < 100; ++i) {
		//Both key(string) and value(complex_data) need an allocator in their constructors
		char_string  key_object(alloc_inst);
		complex_data mapped_object(i, "default_name", alloc_inst);
		map_value_type value(key_object, mapped_object);
		//Modify values and insert them in the map
		mymap->insert(value);
	}

	for (auto & p : *mymap)
	{
		std::cout << "key : " << p.first << ", value : " << p.second.char_string_ << std::endl;
	}

	std::cin.get();
	return 0;
}


//Demo_Boost unordered containers
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>


#include <boost/unordered_map.hpp>     //boost::unordered_map


#include <functional>                  //std::equal_to
#include <boost/functional/hash.hpp>   //boost::hash


int main10()
{
	using namespace boost::interprocess;
	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Create shared memory
	managed_shared_memory segment(create_only, "MySharedMemory", 65536);

	//Note that unordered_map<Key, MappedType>'s value_type is std::pair<const Key, MappedType>,
	//so the allocator must allocate that pair.
	typedef int    KeyType;
	typedef float  MappedType;
	typedef std::pair<const int, float> ValueType;

	//Typedef the allocator
	typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;

	//Alias an unordered_map of ints that uses the previous STL-like allocator.
	typedef boost::unordered_map
		< KeyType, MappedType
		, boost::hash<KeyType>, std::equal_to<KeyType>
		, ShmemAllocator>
		MyHashMap;

	//Construct a shared memory hash map.
	//Note that the first parameter is the initial bucket count and
	//after that, the hash function, the equality function and the allocator
	MyHashMap *myhashmap = segment.construct<MyHashMap>("MyHashMap")  //object name
		(3, boost::hash<int>(), std::equal_to<int>()                  //
			, segment.get_allocator<ValueType>());                         //allocator instance

																		   //Insert data in the hash map
	for (int i = 0; i < 100; ++i) {
		myhashmap->insert(ValueType(i, (float)i));
	}
	return 0;
}

//Demo_Boost.MultiIndex containers

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/string.hpp>


#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>


using namespace boost::interprocess;
namespace bmi = boost::multi_index;

typedef managed_shared_memory::allocator<char>::type              char_allocator;
typedef basic_string<char, std::char_traits<char>, char_allocator>shm_string;

//Data to insert in shared memory
struct employee
{
	int         id;
	int         age;
	shm_string  name;
	employee(int id_
		, int age_
		, const char *name_
		, const char_allocator &a)
		: id(id_), age(age_), name(name_, a)
	{}
};

//Tags
struct id {};
struct age {};
struct name {};

// Define a multi_index_container of employees with following indices:
//   - a unique index sorted by employee::int,
//   - a non-unique index sorted by employee::name,
//   - a non-unique index sorted by employee::age.
typedef bmi::multi_index_container<
	employee,
	bmi::indexed_by<
	bmi::ordered_unique
	<bmi::tag<id>, bmi::member<employee, int, &employee::id> >,
	bmi::ordered_non_unique<
	bmi::tag<name>, bmi::member<employee, shm_string, &employee::name> >,
	bmi::ordered_non_unique
	<bmi::tag<age>, bmi::member<employee, int, &employee::age> > >,
	managed_shared_memory::allocator<employee>::type
> employee_set;

int main()
{
	//Remove shared memory on construction and destruction
	struct shm_remove
	{
		shm_remove() { shared_memory_object::remove("MySharedMemory"); }
		~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
	} remover;

	//Create shared memory
	managed_shared_memory segment(create_only, "MySharedMemory", 65536);

	//Construct the multi_index in shared memory
	employee_set *es = segment.construct<employee_set>
		("My MultiIndex Container")            //Container's name in shared memory
		(employee_set::ctor_args_list()
			, segment.get_allocator<employee>());  //Ctor parameters

												   //Now insert elements
	char_allocator ca(segment.get_allocator<char>());
	es->insert(employee(0, 31, "Joe", ca));
	es->insert(employee(1, 27, "Robert", ca));
	es->insert(employee(2, 40, "John", ca));
	return 0;
}