#include <iostream>
#include <fstream>
#include <windows.h>
#include <string>
#include <ctime>

void LogWrite(std::string data, HANDLE outHandle) {
	WriteFile(outHandle, data.c_str(), data.length(), NULL, NULL);
}

int main()
{
	const int pageSize = 4096;
	const int pageCount = 12;

	srand(time(NULL));
	HANDLE writeSemaphores[pageCount];
	HANDLE readSemaphores[pageCount];
	HANDLE IOMutex = OpenMutex(MUTEX_MODIFY_STATE | SYNCHRONIZE, FALSE, "IOMutex");
	HANDLE mapFile = OpenFileMapping(GENERIC_READ, FALSE, "MAPPING");
	LPVOID fileView = MapViewOfFile(mapFile, FILE_MAP_READ, 0, 0, pageSize * pageCount);
	HANDLE handleStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD page = 0;
	for (int i = 0; i < pageCount; i++) {
		std::string semaphoreName = "writeSemaphore_" + std::to_string(i);
		writeSemaphores[i] = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, semaphoreName.c_str());
		semaphoreName = "readSemaphore_" + std::to_string(i);
		readSemaphores[i] = OpenSemaphore(SEMAPHORE_MODIFY_STATE | SYNCHRONIZE, FALSE, semaphoreName.c_str());
	}
	
	VirtualLock(fileView, pageSize * pageCount);

	for (int i = 0; i < 3; i++) {
		page = WaitForMultipleObjects(pageCount, readSemaphores, FALSE, INFINITE);
		LogWrite("TAKE | Semaphore | " + std::to_string(GetTickCount()) + "\n", handleStdOut);

		WaitForSingleObject(IOMutex, INFINITE);
		LogWrite("TAKE | Mutex | " + std::to_string(GetTickCount()) + "\n", handleStdOut);
		Sleep(500 + (rand() % 1001));
		LogWrite("READ | Page: " + std::to_string(page) + " | " + std::to_string(GetTickCount()) + "\n", handleStdOut);

		ReleaseMutex(IOMutex);
		LogWrite("FREE | Mutex | " + std::to_string(GetTickCount()) + "\n", handleStdOut);

		ReleaseSemaphore(writeSemaphores[page], 1, NULL);
		LogWrite("FREE | Semaphore | " + std::to_string(GetTickCount()) + "\n\n", handleStdOut);

	}

	CloseHandle(IOMutex);
	CloseHandle(mapFile);
	CloseHandle(fileView);
	CloseHandle(handleStdOut);
	return 0;
}