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
