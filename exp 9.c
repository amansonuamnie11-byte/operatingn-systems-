#include <windows.h>
#include <stdio.h>

int main() {
    HANDLE hMap;
    char *data;

    // Create shared memory segment
    hMap = CreateFileMapping(
        INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        0, 1024, "MySharedMemory"
    );

    if (hMap == NULL) {
        printf("CreateFileMapping failed: %lu\n", GetLastError());
        return 1;
    }

    // Map shared memory
    data = (char*) MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 1024);
    if (data == NULL) {
        printf("MapViewOfFile failed: %lu\n", GetLastError());
        CloseHandle(hMap);
        return 1;
    }

    // Create a child process to read the shared memory
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Child process will read shared memory (same program with argument)
    CreateProcess(
        NULL,
        "IPC.exe child",    // Run same program with "child" argument
        NULL, NULL, FALSE, 0, NULL, NULL,
        &si, &pi
    );

    // ============================
    // PARENT PROCESS (WRITER)
    // ============================
    if (__argc == 1) {    // No arguments → this is parent
        strcpy(data, "Hello from Parent using Shared Memory!");
        printf("Parent wrote: %s\n", data);

        // Wait for child to read
        WaitForSingleObject(pi.hProcess, INFINITE);

        UnmapViewOfFile(data);
        CloseHandle(hMap);
        return 0;
    }

    // ============================
    // CHILD PROCESS (READER)
    // ============================
    if (__argc == 2 && strcmp(__argv[1], "child") == 0) {

        // Attach to same memory
        HANDLE hMap2 = OpenFileMapping(FILE_MAP_READ, FALSE, "MySharedMemory");
        char *readData = (char*) MapViewOfFile(hMap2, FILE_MAP_READ, 0, 0, 1024);

        printf("Child read: %s\n", readData);

        UnmapViewOfFile(readData);
        CloseHandle(hMap2);
        return 0;
    }

    return 0;
}
