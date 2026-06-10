#include <inttypes.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "client_net.h"
#include "common.h"
#include "raygui.h"
#include "raymath.h"

char server_ip[20] = SERVER;

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
        DrawRectangleRec(small_grid_rect, ColorAlpha(BLACK, 0.35));
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

bool DetectClickInArea(BigGrid* grid, Rectangle game_area_rect, Vector2 selected_pos,
                       float big_grid_line_thickness, float small_grid_size,
                       float small_grid_line_thickness, float cell_size, int b_col, int b_row) {
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
        if (grid->grids[b_row * 3 + b_col].state == CELL_EMPTY) {
          PlayerMove move = {b_row * 3 + b_col, s_row * 3 + s_col};
          char send_buf[BUFLEN] = {0};
          memcpy(send_buf, &move, sizeof(move));
          Send(send_buf, sizeof(move));
          return true;
        }
      }
    }
  }
  return false;
}

void OnMouseClick(BigGrid* grid, Rectangle game_area_rect, int* turn_area) {
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
        if (DetectClickInArea(grid, game_area_rect, selected_pos, big_grid_line_thickness,
                              small_grid_size, small_grid_line_thickness, cell_size, b_col, b_row))
          return;
      }
    }
  } else {
    int b_col = *turn_area % 3;
    int b_row = *turn_area / 3;
    if (DetectClickInArea(grid, game_area_rect, selected_pos, big_grid_line_thickness,
                          small_grid_size, small_grid_line_thickness, cell_size, b_col, b_row))
      return;
  }
}

