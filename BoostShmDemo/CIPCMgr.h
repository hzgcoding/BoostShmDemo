#pragma once

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/containers/set.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>     //boost::unordered_map
#include <boost/functional/hash.hpp>   //boost::hash
#include <functional>                  //std::equal_to
#include <iostream>
#include <unordered_map>

namespace ipc = boost::interprocess;
namespace bctn = boost::container;
using ManShm = ipc::managed_shared_memory;
using SegmentMgr = ManShm::segment_manager;
template <typename T> using BoostAlloc = ipc::allocator<T, SegmentMgr>;
template <typename K> using BoostList = bctn::list<K, BoostAlloc<K>>;
template <typename K> using BoostVector = bctn::vector<K, BoostAlloc<K>>;
template <typename K> using BoostSet = bctn::set<K, std::less<K>, BoostAlloc<K>>;
template <typename K, typename V> using BoostMap = bctn::map<K, V, std::less<K>, BoostAlloc<std::pair<const K, V>> >;
template <typename K, typename V> using BoostHashMap = boost::unordered_map<K, V, boost::hash<K>, std::equal_to<K>, BoostAlloc<std::pair<K, V>> >;

typedef BoostAlloc<void>        VoidAlloc;
typedef BoostAlloc<int>         IntAlloc;
typedef BoostAlloc<char>        CharAlloc;
typedef BoostAlloc<double>      DoubleAlloc;
typedef bctn::basic_string<char, std::char_traits<char>, BoostAlloc<char> > BoostString;


class CIPCMgr
{
public:
    CIPCMgr();
    ~CIPCMgr();

public:

    bool Init(const int nAllocSize);
    
    template <typename TDataObj, typename TAllocator>
    bool Construct(const std::string strObjName, TAllocator alloc)
    {

        return false;
    }


    bool Destory();
private:
    bool    m_bEnable = true;
};

#include <iostream>
#include <chrono>

using namespace std::chrono;
class CTimeMeter
{
public:
    CTimeMeter(const std::string strInfo)
    {
        Reset(strInfo);
    }

    ~CTimeMeter()
    {}
    void Begin()
    {
        m_start = std::chrono::system_clock::now();
    }

    void Reset(const std::string strInfo)
    {
        m_strInfo = strInfo;
        m_start = std::chrono::system_clock::now();
    }

    void End()
    {
        m_end = system_clock::now();
        auto duration = duration_cast<microseconds>(m_end - m_start);
        std::cout << m_strInfo << " takes " << double(duration.count()) * microseconds::period::num / microseconds::period::den << " seconds" << std::endl;
    }
public:
    std::string               m_strInfo;
    time_point<system_clock>  m_start;
    time_point<system_clock>  m_end;
};


