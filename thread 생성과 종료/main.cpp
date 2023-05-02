#include <process.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <time.h>

#pragma comment(lib, "winmm.lib")

#define	WAIT_EXIT_EVENT	WAIT_OBJECT_0
#define WAIT_SIGNAL WAIT_OBJECT_0 + 1

#define NOP	WAIT_OBJECT_0 + 20
#define NOP1	WAIT_OBJECT_0 + 21
#define NOP2	WAIT_OBJECT_0 + 22

#define TOTAL_THREAD_CNT	5
#define	EXIT_AFTER_N_SEC	20

int g_Data = 0;
int g_Connect = 0;
//int g_Shutdown = false;

HANDLE hAcceptThread;
HANDLE hDisconnectThread;
HANDLE hUpdateThread1;
HANDLE hUpdateThread2;
HANDLE hUpdateThread3;

HANDLE hThreadArr[TOTAL_THREAD_CNT];

HANDLE hEventExitThread;
HANDLE hEventAccept;
HANDLE hEventDisconnect;
HANDLE hEventUpdate;

HANDLE hEventArrForAcceptThread[2];
HANDLE hEventArrForDisconnectThread[2];
HANDLE hEventArrForUpdateThread[2];
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
			printf("g_Connect: %d\n", g_Connect);
			countPerSec += 1;
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
	
	
	printf("경과시간: %d초\n", (timeGetTime() - startTime) / 1000);

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
	hEventAccept = CreateEvent(nullptr, false, false, L"AcceptEvent");
	if (hEventAccept == nullptr)
	{
		printf("Accept 이벤트객체 생성 실패: %d", GetLastError());
		return false;
	}
	hEventDisconnect = CreateEvent(nullptr, false, false, L"DisconnectEvent");
	if (hEventDisconnect == nullptr)
	{
		printf("Disconnect 이벤트객체 생성 실패: %d", GetLastError());
		return false;
	}
	hEventUpdate = CreateEvent(nullptr, false, false, L"UpdateEvent");
	if (hEventUpdate == nullptr)
	{
		printf("Update 이벤트객체 생성 실패: %d", GetLastError());
		return false;
	}
	
	hEventArrForAcceptThread[0] = hEventExitThread;
	hEventArrForAcceptThread[1] = hEventAccept;

	hEventArrForDisconnectThread[0] = hEventExitThread;
	hEventArrForDisconnectThread[1] = hEventDisconnect;

	hEventArrForUpdateThread[0] = hEventExitThread;
	hEventArrForUpdateThread[1] = hEventUpdate;
	
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
		printf("접속 스레드 생성 실패: %d", GetLastError());
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
	CloseHandle(hEventAccept);
	CloseHandle(hEventDisconnect);
	CloseHandle(hEventUpdate);

	timeEndPeriod(1);
}

#define _MT

DWORD waitTime1;
unsigned _stdcall AcceptThread(void* args)
{
	
	srand((unsigned int)_time32(nullptr) + 200);

	DWORD errorCode;
	DWORD result;
	DWORD waitTime = rand() % 901 + 100;
	for (;;)
	{
		result = WaitForMultipleObjects(2, hEventArrForAcceptThread, false, waitTime);
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
	int decrementCnt;
	for (;;)
	{
		result = WaitForMultipleObjects(2, hEventArrForDisconnectThread, false, waitTime);
		switch (result)
		{
		case WAIT_EXIT_EVENT:
			printf("Disconnect 스레드 종료\n");
			return 0;
		case WAIT_TIMEOUT:
			waitTime = rand() % 901 + 100;
			
			for (DWORD i = 0; i < waitTime; ++i)
			{
				InterlockedDecrement((long*)&g_Connect);
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
		result = WaitForMultipleObjects(2, hEventArrForUpdateThread, false, 10);
		switch (result)
		{
		case WAIT_EXIT_EVENT:
			printf("Update 스레드%lld 종료\n", (long long)args);
			return 0;
		case WAIT_SIGNAL:
			data = InterlockedIncrement((long*)&g_Data);
			if (data % 1000 == 0)
			{
				printf("g_Data: %d\n", data);
			}
			break;
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