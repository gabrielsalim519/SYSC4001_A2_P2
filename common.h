// common.h (Windows version)
#ifndef COMMON_H
#define COMMON_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    LONG multiple;   // e.g., 3
    LONG counter1;   // Process 1’s counter
    LONG done;       // 0->running, 1->stop both
} Shared;

static __inline void snooze_ms(DWORD ms) { Sleep(ms); }

// Names for shared objects (use Global\ to work across sessions if needed)
#define SHM_NAME  L"Local\\A2P2_Shared"
#define SEM_NAME  L"Local\\A2P2_Mutex"

#endif
