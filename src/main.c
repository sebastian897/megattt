#include <raylib.h>
#include <stdint.h>

typedef enum CellState
{
  CELL_EMPTY,
  CELL_X,
  CELL_O
}CellState;

struct Cell {
  CellState state;
};

int main(void) {
  InitWindow(800, 600, "Mega Tic Tac Toe");

  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}