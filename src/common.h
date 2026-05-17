#pragma once
#include <stdint.h>

#define SERVER "192.168.40.158"
#define PORT 5150
#define BUFLEN 1024
#define PLAYERS_MAX 2

typedef enum PlayerState { MENU, CONNECTING, PLAYING, GAME_OVER, EXIT } PlayerState;

typedef enum CellState { CELL_EMPTY, CELL_X, CELL_O, CELL_DRAW } CellState;
typedef struct Cell {
  CellState state;
} Cell;

typedef struct SmallGrid {
  Cell cells[9];
  CellState state;
} SmallGrid;

typedef struct BigGrid {
  SmallGrid grids[9];
} BigGrid;

typedef struct PlayerMove {
  int big_grid_idx;
  int small_grid_idx;
} PlayerMove;

typedef struct game_packet {
  BigGrid grid;
  uint8_t turn;
} game_packet;