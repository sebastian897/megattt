#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#include "raymath.h"

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

void DrawGameArea(Rectangle game_area_rect) {
  DrawTTTShape(game_area_rect);
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.width - big_grid_line_thickness * 2) / 3;
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      DrawTTTShape(
          scale_rect((Rectangle){game_area_rect.x + j * (small_grid_size + big_grid_line_thickness),
                                 game_area_rect.y + i * (small_grid_size + big_grid_line_thickness),
                                 small_grid_size, small_grid_size},
                     0.9));
    }
  }
}

void OnMouseClick(BigGrid* grid, Rectangle game_area_rect, uint8_t* player) {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
  Vector2 selected_pos =
      Vector2Subtract(GetMousePosition(), (Vector2){game_area_rect.x, game_area_rect.y});
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.width - big_grid_line_thickness * 2) / 3;
  float small_grid_line_thickness = small_grid_size * 0.9 / 50;
  float cell_size = (small_grid_size - small_grid_line_thickness * 2) / 3;
  for (int b_col = 0; b_col < 3; b_col++) {
    for (int b_row = 0; b_row < 3; b_row++) {
      for (int s_col = 0; s_col < 3; s_col++) {
        for (int s_row = 0; s_row < 3; s_row++) {
          Rectangle cell_rect = scale_rect(
              (Rectangle){game_area_rect.x + b_col * (small_grid_size + big_grid_line_thickness) +
                              s_col * (cell_size + small_grid_line_thickness),
                          game_area_rect.y + b_row * (small_grid_size + big_grid_line_thickness) +
                              s_row * (cell_size + small_grid_line_thickness),
                          cell_size, cell_size},
              0.9);
          if (CheckCollisionPointRec(selected_pos, cell_rect)) {
            grid->grids[b_row * 3 + b_col].cells[s_row * 3 + s_col].state = *player + 1;
            *player = (*player + 1) % 2;
          }
        }
      }
    }
  }
}

void DrawTurns(BigGrid* grid, Rectangle game_area_rect) {
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.width - big_grid_line_thickness * 2) / 3;
  float small_grid_line_thickness = small_grid_size / 50;
  float cell_size = (small_grid_size - small_grid_line_thickness * 2) / 3 * 0.9;
  float shape_thickness = cell_size / 10;
  for (int b_col = 0; b_col < 3; b_col++) {
    for (int b_row = 0; b_row < 3; b_row++) {
      Rectangle small_grid_rect = scale_rect(
          (Rectangle){game_area_rect.x + b_col * (small_grid_size + big_grid_line_thickness),
                      game_area_rect.y + b_row * (small_grid_size + big_grid_line_thickness),
                      small_grid_size, small_grid_size},
          0.9);
      for (int s_col = 0; s_col < 3; s_col++) {
        for (int s_row = 0; s_row < 3; s_row++) {
          CellState cell_state = grid->grids[b_row * 3 + b_col].cells[s_row * 3 + s_col].state;
          if (cell_state != CELL_EMPTY) {
            Rectangle cell_rect = scale_rect(
                (Rectangle){small_grid_rect.x + s_col * (cell_size + small_grid_line_thickness),
                            small_grid_rect.y + s_row * (cell_size + small_grid_line_thickness),
                            cell_size, cell_size},
                0.85);
            if (cell_state == CELL_X) {
              DrawLineEx((Vector2){cell_rect.x, cell_rect.y},
                         (Vector2){cell_rect.x + cell_rect.width, cell_rect.y + cell_rect.height},
                         cell_rect.height / 10, RED);
              DrawLineEx((Vector2){cell_rect.x, cell_rect.y + cell_rect.height},
                         (Vector2){cell_rect.x + cell_rect.width, cell_rect.y},
                         cell_rect.height / 10, RED);
            } else if (cell_state == CELL_O) {
              DrawCircle(cell_rect.x + cell_rect.width / 2, cell_rect.y + cell_rect.height / 2,
                         cell_rect.width / 2, BLUE);
              DrawCircle(cell_rect.x + cell_rect.width / 2, cell_rect.y + cell_rect.height / 2,
                         cell_rect.width / 2 - shape_thickness, WHITE);
            }
          }
        }
      }
    }
  }
}

int main(void) {
  Vector2 window_size = {1200, 1200};
  InitWindow(window_size.x, window_size.y, "MegaTicTacToe");
  Rectangle game_area_rect = (Rectangle){0, 0, window_size.x, window_size.y};
  BigGrid grid = {0};
  uint8_t player = 0;
  SetTargetFPS(60);
  while (!WindowShouldClose()) {
    OnMouseClick(&grid, game_area_rect, &player);
    BeginDrawing();
    ClearBackground(WHITE);
    DrawGameArea(game_area_rect);
    DrawTurns(&grid, game_area_rect);
    EndDrawing();
  }
  CloseWindow();
  return 0;
}