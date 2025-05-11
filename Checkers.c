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
        if (i < 12) b[i] = BLACK_MAN; 
        else if (i >= 20)  b[i] = WHITE_MAN;
        else  b[i] = EMPTY;
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
                    if (capture) return capture;
                }
            }
            else
            {
                for (int i = 0; i < 4; i++)
                {
                    int8_t currentPos = pos;
                    int seen_enemy = 0;

                    while (1)
                    {
                        int8_t nextPos = get_diagonal(currentPos, i);
                        if (nextPos == -1) break;

                        if (b[nextPos] == EMPTY)
                        {
                            if (seen_enemy) {
                                return 1;
                            }
                            currentPos = nextPos;
                        }
                        else if ((b[nextPos] & WHITE) != (b[pos] & WHITE))
                        {
                            if (seen_enemy) break; 
                            seen_enemy = 1;
                            currentPos = nextPos;
                        }
                        else
                        {
                            break; 
                        }
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
    if (piece == EMPTY || pos < 0 || pos > 31) return;

    int8_t temp_moves[32];
    int8_t temp_count = 0;
    int8_t local_capture = 0;

    if ((piece & KING) == 0) 
    {
        int start_diagonal = ((piece & BLACK) != 0) ? 2 : 0;
        for (int i = start_diagonal; i < start_diagonal + 2; i++)
        {
            int8_t temp_cap = 0;
            int8_t diagonal = get_diagonal_move(b, pos, i, &temp_cap);

            if (diagonal != -1) {
                if (temp_cap) {
                    if (!local_capture) temp_count = 0;
                    temp_moves[temp_count++] = diagonal;
                    local_capture = 1;
                }
                else if (!local_capture && *capture == 0) {
                    temp_moves[temp_count++] = diagonal;
                }
            }
        }
    }
    else 
    {
        for (int i = 0; i < 4; i++)
        {
            int8_t currentPos = pos;
            int seen_enemy = 0;

            while (1)
            {
                int8_t nextPos = get_diagonal(currentPos, i);
                if (nextPos == -1) break;

                if (b[nextPos] == EMPTY)
                {
                    if (seen_enemy)
                    {
                        if (!local_capture) temp_count = 0;
                        temp_moves[temp_count++] = nextPos;
                        local_capture = 1;
                        break;
                    }
                    else if (!local_capture && *capture == 0)
                    {
                        temp_moves[temp_count++] = nextPos;
                        currentPos = nextPos;
                    }
                    else
                    {
                        break;
                    }
                }
                else if ((b[nextPos] & WHITE) != (piece & WHITE))
                {
                    if (seen_enemy) break;
                    seen_enemy = 1;
                    currentPos = nextPos;
                }
                else
                {
                    break;
                }
            }
        }
    }

    *capture = local_capture;

    if (*capture == 1 || local_capture == 0)
    {
        for (int i = 0; i < temp_count; i++) {
            out_moves[(*count)++] = temp_moves[i];
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
            int8_t temp_count;
            get_moves(b, pos, moves, &temp_count, &capture);
            for (int i = 0; i < temp_count; i++) {
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
DLL_EXPORT void get_bitboard(int8_t * short_board) {
    CellFlag b[32];
    for (int i = 0; i < 32; ++i) {
        if (i < 12) b[i] = BLACK_MAN;
        else if (i >= 20)  b[i] = WHITE_MAN;
        else  b[i] = EMPTY;
    }
    for (int i = 0; i < 16; i++) {
        short_board[i] = (uint8_t)((b[2 * i] << 0) | (b[2 * i + 1] << 3));
    }
}
#ifdef __cplusplus
}
#endif