#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <tlhelp32.h>
#define DLL_EXPORT __declspec(dllexport)
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#define DLL_EXPORT __attribute__((visibility("default")))
#endif
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#ifdef __cplusplus
extern "C" {
#endif


typedef enum {
    EMPTY = 0b000,
    WHITE = 0b001 << 0,
    BLACK = 0b010 << 0,
    MAN = 0b000 << 2,
    KING = 0b001 << 2,

    WHITE_MAN = WHITE | MAN,  // 0b001
    WHITE_KING = WHITE | KING, // 0b101
    BLACK_MAN = BLACK | MAN,  // 0b010
    BLACK_KING = BLACK | KING, // 0b110
} CellFlag;

typedef uint8_t board[32];


DLL_EXPORT void init_board(board b) {
    for (int i = 0; i < 32; ++i) {
        if (i < 12) {
            b[i] = BLACK_MAN;
        }
        else if (i >= 20) {
            b[i] = WHITE_MAN;
        }
        else {
            b[i] = EMPTY;
        }
    }
}
int get_diagonal(int pos, int direction) {
    int row = pos / 4+1;
    int even = (row % 2 == 0);

    int offset = 0;
    switch (direction) {
    case 0: offset = even ? -5 : -4; break; // top-left
    case 1: offset = even ? -4 : -3; break; // top-right
    case 2: offset = even ? +3 : +4; break; // bottom-left
    case 3: offset = even ? +4 : +5; break; // bottom-right
    default: return -1;
    }
    int target = pos + offset; 
    if (target < 0 || target >= 32) return -1;

    int new_row = target / 4+1;
    if (abs(new_row - row) != 1) return -1;

    return target;
}
int get_diagonal_move(const board b, int pos, int i, int* capture) {
    int diagonal = get_diagonal(pos, i);
    if (diagonal == -1) return -1;
    if (b[diagonal] == EMPTY && *capture == 0)
        return diagonal;
    else if (b[diagonal] != EMPTY && (b[diagonal] & WHITE) != (b[pos] & WHITE)) //if different color
    {
        int next_diagonal = get_diagonal(diagonal, i);
        if (next_diagonal != -1 && b[next_diagonal] == EMPTY)
        {
            *capture = 1;
            return next_diagonal;
        }
    }
    return -1;
}
DLL_EXPORT int can_capture(const board b, int color) {
    int capture = 0;
    if (color == -1) color = 0;
    for (int pos = 0; pos < 32; pos++) {
        if (b[pos] != EMPTY && (b[pos] & WHITE) == color) {//color 0 == black, not 0 == white
            if ((b[pos] & KING) == 0) //if not king
            {
                int start_diagonal = 0;
                if ((b[pos] & BLACK) != 0) start_diagonal = 2;
                for (int i = start_diagonal; i < start_diagonal + 2; i++)
                {
                    int diagonal = get_diagonal_move(b, i, i, &capture);
                }
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    int currentPos = get_diagonal_move(b, pos, i, &capture);
                    while (currentPos != -1)
                    {
                        currentPos = get_diagonal_move(b, currentPos, i, &capture);
                    }
                }
            }
        }
    }
    return capture;
}
DLL_EXPORT void get_moves(const board b, int pos, int* out_moves, int* count, int* capture) {
    *count = 0;
    CellFlag piece = b[pos];
    if (piece == EMPTY) return;
    if (pos < 0 || pos > 31) return;


    if ((piece & KING) == 0) //if not king
    {
        int start_diagonal = 0;
        if ((piece & BLACK) != 0) start_diagonal = 2;
        for (int i = start_diagonal; i < start_diagonal+2; i++) 
        {
            int diagonal = get_diagonal_move(b, pos, i, capture);
            out_moves[(*count)++] = diagonal;
        }
    }
    else 
    {
        for (int i = 0; i < 4; i++)
        {
            int currentPos = get_diagonal_move(b, pos, i, capture);
            while (currentPos != -1) 
            {
                out_moves[(*count)++] = currentPos;
                currentPos = get_diagonal_move(b, currentPos, i, capture);
            }
        }
    }
}
DLL_EXPORT board* get_board() {
    board* b = (board*)malloc(sizeof(board));
    init_board(*b);
    return b;
}
__declspec(dllexport) void test() {
    printf("1");
    //return 1;
}
#ifdef __cplusplus
}
#endif