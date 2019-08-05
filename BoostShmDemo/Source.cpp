
#include <boost/interprocess/managed_shared_memory.hpp>
#include <cstdlib> //std::system
#include <sstream>
#include <cstddef>
#include <cassert>
#include <utility>
#include <iostream>
#include "CIPCMgr.h"

//如果在共享内存中存放复杂结构，在map、set为key，需要实现小于操作作符
//对于key是自定义的类型，map的key需要定义operator<，内置类型(如,int)是自带operator<;
//对于key是自定义的类型，unordered_map需要定义hash_value函数并且重载operator==，内置类型都自带该功能。
class ShmTestData
{
public:
    int               id_;
    BoostString       strBst_;

public:
    //因为void_allocator能够转换为任何类型的allocator<T>, 所以我们能够简单的在构造函数中使用void_allocator来初始化所有的内部容器
    ShmTestData(int id, const char *name, const VoidAlloc &void_alloc)
        : id_(id), strBst_(name, void_alloc)
    {}
    ~ShmTestData() {}

    bool operator <(const ShmTestData& rh) const
    {
        return this->id_ < rh.id_;
    }
};

struct CommonData
{
public:
    int               id_;
    std::string       string_;
public:
    CommonData(int id, const char *name)
        : id_(id), string_(name)
    {}
    ~CommonData() {}

    bool operator <(const CommonData& rh) const
    {
        return this->id_ < rh.id_;
    }
};


//demo1 共享内存server端 测试一些基本数据
int Demo_Common(int argc, char *argv[])
{
    using namespace std;
    using namespace boost::interprocess;
    typedef std::pair<double, int> MyType;
    
    std::string strShm = "MySharedMemory";
    std::string strShmObj = "MyType instance";
    std::string strShmObjArr = "MyType array";
    std::string strShmObjArrIt = "MyType array from it";

    //Parent process
      //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
    } remover;

    //Construct managed shared memory
    managed_shared_memory segment(create_only, "MySharedMemory", 65536);

    //Create an object of MyType initialized to {0.0, 0}
    MyType *instance = segment.construct<MyType>(strShmObj.data())(2.5, 5);            //ctor first argument
    cout << "construct MyType instance" << endl;
    
    //Create an array of 10 elements of MyType initialized to {0.0, 0}
    //name of the object //number of elements //Same two ctor arguments for all objects
    MyType *array = segment.construct<MyType>("MyType array")[10](3.3, 10);            
    cout << "construct MyType array" << endl;

    
    //create an array of 10 elements
    MyType *arrTiger = segment.construct<MyType>("Tiger array")[10]();
    for (int i = 0; i < 10; ++i)
    {
        arrTiger[i] = std::make_pair(i * 10, i * 11);
    }
    cout << "construct MyType Tiger array" << endl;
    
    //Create an array of 3 elements of MyType initializing each one
    //to a different value {0.0, 0}, {1.0, 1}, {2.0, 2}...
    float float_initializer[3] = { 0.0, 1.0, 2.0 };
    int   int_initializer[3] = { 0, 1, 2 };

    MyType *array_it = segment.construct_it<MyType>
        ("MyType array from it")   //name of the object
        [3]                        //number of elements
    (&float_initializer[0]    //Iterator for the 1st ctor argument
        , &int_initializer[0]);    //Iterator for the 2nd ctor argument

    cout << "construct MyType from it" << endl;

    cin.get();
    //find object
    auto res = segment.find<MyType>(strShmObj.data());
    MyType* pMyType = res.first;
    if ( pMyType != nullptr)
    {
        cout << "After modified " << strShmObj;
        cout << "First : " << pMyType->first << endl;
        cout << "Second : " << pMyType->second << endl;
    }

    cin.get();

    //Check child has destroyed all objects
    if (segment.find<MyType>("MyType array").first ||
        segment.find<MyType>("MyType instance").first ||
        segment.find<MyType>("MyType array from it").first)
        return 1;

    return 0;
}


