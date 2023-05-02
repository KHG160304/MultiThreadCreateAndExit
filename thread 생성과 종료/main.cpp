#include <process.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "winmm.lib")

#define	WAIT_EXIT_EVENT	WAIT_OBJECT_0

#define TOTAL_THREAD_CNT	5
#define	EXIT_AFTER_N_SEC	20

int g_Data = 0;
int g_Connect = 0;

HANDLE hAcceptThread;
HANDLE hDisconnectThread;
HANDLE hUpdateThread1;
HANDLE hUpdateThread2;
HANDLE hUpdateThread3;

HANDLE hThreadArr[TOTAL_THREAD_CNT];

HANDLE hEventExitThread;
/*
	초기화 함수
*/
bool Init();
/*
	프로세스 마지막에 정리하는 함수
*/
void CleanUp();
/*
	100 ~ 1000ms 마다 랜덤하게 g_Connect 를 + 1
*/
unsigned _stdcall AcceptThread(void* args);
/*
	100 ~ 1000ms 마다 랜덤하게 g_Connect 를 - 1
*/
unsigned _stdcall DisconnectThread(void* args);
/*
	10ms 마다 g_Data 를 + 1

		그리고 g_Data 가 1000 단위가 될 때 마다 이를 출력
*/
unsigned _stdcall UpdateThread(void* args);

void UpdateMainThread()
{
	static DWORD startTime = timeGetTime();
	DWORD countPerSec = 0;
	DWORD result;
	DWORD waitTime = 1000;
	for (;;)
	{
		result = WaitForMultipleObjects(TOTAL_THREAD_CNT, hThreadArr, true, waitTime);
		switch (result)
		{
		case WAIT_TIMEOUT:
			countPerSec += 1;
			printf("[%d sec]g_Connect: %d\n", countPerSec, g_Connect);
			if (countPerSec == EXIT_AFTER_N_SEC)
			{
				waitTime = INFINITE;
				SetEvent(hEventExitThread);
			}
			continue;
		case WAIT_FAILED:
			printf("%d", GetLastError());
		}

		break;
	}

	printf("스레드 종료 완료\n");
}

int main(void)
{
	if (!Init())
	{
		return -1;
	}

	UpdateMainThread();
	CleanUp();
	return 0;
}

bool Init()
{
	timeBeginPeriod(1);

	hEventExitThread = CreateEvent(nullptr, true, false, L"ExitThread");
	if (hEventExitThread == nullptr)
	{
		printf("Exit 이벤트객체 생성 실패: %d", GetLastError());		
		return false;
	}
	
	hAcceptThread = (HANDLE)_beginthreadex(nullptr, 0, AcceptThread, nullptr, 0, nullptr);
	if (hAcceptThread == nullptr)
	{
		printf("Accept 스레드 생성 실패: %d", GetLastError());
		return false;
	}
	hDisconnectThread = (HANDLE)_beginthreadex(nullptr, 0, DisconnectThread, nullptr, 0, nullptr);
	if (hDisconnectThread == nullptr)
	{
		printf("Disconnect 스레드 생성 실패: %d", GetLastError());
		return false;
	}

	hUpdateThread1 = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, (void*)1, 0, nullptr);
	if (hUpdateThread1 == nullptr)
	{
		printf("Update 스레드1 생성 실패: %d", GetLastError());
		return false;
	}
	hUpdateThread2 = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, (void*)2, 0, nullptr);
	if (hUpdateThread2 == nullptr)
	{
		printf("Update 스레드2 생성 실패: %d", GetLastError());
		return false;
	}
	hUpdateThread3 = (HANDLE)_beginthreadex(nullptr, 0, UpdateThread, (void*)3, 0, nullptr);
	if (hUpdateThread3 == nullptr)
	{
		printf("Update 스레드3 생성 실패: %d", GetLastError());
		return false;
	}

	hThreadArr[0] = hAcceptThread;
	hThreadArr[1] = hDisconnectThread;
	hThreadArr[2] = hUpdateThread1;
	hThreadArr[3] = hUpdateThread2;
	hThreadArr[4] = hUpdateThread3;
	return true;
}

void CleanUp()
{
	CloseHandle(hEventExitThread);

	timeEndPeriod(1);
}

DWORD waitTime1;
unsigned _stdcall AcceptThread(void* args)
{
	
	srand((unsigned int)_time32(nullptr) + 200);

	DWORD errorCode;
	DWORD result;
	DWORD waitTime = rand() % 901 + 100;
	for (;;)
	{
		result = WaitForSingleObject(hEventExitThread, waitTime);
		switch (result)
		{
		case WAIT_EXIT_EVENT:
			printf("Accept 스레드 종료\n");
			return 0;
		case WAIT_TIMEOUT:
			waitTime = rand() % 901 + 100;
			for (DWORD i = 0; i < waitTime; ++i)
			{
				InterlockedIncrement((long*)&g_Connect);
			}
			continue;
		case WAIT_FAILED:
			errorCode = GetLastError();
		}
	}
	return 0;
}

DWORD waitTime2;
unsigned _stdcall DisconnectThread(void* args)
{
	srand((unsigned int)_time32(nullptr) + 100);

	DWORD errorCode;
	DWORD result;
	DWORD waitTime = rand() % 901 + 100;
	for (;;)
	{
		result = WaitForSingleObject(hEventExitThread, waitTime);
		switch (result)
		{
		case WAIT_EXIT_EVENT:
			printf("Disconnect 스레드 종료\n");
			return 0;
		case WAIT_TIMEOUT:
			waitTime = rand() % 901 + 100;

			if (InterlockedCompareExchange((long*)&g_Connect, 0, 0) == 0)
			{
				continue;
			}

			for (DWORD i = 0; i < waitTime; ++i)
			{
				if (InterlockedDecrement((long*)&g_Connect) == 0)
				{
					break;
				}
			}
			continue;
		case WAIT_FAILED:
			errorCode = GetLastError();
		}

		break;
	}
	return 0;
}

unsigned _stdcall UpdateThread(void* args)
{
	DWORD errorCode;
	DWORD result;
	int data;
	for (;;)
	{
		result = WaitForSingleObject(hEventExitThread, 10);
		switch (result)
		{
		case WAIT_EXIT_EVENT:
			printf("Update 스레드%lld 종료\n", (long long)args);
			return 0;
		case WAIT_TIMEOUT:
			data = InterlockedIncrement((long*)&g_Data);
			if (data % 1000 == 0)
			{
				printf("g_Data: %d\n", data);
			}
			break;
		case WAIT_FAILED:
			errorCode = GetLastError();
			break;
		}
	}
	return 0;
}