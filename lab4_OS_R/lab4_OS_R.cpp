#include <iostream>
#include <ctime>
#include <Windows.h>
#include <string>

int main()
{
	const int pageSize = 4096;
	const int pageCount = 12;

	srand(time(nullptr));

	HANDLE hMappedFile = OpenFileMapping(GENERIC_READ, false, L"mappedFile");
	DWORD written = 0;
	HANDLE handleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

	if (hMappedFile != INVALID_HANDLE_VALUE) {
		LPVOID fileView = MapViewOfFile(hMappedFile, FILE_MAP_READ, NULL, NULL, pageCount * pageSize);
		HANDLE IOMutex = OpenMutex(MUTEX_MODIFY_STATE | SYNCHRONIZE, false, L"IOMutex");
		HANDLE readSemaphore[pageCount], writeSemaphore[pageCount];

		DWORD page = 0;

		for (int i = 0; i < pageCount; ++i) {
			std::wstring mutexName = L"readSemaphore_" + std::to_wstring(i);
			readSemaphore[i] = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, false, mutexName.data());
			mutexName = L"writeSemaphore_" + std::to_wstring(i);
			writeSemaphore[i] = OpenSemaphore(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, false, mutexName.data());
		}

		std::string msg = "PROCESS NUMBER " + std::to_string(GetCurrentProcessId()) + " STARTED:\n";
		WriteFile(handleOutput, msg.data(), msg.length(), &written, NULL);

		VirtualLock(fileView, pageCount * pageSize);

		for (int i = 0; i < 3; ++i) {
			page = WaitForMultipleObjects(pageCount, readSemaphore, false, INFINITE);
			msg = "\t" + std::to_string(GetTickCount()) + " | TAKE | SEMAPHORE\n";
			WriteFile(handleOutput, msg.data(), msg.length(), &written, NULL);

			WaitForSingleObject(IOMutex, INFINITE);
			msg = "\t" + std::to_string(GetTickCount()) + " | TAKE | MUTEX\n";
			WriteFile(handleOutput, msg.data(), msg.length(), &written, NULL);

			Sleep(std::rand() % 1001 + 500);
			msg = "\t" + std::to_string(GetTickCount()) + " | READ | PAGE: " + std::to_string(page);
			WriteFile(handleOutput, msg.data(), msg.length(), &written, NULL);

			ReleaseMutex(IOMutex);
			msg = "\t" + std::to_string(GetTickCount()) + " | FREE | MUTEX\n";
			WriteFile(handleOutput, msg.data(), msg.length(), &written, NULL);

			ReleaseSemaphore(writeSemaphore[page], 1, NULL);
			msg = "\t" + std::to_string(GetTickCount()) + " | FREE | SEMAPHORE\n";
			WriteFile(handleOutput, msg.data(), msg.length(), &written, NULL);
		}

		VirtualUnlock(fileView, pageSize * pageCount);

		for (int i = 0; i < pageCount; i++) {
			CloseHandle(readSemaphore[i]);
			CloseHandle(writeSemaphore[i]);
		}

		CloseHandle(IOMutex);
		CloseHandle(hMappedFile);

		UnmapViewOfFile(fileView);
	}
	else {
		std::string msg = "CAN\'T OPEN MAPPED FILE, ERROR: " + std::to_string(GetLastError());
		WriteFile(handleOutput, msg.data(), msg.size(), &written, NULL);
	}

	CloseHandle(handleOutput);

	return 0;
}