//demo2  使用自定义的map
//a bad example that cannot read by another process
// find answer now : because server and client also use std::map, but in fact, boost::interprocess::map is correct one.
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/string.hpp>

int Demo_TwoMap()
{
    using namespace boost::interprocess;

    typedef std::pair<const BoostString, BoostString> TPairStr2Str;

    typedef BoostMap<BoostString, BoostString>  MapStr2Str;

    /*-- 自定义map<string, map<string, string>> --*/
    typedef std::pair<const BoostString, MapStr2Str> TPairStr2StrMap;
    //typedef map<BoostString, TPairStr2StrMap>  MapStr2StrMap;
    typedef BoostMap<BoostString, MapStr2Str>  MapStr2StrMap;


    using namespace std;
    using namespace boost::interprocess;
    typedef std::pair<double, int> MyType;

    std::string strShm = "MySharedMemory";
    std::string strShmObj = "MyType instance";
    std::string strShmObjArr = "MyType array";
    std::string strShmObjArrIt = "MyType array from it";

    //Parent process
    //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
    } remover;

    managed_shared_memory oShm(open_or_create, "MySharedMemory", 102400);
    VoidAlloc vdAlloc(oShm.get_segment_manager());

    ////在共享内存上构建string_string_string_map，并返回指针
    MapStr2StrMap *pFatherMap = oShm.construct<MapStr2StrMap>("father_map")(std::less<BoostString>(), vdAlloc);

    //构建自定义string
    BoostString  strKeyKey("IAmKeyKey", vdAlloc);
    BoostString  strKey("IAmKey", vdAlloc);
    BoostString  strValue("IAmValue", vdAlloc);

    //在共享内存上构建string_string_map并返回指针
    MapStr2Str *pSonMap = oShm.construct<MapStr2Str>("son_map")(std::less<BoostString>(), vdAlloc);
    //构建自定义map<string, string>的键值对，不能够使用std::string
    TPairStr2Str  oSonPair(strKey, strValue);
    pSonMap->insert(oSonPair);

    //构建自定义map<string, map<string, string>>的键值对
    TPairStr2StrMap oFatherPair(strKeyKey, *pSonMap);
    pFatherMap->insert(oFatherPair);


    /* ---读取共享内存，并解析map--- */
    managed_shared_memory oShmRead(open_or_create, "MySharedMemory", 102400);
    VoidAlloc alloc_inst1(oShmRead.get_segment_manager());
    

    MapStr2StrMap *pFMap = oShmRead.find<MapStr2StrMap>("father_map").first;

    //创建对应迭代器
    MapStr2StrMap::iterator sss_ite;
    MapStr2Str::iterator ss_ite;

    //从map中遍历数据，与stl的map一致
    sss_ite = pFMap->find(strKeyKey);
    if (sss_ite != pFMap->end())
    {
        cout << sss_ite->first << endl;
        ss_ite = sss_ite->second.find(strKey);
        cout << ss_ite->second << endl;
    }

    cin.get();

    return 0;
}



//a demo that to create map
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <functional>
#include <utility>

int Demo_Map()
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
    typedef allocator<ValueType, managed_shared_memory::segment_manager> ShmemAllocator;

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
    for (int i = 0; i < 100; ++i)
    {
        mymap->insert(std::pair<const int, float>(i, (float)i));
    }

    std::cin.get();
    return 0;
}



//demo4 : create a complicated shm map 

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <iostream>
#include <chrono>   
#include <string>   
#include <vector>   

