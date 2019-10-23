// WinShmWrite.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "stdafx.h"
#include <windows.h>
#include <iostream>
using namespace std;

#define BUF_SIZE 4096

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	// 定义共享数据
	char szBuffer[] = "____ 世界上最好的语言?";

	// 创建共享文件句柄 
	HANDLE hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,   // 物理文件句柄
		NULL,   // 默认安全级别
		PAGE_READWRITE,   // 可读可写
		0,   // 高位文件大小
		BUF_SIZE,   // 地位文件大小
		L"ShareMemory"   // 共享内存名称
	);

	// 映射缓存区视图 , 得到指向共享内存的指针
	LPVOID lpBase = MapViewOfFile(
		hMapFile,            // 共享内存的句柄
		FILE_MAP_ALL_ACCESS, // 可读写许可
		0,
		0,
		BUF_SIZE
	);

	// 将数据拷贝到共享内存
	strcpy((char*)lpBase, szBuffer);
	std::cout << "Write Finish, waiting ..." << std::endl;
	// 线程挂起等其他线程读取数据
	Sleep(20000);

	// 解除文件映射
	UnmapViewOfFile(lpBase);
	// 关闭内存映射文件对象句柄
	CloseHandle(hMapFile);
	std::cin.get();
	return 0;
}

