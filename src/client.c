#include <math.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>

#include "raygui.h"
#include "raymath.h"

#define ARRAY_LENGTH(x) ((int)(sizeof(x) / sizeof((x)[0])))

static const uint16_t cell_patterns[] = {7, 56, 448, 73, 146, 292, 273, 84};

typedef enum GameState { MENU, PLAYING, GAME_OVER, EXIT } GameState;

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

uint8_t CalcBigGridState(BigGrid* grid) {
  uint16_t player_pattern[2] = {0, 0};
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->grids[c].state == CELL_X) {
      player_pattern[0] += pow(2, c);
      player_count[0] += 1;
    } else if (grid->grids[c].state == CELL_O) {
      player_pattern[1] += pow(2, c);
      player_count[1] += 1;
    }
  }
  for (int player = 0; player < 2; player++) {
    for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
      if ((player_pattern[player] & cell_patterns[pat]) == cell_patterns[pat]) {
        return player + 1;
      }
    }
  }
  if (player_count[0] + player_count[1] == 9) {
    return CELL_DRAW;
  }
  return CELL_EMPTY;
}

void CalcSmallGridState(SmallGrid* grid) {
  uint16_t player_pattern[2] = {0, 0};
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->cells[c].state == CELL_X) {
      player_pattern[0] += pow(2, c);
      player_count[0] += 1;
    } else if (grid->cells[c].state == CELL_O) {
      player_pattern[1] += pow(2, c);
      player_count[1] += 1;
    }
  }
  for (int player = 0; player < 2; player++) {
    for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
      if ((player_pattern[player] & cell_patterns[pat]) == cell_patterns[pat]) {
        grid->state = player + 1;
      }
    }
  }
  if (grid->state == CELL_EMPTY && player_count[0] + player_count[1] == 9) {
    grid->state = CELL_DRAW;
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

void DrawGameArea(BigGrid* grid, Rectangle game_area_rect, int turn_area) {
  DrawTTTShape(game_area_rect);
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.width - big_grid_line_thickness * 2) / 3;
  for (int b_col = 0; b_col < 3; b_col++) {
    for (int b_row = 0; b_row < 3; b_row++) {
      Rectangle small_grid_rect =
          (Rectangle){game_area_rect.x + b_col * (small_grid_size + big_grid_line_thickness),
                      game_area_rect.y + b_row * (small_grid_size + big_grid_line_thickness),
                      small_grid_size, small_grid_size};
      if ((turn_area >= 0 && turn_area != b_row * 3 + b_col) ||
          (turn_area < 0 && grid->grids[b_row * 3 + b_col].state != CELL_EMPTY)) {
        DrawRectangleRec(small_grid_rect, ColorAlpha(BLACK, 0.2));
      }
      small_grid_rect = scale_rect(small_grid_rect, 0.9);
      DrawTTTShape(small_grid_rect);
      SmallGrid small_grid = grid->grids[b_row * 3 + b_col];
      if (small_grid.state == CELL_X) {
        DrawLineEx((Vector2){small_grid_rect.x, small_grid_rect.y},
                   (Vector2){small_grid_rect.x + small_grid_rect.width,
                             small_grid_rect.y + small_grid_rect.height},
                   small_grid_rect.height / 10, RED);
        DrawLineEx((Vector2){small_grid_rect.x, small_grid_rect.y + small_grid_rect.height},
                   (Vector2){small_grid_rect.x + small_grid_rect.width, small_grid_rect.y},
                   small_grid_rect.height / 10, RED);
      } else if (small_grid.state == CELL_O) {
        DrawRing((Vector2){small_grid_rect.x + small_grid_rect.width / 2,
                           small_grid_rect.y + small_grid_rect.height / 2},
                 small_grid_rect.width / 2 - big_grid_line_thickness, small_grid_rect.width / 2, 0,
                 360, 64, BLUE);
      }
    }
  }
}

void DetectClickInArea(BigGrid* grid, Rectangle game_area_rect, Vector2 selected_pos,
                       float big_grid_line_thickness, float small_grid_size,
                       float small_grid_line_thickness, float cell_size, int b_col, int b_row,
                       uint8_t* player, int* turn_area) {
  Rectangle small_grid_rect =
      scale_rect((Rectangle){game_area_rect.x + b_col * (small_grid_size + big_grid_line_thickness),
                             game_area_rect.y + b_row * (small_grid_size + big_grid_line_thickness),
                             small_grid_size, small_grid_size},
                 0.9);
  for (int s_col = 0; s_col < 3; s_col++) {
    for (int s_row = 0; s_row < 3; s_row++) {
      Rectangle cell_rect = {small_grid_rect.x + s_col * (cell_size + small_grid_line_thickness),
                             small_grid_rect.y + s_row * (cell_size + small_grid_line_thickness),
                             cell_size, cell_size};
      if (CheckCollisionPointRec(selected_pos, cell_rect)) {
        grid->grids[b_row * 3 + b_col].cells[s_row * 3 + s_col].state = *player + 1;
        *player = (*player + 1) % 2;
        if (grid->grids[b_row * 3 + b_col].state == CELL_EMPTY) {
          CalcSmallGridState(&grid->grids[b_row * 3 + b_col]);
        }
        if (grid->grids[s_row * 3 + s_col].state == CELL_EMPTY) {
          *turn_area = s_row * 3 + s_col;
        } else {
          *turn_area = -1;
        }
      }
    }
  }
}

