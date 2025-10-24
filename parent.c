// parent_win.c
#include "common.h"

static void run_mode12(void) {
    long long cycles = 0;
    int counter = 0;
    for (;;) {
        if (counter % 3 == 0) {
            wprintf(L"[Parent] Cycle number: %lld – %d is a multiple of 3\n", cycles, counter);
        } else {
            wprintf(L"[Parent] Cycle number: %lld\n", cycles);
        }
        fflush(stdout);
        counter++;
        snooze_ms(60);
        cycles++;
    }
}

static void run_mode3(HANDLE hChild) {
    long long cycles = 0;
    int counter = 0;
    for (;;) {
        if (counter % 3 == 0) {
            wprintf(L"[Parent] Cycle number: %lld – %d is a multiple of 3\n", cycles, counter);
        } else {
            wprintf(L"[Parent] Cycle number: %lld\n", cycles);
        }
        fflush(stdout);
        counter++;

        DWORD r = WaitForSingleObject(hChild, 0);
        if (r == WAIT_OBJECT_0) {
            DWORD code = 0; GetExitCodeProcess(hChild, &code);
            wprintf(L"[Parent] Child finished (exit=%lu). Parent exiting.\n", code);
            fflush(stdout);
            break;
        }
        snooze_ms(60);
        cycles++;
    }
}

static void run_mode45(int mode, HANDLE hMap, HANDLE hSem) {
    Shared *sh = (Shared*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
    if (!sh) { fwprintf(stderr, L"parent: MapViewOfFile failed (%lu)\n", GetLastError()); return; }

    if (mode == 5) WaitForSingleObject(hSem, INFINITE);
    sh->multiple = 3;
    sh->counter1 = 0;
    sh->done     = 0;
    if (mode == 5) ReleaseSemaphore(hSem, 1, NULL);

    long long cycles = 0;
    for (;;) {
        LONG c1, mult, stop;
        if (mode == 5) WaitForSingleObject(hSem, INFINITE);
        c1   = ++(sh->counter1);
        mult = sh->multiple;
        stop = sh->done;
        if (mode == 5) ReleaseSemaphore(hSem, 1, NULL);

        if (c1 % mult == 0) {
            wprintf(L"[Parent] counter1=%ld – %ld is a multiple of %ld\n", c1, c1, mult);
        } else {
            wprintf(L"[Parent] counter1=%ld\n", c1);
        }
        fflush(stdout);

        if (stop || c1 > 500) {
            if (mode == 5) WaitForSingleObject(hSem, INFINITE);
            sh->done = 1;
            if (mode == 5) ReleaseSemaphore(hSem, 1, NULL);
            break;
        }
        snooze_ms(40);
        cycles++;
    }

    UnmapViewOfFile(sh);
}

static BOOL spawn_child(int mode, PROCESS_INFORMATION *pi) {
    wchar_t cmd[256];
    _snwprintf(cmd, 255, L"child.exe %d", mode);

    STARTUPINFOW si = {0};
    si.cb = sizeof(si);
    return CreateProcessW(
        NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, pi
    );
}

int wmain(int argc, wchar_t **wargv) {
    int mode = (argc >= 2) ? _wtoi(wargv[1]) : 1;

    // Create shared objects for modes 4–5
    HANDLE hMap = NULL, hSem = NULL;
    if (mode >= 4) {
        hMap = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
                                  0, sizeof(Shared), SHM_NAME);
        if (!hMap) { fwprintf(stderr, L"parent: CreateFileMapping failed (%lu)\n", GetLastError()); return 2; }
        if (mode == 5) {
            hSem = CreateSemaphoreW(NULL, 1, 1, SEM_NAME);
            if (!hSem) { fwprintf(stderr, L"parent: CreateSemaphore failed (%lu)\n", GetLastError()); return 2; }
        }
    }

    // Launch child.exe <mode>
    PROCESS_INFORMATION pi = {0};
    if (!spawn_child(mode, &pi)) {
        fwprintf(stderr, L"parent: CreateProcess(child) failed (%lu)\n", GetLastError());
        return 1;
    }

    // Parts behavior
    if (mode == 1 || mode == 2) {
        // Infinite until you press Ctrl+C in this console (akin to “kill from shell”)
        run_mode12();
    } else if (mode == 3) {
        run_mode3(pi.hProcess);               // exits when child finishes (< -500)
        WaitForSingleObject(pi.hProcess, INFINITE);
    } else {
        run_mode45(mode, hMap, hSem);         // shared memory (+ semaphore in mode 5)
        WaitForSingleObject(pi.hProcess, INFINITE);
    }

    // Cleanup
    if (hMap) CloseHandle(hMap);
    if (hSem) CloseHandle(hSem);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 0;
}
