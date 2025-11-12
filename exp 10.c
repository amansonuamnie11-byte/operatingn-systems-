#include <windows.h>
#include <stdio.h>

int main() {
    HANDLE hQueue;
    char msg[100];

    // Create a message queue
    hQueue = CreateMailslot(
        "\\\\.\\mailslot\\MyMessageQueue", // Queue name
        0,                                 // No max message size limit
        MAILSLOT_WAIT_FOREVER,             // Blocking read
        NULL                               // Default security
    );

    // If creation failed, try opening as client (child)
    if (hQueue == INVALID_HANDLE_VALUE) {
        // Child – open existing queue
        HANDLE hClient = CreateFile(
            "\\\\.\\mailslot\\MyMessageQueue",
            GENERIC_WRITE,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hClient == INVALID_HANDLE_VALUE) {
            printf("Unable to open message queue.\n");
            return 1;
        }

        // CHILD PROCESS: read incoming message
        DWORD bytesRead;
        HANDLE hSlot = CreateMailslot(
            "\\\\.\\mailslot\\MyMessageQueue",
            0,
            MAILSLOT_WAIT_FOREVER,
            NULL
        );

        if (hSlot == INVALID_HANDLE_VALUE) {
            printf("Child: Unable to open mailslot.\n");
            return 1;
        }

        ReadFile(hSlot, msg, sizeof(msg), &bytesRead, NULL);
        msg[bytesRead] = '\0';

        printf("Child received: %s\n", msg);

        CloseHandle(hSlot);
        CloseHandle(hClient);
        return 0;
    }

    // Parent process – create child to read message
    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    CreateProcess(
        NULL,
        "msgq.exe child",
        NULL, NULL, FALSE, 0, NULL, NULL,
        &si, &pi
    );

    Sleep(500); // Give child time to start reading

    // PARENT: send message to child
    HANDLE hClient = CreateFile(
        "\\\\.\\mailslot\\MyMessageQueue",
        GENERIC_WRITE,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hClient == INVALID_HANDLE_VALUE) {
        printf("Parent: Unable to write.\n");
        return 1;
    }

    DWORD bytesWritten;
    char *data = "Hello from Parent via Message Queue!";
    WriteFile(hClient, data, strlen(data), &bytesWritten, NULL);

    printf("Parent sent: %s\n", data);

    // Wait for child to print output
    WaitForSingleObject(pi.hProcess, INFINITE);

    CloseHandle(hQueue);
    CloseHandle(hClient);

    return 0;
}
