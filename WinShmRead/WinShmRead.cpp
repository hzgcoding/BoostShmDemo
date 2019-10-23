// WinShmRead.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>  
#include <windows.h>  
using namespace std;

#define BUF_SIZE 4096

int main()
{
	// 打开共享的文件对象
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, L"ShareMemory");
	if (hMapFile)
	{
		LPVOID lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		// 将共享内存数据拷贝出来
		char szBuffer[BUF_SIZE] = { 0 };
		strcpy(szBuffer, (char*)lpBase);
		printf("%s", szBuffer);

		// 解除文件映射
		UnmapViewOfFile(lpBase);
		// 关闭内存映射文件对象句柄
		CloseHandle(hMapFile);
	}
	else
	{
		// 打开共享内存句柄失败
		printf("OpenMapping Error");
	}
	std::cin.get();
	return 0;
}


