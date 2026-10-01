#include "solver.h"

#include "common.h"
#include "stdint.h"

static const uint16_t cell_patterns[] = {7, 56, 448, 73, 146, 292, 273, 84};
static const uint16_t cell_values[] = {3, 2, 3, 2, 5, 2, 3, 2, 3};
static BigGrid Future_Grid = {0};

void CalcBigGridState(BigGrid* grid) {
  uint16_t player_pattern[2] = {0, 0};
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->grids[c].state == CELL_X) {
      player_pattern[0] += 1 << c;
      player_count[0] += 1;
    } else if (grid->grids[c].state == CELL_O) {
      player_pattern[1] += 1 << c;
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
  if (player_count[0] + player_count[1] == 9) {
    grid->state = CELL_DRAW;
  }
}

void CalcSmallGridState(SmallGrid* grid) {
  uint16_t player_pattern[2] = {0, 0};  // pattern of winning solutions
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->cells[c].state == CELL_X) {  // idx into with state
      player_pattern[0] += 1 << c;         // bitwise operation
      player_count[0] += 1;
    } else if (grid->cells[c].state == CELL_O) {
      player_pattern[1] += 1 << c;
      player_count[1] += 1;
    }
  }
  for (int player = 0; player < 2; player++) {
    for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
      if (grid->state == CELL_EMPTY &&
          (player_pattern[player] & cell_patterns[pat]) == cell_patterns[pat]) {
        grid->state = player + 1;
      }
    }
  }
  if (grid->state == CELL_EMPTY && player_count[0] + player_count[1] == 9) {
    grid->state = CELL_DRAW;
  }
}

void MoveSolver(
    BigGrid* grid,
    int turn_area) {  // note to self: needs to iterate from future board table rather
                      // than active table, maybe make function to capture state of game, also move
                      // the futuregrid into this function, and use it as an array of grids, each
                      // grid reperesenting a layer of depth (array[L]) maybe? future grid values
                      // that need to change are only within the turn area
  int layer_depth = 4;  // iteration begins at layer 0, 1 layer = 2 iterations
  uint16_t player_pattern = 0;
  for (int d = 0; d < 1; d++) {  // d=depth (0 = smallgrid, 1 = biggrid)
    if (turn_area != 0) {
      if (d == 0) {
        for (int c = 0; c < 9; c++) {
          if (grid->grids[turn_area].cells[c].state == CELL_EMPTY) {
            player_pattern += 1 << c;
            for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
              if ((player_pattern & cell_patterns[pat]) == cell_patterns[pat]) {
                Future_Grid.grids[turn_area].cells[c].FutureValue += 1;
              }
            }
          } else if (grid->grids[turn_area].cells[c].state == CELL_X) {
            Future_Grid.grids[turn_area].cells[c].state = CELL_X;
          }
        }
      } else {
        for (int c = 0; c < 9; c++) {
          if (grid->grids[c].state == CELL_EMPTY) {
            player_pattern += 1 << c;
            for (int pat = 0; pat < ARRAY_LENGTH(cell_patterns); pat++) {
              if ((player_pattern & cell_patterns[pat]) == cell_patterns[pat]) {
                Future_Grid.grids[c].FutureValue += 1;
              }
            }
          } else if (grid->grids[c].state == CELL_X) {
            Future_Grid.grids[c].state = CELL_X;
          }
        }
      }
    }
  }
}