int Demo_MapVec()
{
    using namespace std::chrono;
    using namespace boost::interprocess;

    typedef BoostMap<BoostString, ShmTestData> TShmMap;

    //在程序开始和结束的时候移除同名的共享内存
    //如果只是读其他程序创建的共享内存块则不该包含remover
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
    } remover;
    //创建共享内存，根据测试，在32位的linux上，单块命名的共享内存大小最大为2^31-1，如果是读其他程序创建的共享内存，则此句应写为
    //managed_shared_memory segment(open_only, "MySharedMemory");

    CTimeMeter oMeter("CreateShm");
    const int SHM_SIZE = 1024 * 1024 * 1024; //1G
    managed_shared_memory segment(create_only, "MySharedMemory", SHM_SIZE);
    oMeter.End();


    oMeter.Reset("Shm CURD");
    //一个能够转换为任何allocator<T, segment_manager_t>类型的allocator 
    VoidAlloc vdAlloc(segment.get_segment_manager());

    //在共享内存上创建map
    //如果map已由其他程序创建，或者不确定map是否已创建，应如下：
    TShmMap *pShmMap = segment.construct<TShmMap>("MyMap")(std::less<BoostString>(), vdAlloc);
    int nCount = 100000;
    for (int i = 0; i < nCount; ++i)
    {
        //key(string) 和value(ShmTestData) 都需要在他们的构造函数中包含一个allocator
        char tmp[16] = "";
        sprintf(tmp, "test%d", i);
        BoostString  keyStr(tmp, vdAlloc);
        ShmTestData  shmData(i, "default_name", vdAlloc);
        std::pair<const BoostString, ShmTestData> oValue(keyStr, shmData);
        //向map中插值
        pShmMap->insert(oValue);
    }
    oMeter.End();

    oMeter.Reset("Common CURD");
    std::map<std::string, CommonData> commonMap;
    for (int i = 0; i < nCount; ++i)
    {
        char tmp[16] = "";
        sprintf(tmp, "test%d", i);
        ::std::string   keyStr(tmp);
        CommonData      oData(i, "default_name");
        std::pair<std::string, CommonData> value(keyStr, oData);
        //向map中插值
        commonMap.insert(value);
    }
    oMeter.End();

    std::cout << "shm vector--------------------------------------------" << std::endl;
    try
    {
        typedef BoostVector<ShmTestData> TShmVector;
        oMeter.Reset("Shm vector");
        TShmVector* pCpxVector = segment.construct<TShmVector>("MyVector")(vdAlloc);
        if (pCpxVector == nullptr)
        {
            std::cout << "Fail to " << std::endl;
            return 0;
        }

        for (int i = 0; i < nCount; ++i)
        {
            //包含一个allocator
            ShmTestData obj(i, "default_name", vdAlloc);
            pCpxVector->push_back(obj);
        }

        oMeter.End();
    }
    catch (std::exception &e)
    {
        std::cout << e.what() << std::endl;
    }

    std::cout << "shm set--------------------------------------------" << std::endl;
    std::cin.get();
    return 0;
}



//demo: set example
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <functional>
#include <utility>
#include <set>
int Demo_Set()
{
    using namespace boost::interprocess;

    //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
    } remover;

    //Create shared memory object
    managed_shared_memory oShm(create_only, "MySharedMemory", 6553600);

    //Initialize the shared memory STL-compatible allocator
    //VoidAlloc  vdAlloc(oShm.get_segment_manager());
    BoostAlloc<void>  vdAlloc(oShm.get_segment_manager());

    typedef BoostSet<ShmTestData>    TCpxSet;
    //Construct another complex set 
    TCpxSet *pCpxSet = oShm.construct<TCpxSet>("CpxSet")(std::less<ShmTestData>(), vdAlloc);
    if ( pCpxSet!= nullptr)
    {
        CTimeMeter oMeter("MySet");
        for (int i = 0; i < 100000; ++i)
        {
            ShmTestData    data(i, "Tiger", vdAlloc);
            pCpxSet->insert(data);
        }
        oMeter.End();

        //cpxSet *pDstSet = shm.find<cpxSet>("CpxSet").first;
        //if ( pDstSet != nullptr)
        //{
        //    for (auto& elem : *pDstSet)
        //    {
        //        std::cout << elem.id_ << "," << elem.char_string_ << std::endl;
        //    }
        //}
    }

    if (1)
    {

        std::set<CommonData> mySet;
        CTimeMeter oMeter("std set");
        for (int i = 0; i < 100000; ++i)
        {
            CommonData data(i, "SETTIger0");
            mySet.insert(data);
        }
        oMeter.End();
    }

    std::cout << "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY" << std::endl;
    std::cin.get();
    return 0;
}



