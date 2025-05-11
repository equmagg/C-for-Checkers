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
int8_t get_diagonal(int8_t pos, int direction) {
    int row = pos / 4+1;
    int even = (row % 2 == 0);

    int8_t offset = 0;
    switch (direction) {
    case 0: offset = even ? -5 : -4; break; // top-left
    case 1: offset = even ? -4 : -3; break; // top-right
    case 2: offset = even ? +3 : +4; break; // bottom-left
    case 3: offset = even ? +4 : +5; break; // bottom-right
    default: return -1;
    }
    int8_t target = pos + offset; 
    if (target < 0 || target >= 32) return -1;

    int new_row = target / 4+1;
    if (abs(new_row - row) != 1) return -1;

    return target;
}
int8_t get_diagonal_move(const board b, int8_t pos, int i, int8_t* capture) {
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
DLL_EXPORT int8_t can_capture(const board b, int8_t color) {
    int8_t capture = 0;
    if (color == -1) color = 0;
    for (int pos = 0; pos < 32; pos++) {
        if (b[pos] != EMPTY && (b[pos] & WHITE) == color) {//color 0 == black, not 0 == white
            if ((b[pos] & KING) == 0) //if not king
            {
                int start_diagonal = 0;
                if ((b[pos] & BLACK) != 0) start_diagonal = 2;
                for (int i = start_diagonal; i < start_diagonal + 2; i++)
                {
                    int8_t diagonal = get_diagonal_move(b, pos, i, &capture);
                }
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    int8_t currentPos = get_diagonal_move(b, pos, i, &capture);
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
DLL_EXPORT void get_moves(const board b, int8_t pos, int8_t* out_moves, int8_t* count, int8_t* capture) {
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
            int8_t diagonal = get_diagonal_move(b, pos, i, capture);
            if(diagonal != -1) out_moves[(*count)++] = diagonal;
        }
    }
    else 
    {
        for (int i = 0; i < 4; i++)
        {
            int8_t currentPos = get_diagonal_move(b, pos, i, capture);
            while (currentPos != -1) 
            {
                out_moves[(*count)++] = currentPos;
                currentPos = get_diagonal_move(b, currentPos, i, capture);
            }
        }
    }
}
DLL_EXPORT void get_all_moves(const board b, int8_t* out_moves, int8_t color) {
    if (color == -1) color = 0;
    int8_t count = 0;
    int8_t capture = can_capture(b, color);
    for (int pos = 0; pos < 32; pos++) {
        if (b[pos] != EMPTY && (b[pos] & WHITE) == color) {
            int8_t moves[13];
            int8_t c;
            get_moves(b, pos, moves, &c, &capture);
            for (int i = 0; i < c; i++) {
                out_moves[count++] = pos;
                out_moves[count++] = moves[i];
            }
        
        }
    }
}
DLL_EXPORT int8_t move(board b, int8_t startPos, int8_t endPos) {
    if (startPos < 0 || startPos>31 || endPos < 0 || endPos>31) return 0;
    b[endPos] = b[startPos];
    b[startPos] = 0;
    int8_t direction = -1;
    int row = endPos / 4;
    if (row == 0 && b[endPos] == WHITE_MAN) {
        b[endPos] = WHITE_KING;
    }
    else if (row == 7 && b[endPos] == BLACK_MAN) {
        b[endPos] = BLACK_KING;
    }
    int8_t offset = endPos - startPos;
    if(offset < 0) {//forward 0 1
        int8_t diagonal = startPos;
        int8_t diagonal2 = startPos;
        for (int i = 0; i < 7; i++) {
            diagonal = get_diagonal(diagonal, 0);
            if (diagonal == endPos) {
                direction = 0;
                break;
            }
            diagonal2 = get_diagonal(diagonal2, 1);
            if (diagonal2 == endPos) {
                direction = 1;
                break;
            }
        }
    }
    else if(offset > 0) {//back 2 3
        int8_t diagonal = startPos;
        int8_t diagonal2 = startPos;
        for (int i = 0; i < 7; i++) {
            diagonal = get_diagonal(diagonal, 2);
            if (diagonal == endPos) {
                direction = 2;
                break;
            }
            diagonal2 = get_diagonal(diagonal2, 3);
            if (diagonal2 == endPos) {
                direction = 3;
                break;
            }
        }
    }
    if (direction == -1) return 0;
    int inverted = 3 - direction;
    int8_t diagonal = get_diagonal(endPos, inverted);
    if (diagonal != -1 && b[diagonal] != EMPTY) {
        b[diagonal] = 0;
        int8_t capture = 0;
        int8_t count = 0;
        int8_t moves[13];
        get_moves(b, endPos, moves, &count, &capture);
        return capture;//can capture again
    }
    return 0;
}
DLL_EXPORT int test() {
    return 1;
}
#ifdef __cplusplus
}
#endif