void DrawTurns(BigGrid* grid, Rectangle game_area_rect) {
  float big_grid_line_thickness = game_area_rect.height / 50;
  float small_grid_size = (game_area_rect.height - big_grid_line_thickness * 2) / 3;
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

void DrawIndicator(bool turn, Vector2 window_size) {
  if (!turn) return;
  float padding = window_size.y * 0.005;
  float radius = window_size.y * 0.01;
  DrawCircle(window_size.x - radius - padding, 0 + radius + padding, radius, ColorAlpha(RED, 0.5));
}

void RenderMenu(PlayerState* g_state, const Vector2 window_size) {
  float padding = window_size.x * 23 / 270;
  Vector2 button_size = {window_size.x - padding * 2, window_size.y * 5 / 36};

  if (GuiButton((Rectangle){padding, window_size.y * 109 / 360, button_size.x, button_size.y},
                "Play")) {
    connect_packet packet = {PASSWORD, "Seb"};
    char buf[BUFLEN];
    memcpy(buf, &packet, sizeof(packet));
    Send(buf, sizeof(packet));

    *g_state = CONNECTING;
  }
  if (GuiButton((Rectangle){padding, window_size.y * 109 / 360 + padding + button_size.y,
                            button_size.x, button_size.y},
                "Exit"))
    *g_state = EXIT;
}

void RenderGameOver(Vector2 window_size, CellState winner, PlayerState* game_state) {
  Vector2 pop_up_size = Vector2Scale(window_size, 0.40);
  Rectangle pop_up = {(window_size.x - pop_up_size.x) / 2, (window_size.y - pop_up_size.y) / 2,
                      pop_up_size.x, pop_up_size.y};
  DrawRectangleRec(pop_up, GRAY);

  const char* msg = winner == CELL_X ? "X wins!" : winner == CELL_O ? "O wins!" : "Draw!";
  DrawText(msg, (window_size.x - MeasureText(msg, 60)) / 2, window_size.y / 2 - 30, 60, WHITE);

  Vector2 menu_button_size = {pop_up.width / 3, pop_up.height / 5};
  float menu_button_padding = pop_up_size.y * 0.05;
  Rectangle menu_button = {pop_up.x + (pop_up.width - menu_button_size.x) / 2,
                           pop_up.y + pop_up.height - menu_button_size.y - menu_button_padding,
                           menu_button_size.x, menu_button_size.y};
  if (GuiButton(menu_button, "Menu?")) *game_state = MENU;
}

void RenderConnecting(Vector2 window_size) {
  const char* msg = "Finding Game...";
  DrawText(msg, (window_size.x - MeasureText(msg, 60)) / 2, window_size.y / 2 - 30, 60, WHITE);
}

void DrawMenuPopUpButton(Vector2 window_size, bool* menu_active) {
  Vector2 pop_up_button_size = {window_size.y * 0.05, window_size.y * 0.05};
  Rectangle pop_up_button = {window_size.x - pop_up_button_size.x,
                             window_size.y - pop_up_button_size.y, pop_up_button_size.x,
                             pop_up_button_size.y};
  if (GuiButton(pop_up_button, "M")) {
    *menu_active = !*menu_active;
  }
}

void HandlePacketData(PlayerState* game_state, BigGrid* grid, int* turn_area, bool* turn) {
  char rec_buf[BUFLEN];
  if (ClientReceive(rec_buf)) {
    game_packet packet = {0};
    memcpy(&packet, rec_buf, sizeof(packet));
    if (packet.type == PT_GAME_DATA) {
      *game_state = PLAYING;
      *grid = packet.grid;
      *turn_area = packet.turn_area;
      *turn = packet.turn;
      // printf("Connecting: Turn = %d\n", *turn);
    }
  }
}

int main(int argc, char* argv[]) {
  int opt;

  while ((opt = getopt(argc, argv, "s:")) != -1) {
    switch (opt) {
      case 's':
        strncpy(server_ip, optarg, sizeof(server_ip) - 1);
        server_ip[sizeof(server_ip) - 1] = '\0';  // Ensure null termination
        printf("server_ip = %s\n", server_ip);
        break;
    }
  }
  // exit(0);

  ClientInit(server_ip);

  // exit(0);
  InitWindow(800, 800, "MegaTicTacToe");
  Vector2 window_size = {GetMonitorHeight(GetCurrentMonitor()) * 0.8,
                         GetMonitorHeight(GetCurrentMonitor()) * 0.8};
  SetWindowSize(window_size.x, window_size.y);
  Rectangle game_area_rect = (Rectangle){0, 0, window_size.x, window_size.y};
  BigGrid grid = {0};
  bool turn;
  int turn_area = -1;
  PlayerState game_state = MENU;
  SetTargetFPS(60);
  bool menu_active = false;
  while (!WindowShouldClose() && game_state != EXIT) {
    switch (game_state) {
      case MENU:
        BeginDrawing();
        ClearBackground(BLACK);
        RenderMenu(&game_state, window_size);
        EndDrawing();
        break;
      case CONNECTING:
        BeginDrawing();
        ClearBackground(BLACK);
        RenderConnecting(window_size);
        EndDrawing();
        HandlePacketData(&game_state, &grid, &turn_area, &turn);
        break;
      case PLAYING:
        if (turn) OnMouseClick(&grid, game_area_rect, &turn_area);
        BeginDrawing();
        ClearBackground(WHITE);
        DrawTurns(&grid, game_area_rect);
        DrawGameArea(&grid, game_area_rect, turn_area);
        DrawIndicator(turn, window_size);
        EndDrawing();
        HandlePacketData(&game_state, &grid, &turn_area, &turn);
        if (grid.state > 0) {
          game_state = GAME_OVER;
          menu_active = true;
        }
        break;
      case GAME_OVER:
        ClearBackground(WHITE);
        BeginDrawing();
        DrawTurns(&grid, game_area_rect);
        DrawGameArea(&grid, game_area_rect, turn_area);
        DrawMenuPopUpButton(window_size, &menu_active);
        if (menu_active) RenderGameOver(window_size, grid.state, &game_state);
        EndDrawing();
        break;
      case EXIT:
        break;
    }
  }
  CloseWindow();
  return 0;
}