//demo: list example
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <functional>
#include <utility>
#include <set>
#include <list>

int Demo_List()
{
    using namespace boost::interprocess;

    //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
    } remover;

    //Create shared memory object
    managed_shared_memory shm(create_only, "MySharedMemory", 6553600);

    //Initialize the shared memory STL-compatible allocator
    BoostAlloc<void>  voidAlloc(shm.get_segment_manager());
    typedef BoostList<ShmTestData>  MyBoostList;
    MyBoostList *pList = shm.construct<MyBoostList>("myboostlist")(voidAlloc);
    if ( pList != nullptr )
    {
        CTimeMeter oMeter("MyBoostList");
        for (int i = 0; i < 100000; ++i)
        {
            ShmTestData    data(i, "Tiger", voidAlloc);
            pList->push_front(data);
            //pHmap->push_back(data);
        }
        oMeter.End();
    }

    if (1)
    {
        ::std::list<CommonData> myList;
        CTimeMeter oMeter("std list");
        for (int i = 0; i < 100000; ++i)
        {
            CommonData data(i, "SETTIger0");
            myList.push_front(data);
        }
        oMeter.End();
    }

    std::cout << "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY" << std::endl;
    std::cin.get();
    return 0;
}

int Demo_UnorderMap()
{
    using namespace boost::interprocess;
    //Remove shared memory on construction and destruction
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove("MySharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("MySharedMemory"); }
    } remover;

    //Create shared memory object
    managed_shared_memory shm(create_only, "MySharedMemory", 6553600);

    //Initialize the shared memory STL-compatible allocator
    BoostAlloc<void>  vdAlloc(shm.get_segment_manager());
    typedef BoostHashMap<BoostString, ShmTestData>  MyBoostHashMap;
    MyBoostHashMap *pHmap = shm.construct<MyBoostHashMap>("myboost_hashmap")(vdAlloc);
    if (pHmap != nullptr)
    {
        CTimeMeter oMeter("MyBoostHashMap");
        for (int i = 0; i < 100000; ++i)
        {
            char tmp[16] = "";
            sprintf(tmp, "test%d", i);
            BoostString    strKey(tmp, vdAlloc);
            ShmTestData    shmData(i, "Tiger", vdAlloc);
            std::pair<const BoostString, ShmTestData> oValue(strKey, shmData);
            pHmap->insert(oValue);
        }
        oMeter.End();

        oMeter.Reset("Find Key");
        BoostString strFind("test999", vdAlloc);
        auto it = pHmap->find(strFind);
        if (it != pHmap->end())
        {
            ShmTestData& oData = it->second;
            std::cout << "Find shm data : " << oData.id_ << ", " << oData.strBst_ << std::endl;
        }
        oMeter.End();
    }
    //if (1)
    //{
    //    ::std::unordered_map<CommonData> myList;
    //    CTimeMeter oMeter("std list");
    //    for (int i = 0; i < 100000; ++i)
    //    {
    //        CommonData data(i, "SETTIger0");
    //        myList.push_front(data);
    //    }
    //    oMeter.End();
    //}

    std::cout << "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY" << std::endl;
    std::cin.get();
    return 0;

}


int main()
{   
    using namespace std;
    Demo_UnorderMap();
    //Demo_TwoMap();
    //Demo_MapVec();
    cin.get();
    return 0;
}
