#define _WINSOCKAPI_
#include <windows.h>
#include <synchapi.h>
#include "SpinLock.h"


static DWORD lock = 0;

bool EnterSpinLock()
{
	if (InterlockedExchange(&lock, 1) != 0)
	{
		return false;
	}

	return true;
}

void DeleteSpinLock()
{
	InterlockedExchange(&lock, 0);
}