void OnMouseClick(BigGrid* grid, Rectangle game_area_rect, uint8_t* player, int* turn_area) {
  if (!IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) return;
  Vector2 selected_pos =
      Vector2Subtract(GetMousePosition(), (Vector2){game_area_rect.x, game_area_rect.y});
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.width - big_grid_line_thickness * 2) / 3;
  float small_grid_line_thickness = small_grid_size * 0.9 / 50;
  float cell_size = (small_grid_size - small_grid_line_thickness * 2) / 3 * 0.9;
  if (*turn_area < 0) {
    for (int b_col = 0; b_col < 3; b_col++) {
      for (int b_row = 0; b_row < 3; b_row++) {
        if (grid->grids[b_row * 3 + b_col].state != CELL_EMPTY) {
          continue;
        }
        DetectClickInArea(grid, game_area_rect, selected_pos, big_grid_line_thickness,
                          small_grid_size, small_grid_line_thickness, cell_size, b_col, b_row,
                          player, turn_area);
      }
    }
  } else {
    int b_col = *turn_area % 3;
    int b_row = *turn_area / 3;
    DetectClickInArea(grid, game_area_rect, selected_pos, big_grid_line_thickness, small_grid_size,
                      small_grid_line_thickness, cell_size, b_col, b_row, player, turn_area);
  }
}

void DrawTurns(BigGrid* grid, Rectangle game_area_rect) {
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.width - big_grid_line_thickness * 2) / 3;
  float small_grid_line_thickness = small_grid_size / 50 * 0.9;
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
              DrawRing(
                  (Vector2){cell_rect.x + cell_rect.width / 2, cell_rect.y + cell_rect.height / 2},
                  cell_rect.width / 2 - shape_thickness, cell_rect.width / 2, 0, 360, 64, BLUE);
            }
          }
        }
      }
    }
  }
}

void RenderMenu(GameState* g_state, const Vector2 window_size) {
  float padding = window_size.x * 23 / 270;
  Vector2 button_size = {window_size.x - padding * 2, window_size.y * 5 / 36};

  if (GuiButton((Rectangle){padding, window_size.y * 109 / 360, button_size.x, button_size.y},
                "Play"))
    *g_state = PLAYING;
  if (GuiButton((Rectangle){padding, window_size.y * 109 / 360 + padding + button_size.y,
                            button_size.x, button_size.y},
                "Exit"))
    *g_state = EXIT;
}

int main(void) {
  InitWindow(800, 800, "MegaTicTacToe");
  Vector2 window_size = {GetMonitorHeight(GetCurrentMonitor()) * 0.8,
                         GetMonitorHeight(GetCurrentMonitor()) * 0.8};
  SetWindowSize(window_size.x, window_size.y);
  Rectangle game_area_rect = (Rectangle){0, 0, window_size.x, window_size.y};
  BigGrid grid = {0};
  uint8_t player = 0;
  int turn_area = -1;
  GameState game_state = MENU;
  SetTargetFPS(60);
  while (!WindowShouldClose() && game_state != EXIT) {
    switch (game_state) {
      case MENU:
        BeginDrawing();
        ClearBackground(BLACK);
        RenderMenu(&game_state, window_size);
        EndDrawing();
        break;
      case PLAYING:
        OnMouseClick(&grid, game_area_rect, &player, &turn_area);
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTurns(&grid, game_area_rect);
        DrawGameArea(&grid, game_area_rect, turn_area);
        EndDrawing();
        break;
      case GAME_OVER:
        BeginDrawing();
        const char* msg = CalcBigGridState(&grid) == CELL_X   ? "X wins!"
                          : CalcBigGridState(&grid) == CELL_O ? "O wins!"
                                                              : "";
        DrawText(msg, (window_size.x - MeasureText(msg, 60)) / 2, window_size.y / 2 - 30, 60, GOLD);
        EndDrawing();
        break;
      case EXIT:
        break;
    }
  }
  CloseWindow();
  return 0;
}