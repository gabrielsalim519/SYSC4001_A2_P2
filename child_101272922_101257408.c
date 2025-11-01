#include "common.h"

int wmain(int argc, wchar_t **wargv) {
    int mode = (argc >= 2) ? _wtoi(wargv[1]) : 1;

    // Handles for parts 4–5
    HANDLE hMap = NULL, hSem = NULL;
    Shared *sh = NULL;

    if (mode >= 4) {
        // Open objects created by parent
        hMap = OpenFileMappingW(FILE_MAP_ALL_ACCESS, FALSE, SHM_NAME);
        if (!hMap) { fwprintf(stderr, L"child: OpenFileMapping failed (%lu)\n", GetLastError()); return 2; }
        sh = (Shared*)MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Shared));
        if (!sh) { fwprintf(stderr, L"child: MapViewOfFile failed (%lu)\n", GetLastError()); return 2; }

        if (mode == 5) {
            hSem = OpenSemaphoreW(SYNCHRONIZE | SEMAPHORE_MODIFY_STATE, FALSE, SEM_NAME);
            if (!hSem) { fwprintf(stderr, L"child: OpenSemaphore failed (%lu)\n", GetLastError()); return 2; }
        }
    }

    long long cycles = 0;
    int counter = 0;

    while (1) {
        if (mode <= 3) {
            // Part 1–3: child counts down; print multiples of 3 message like spec
            if (cycles == 0) counter = 0;
            counter--;
            if (counter % 3 == 0) {
                wprintf(L"[Child] Cycle number: %lld – %d is a multiple of 3\n", cycles, counter);
            } else {
                wprintf(L"[Child] Cycle number: %lld\n", cycles);
            }
            fflush(stdout);

            if (mode == 3 && counter < -500) {
                // Exits → parent’s wait returns
                break;
            }
        } else {
            // Part 4–5: shared memory behavior
            // Wait until counter1 > 100 before emitting anything
            int start = 0;
            if (mode == 5) WaitForSingleObject(hSem, INFINITE);
            start = (sh->counter1 > 100);
            if (mode == 5) ReleaseSemaphore(hSem, 1, NULL);

            if (!start) { snooze_ms(50); cycles++; continue; }

            LONG mult, c1, stop = 0;
            if (mode == 5) WaitForSingleObject(hSem, INFINITE);
            mult = sh->multiple;
            c1   = sh->counter1;
            stop = sh->done;
            if (mode == 5) ReleaseSemaphore(hSem, 1, NULL);

            if (stop) break;

            if (c1 % mult == 0) {
                wprintf(L"[Child] sees counter1=%ld which IS a multiple of %ld\n", c1, mult);
            } else {
                wprintf(L"[Child] sees counter1=%ld\n", c1);
            }
            fflush(stdout);

            // stop when counter1 > 500 (and set done)
            if (mode == 5) WaitForSingleObject(hSem, INFINITE);
            if (sh->counter1 > 500) { sh->done = 1; stop = 1; }
            if (mode == 5) ReleaseSemaphore(hSem, 1, NULL);
            if (stop) break;
        }

        snooze_ms(60);
        cycles++;
    }

    if (sh) UnmapViewOfFile(sh);
    if (hMap) CloseHandle(hMap);
    if (hSem) CloseHandle(hSem);
    return 0;
}
