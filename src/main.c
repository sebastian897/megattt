#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#define ARRAY_LENGTH(x) ((int)(sizeof(x) / sizeof((x)[0])))

static const uint16_t cell_patterns[] = {7, 56, 448, 73, 146, 292, 273, 84};

typedef enum CellState { CELL_EMPTY, CELL_X, CELL_O } CellState;

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

uint8_t CalcBigGridState(BigGrid* grid) {
  uint16_t player_pattern[2] = {0, 0};
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->grids[c].state == CELL_X) {
      player_pattern[0] += 2 ^ c;
      player_count[0] += 1;
    } else if (grid->grids[c].state == CELL_O) {
      player_pattern[1] += 2 ^ c;
      player_count[1] += 1;
    }
  }
  for (int player = 0; player < 2; player++) {
    for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
      if (player_pattern[player] == cell_patterns[pat]) {
        return player;
      }
    }
  }
  if (player_count[0] + player_count[1] == 9) {
    return (player_count[0] < player_count[1]) + 1;
  }
}

void CalcSmallGridState(SmallGrid* grid) {
  uint16_t player_pattern[2] = {0, 0};
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->cells[c].state == CELL_X) {
      player_pattern[0] += 2 ^ c;
      player_count[0] += 1;
    } else if (grid->cells[c].state == CELL_O) {
      player_pattern[1] += 2 ^ c;
      player_count[1] += 1;
    }
  }
  for (int player = 0; player < 2; player++) {
    for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
      if (player_pattern[player] == cell_patterns[pat]) {
        grid->state = player + 1;
      }
    }
  }
  if (grid->state == CELL_EMPTY && player_count[0] + player_count[1] == 9) {
    grid->state = (player_count[0] < player_count[1]) + 1;
  }
}

Rectangle scale_rect(Rectangle rect, float scale) {
  float new_width = rect.width * scale;
  float new_height = rect.height * scale;
  return (Rectangle){rect.x + (rect.width - new_width) / 2, rect.y + (rect.height - new_height) / 2,
                     new_width, new_height};
}

void DrawTTTShape(Rectangle rect) {
  float line_thickness = rect.height / 50;
  float square_size = (rect.width - line_thickness * 2) / 3;
  for (int i = 1; i < 3; i++) {
    DrawRectangleRec((Rectangle){rect.x + i * square_size + (i - 1) * line_thickness, rect.y,
                                 line_thickness, rect.height},
                     BLACK);
  }
  for (int i = 1; i < 3; i++) {
    DrawRectangleRec((Rectangle){rect.x, rect.y + i * square_size + (i - 1) * line_thickness,
                                 rect.width, line_thickness},
                     BLACK);
  }
}

void DrawGameArea(Vector2 pos, Vector2 window_size) {
  Rectangle big_grid_rect = (Rectangle){pos.x, pos.y, window_size.x, window_size.y};
  DrawTTTShape(big_grid_rect);
  float big_grid_line_thickness = big_grid_rect.height / 50;
  float small_grid_size = (big_grid_rect.width - big_grid_line_thickness * 2) / 3;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      DrawTTTShape(scale_rect((Rectangle){pos.x + j * (small_grid_size + big_grid_line_thickness),
                                          pos.y + i * (small_grid_size + big_grid_line_thickness),
                                          small_grid_size, small_grid_size},
                              0.9));
    }
  }
}

int main(void) {
  Vector2 window_size = {1200, 1200};
  InitWindow(window_size.x, window_size.y, "Mega Tic Tac Toe");
  Vector2 game_area_pos = {0, 0};
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(WHITE);
    DrawGameArea(game_area_pos, window_size);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}