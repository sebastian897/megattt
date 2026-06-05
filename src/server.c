#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "buffer_control.h"
#include "common.h"
#include "winsock_compat.h"

#define ARRAY_LENGTH(x) ((int)(sizeof(x) / sizeof((x)[0])))

static const uint16_t cell_patterns[] = {7, 56, 448, 73, 146, 292, 273, 84};

#define MAX_CLIENTS 128

// static client clients[PLAYERS_MAX] = {0};
static SOCKET server_sock;
// #ifdef WIN32
// static int slen = sizeof(struct sockaddr_in);
// #else
// static socklen_t slen = sizeof(struct sockaddr_in);
// #endif

char buf[BUFLEN];

void ServerInit(void) {
  srand(time(NULL));

  struct sockaddr_in server;
#ifdef WIN32
  WSADATA wsa;
#endif

#ifdef WIN32
  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    printf("Server: WSAStartup failed: %d\n", WSAGetLastError());
    exit(1);
  }
#endif

  // Create UDP socket
  server_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (server_sock == INVALID_SOCKET) {
    printf("Server: Socket creation failed: %d\n", WSAGetLastError());
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }

  int opt = 1;
  if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    closesocket(server_sock);
    exit(EXIT_FAILURE);
  }

  // Prepare server address
  server.sin_family = AF_INET;
  const char* ip_str = SERVER_BIND;
  if (inet_pton(AF_INET, ip_str, &server.sin_addr.s_addr) != 1) {
    printf("Server: inet_pton failed: %d\n", WSAGetLastError());
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }
  server.sin_port = htons(PORT);

  // Bind
  if (bind(server_sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
    printf("Server: Bind failed: %d\n", WSAGetLastError());
    closesocket(server_sock);
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }

  printf("Server: Waiting for TCP packet on port %s:%d...\n", ip_str, PORT);

  // Start listening
  if (listen(server_sock, 100) < 0) {
    perror("listen failed");
    closesocket(server_sock);
    exit(EXIT_FAILURE);
  }
}

void ServerShutdown(void) {
  closesocket(server_sock);
#ifdef WIN32
  WSACleanup();
#endif
}

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
  uint16_t player_pattern[2] = {0, 0};
  uint8_t player_count[2] = {0};
  for (int c = 0; c < 9; c++) {
    if (grid->cells[c].state == CELL_X) {  // idx into with state
      player_pattern[0] += 1 << c;
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

typedef enum client_state {
  CS_EMPTY,
  CS_POOL,
  CS_PLAYING,
} client_state;

typedef struct Game Game;

typedef struct client {
  SOCKET sock;
  char name[16];
  client_state state;
  Game* game;
} client;

typedef struct Game {
  BigGrid grid;
  char code[5];
  int turn_area;
  uint8_t turn;
  client* clients[PLAYERS_MAX];
} Game;

void CalcPlayerMove(Game* game, PlayerMove move) {
  game->grid.grids[move.big_grid_idx].cells[move.small_grid_idx].state = game->turn + 1;

  CalcSmallGridState(&game->grid.grids[move.big_grid_idx]);
  CalcBigGridState(&game->grid);

  for (int sg = 0; sg < 9; sg++) {
    printf("%d ", game->grid.grids[sg].state);
  }
  printf("\n");

  if (game->grid.state > 0) {
    printf("Winner = %d\n", game->grid.state);
    for (int c_idx = 0; c_idx < PLAYERS_MAX; c_idx++) {
      game->clients[c_idx]->state = CS_EMPTY;
      game->clients[c_idx]->game = NULL;
    }
  }

  game->turn = (game->turn + 1) % 2;
  if (game->grid.grids[move.small_grid_idx].state == CELL_EMPTY) {
    game->turn_area = move.small_grid_idx;
  } else {
    game->turn_area = -1;
  }
}

game_packet MakePacket(Game* game) {
  return (game_packet){PT_GAME_DATA, sizeof(game_packet), game->grid, game->turn_area, 0};
}

void AddToNewGame(Game games[MAX_CLIENTS / PLAYERS_MAX], client* cl) {
  int first_empty_game = -1;
  for (int g = 0; g < MAX_CLIENTS / PLAYERS_MAX; g++) {
    bool full = true;
    bool empty = true;
    int first_empty_client_slot = -1;
    for (int cl_idx = 0; cl_idx < PLAYERS_MAX; cl_idx++) {
      if (games[g].clients[cl_idx] == NULL) {
        full = false;
        if (first_empty_client_slot < 0) {
          first_empty_client_slot = cl_idx;
        }
      } else {
        empty = false;
      }
    }
    if (!full) {
      if (!empty) {
        printf("Server: Added player to not emtpy game, idx = %d\n", g);
        games[g].clients[first_empty_client_slot] = cl;
        cl->game = &games[g];
        return;
      }
      if (first_empty_game < 0) {
        first_empty_game = g;
      }
    }
  }
  if (first_empty_game < 0) {
    perror("All games are full");
  } else {
    printf("Server: Added player to new game, idx = %d\n", first_empty_game);
    memset(&games[first_empty_game], 0, sizeof(Game));
    games[first_empty_game].clients[0] = cl;
    cl->game = &games[first_empty_game];
  }
  return;
}

bool IsGameFull(Game* game) {
  for (int cl = 0; cl < PLAYERS_MAX; cl++) {
    if (game->clients[cl] == NULL) {
      return false;
    }
  }
  return true;
}

void RemoveClFromItsGame(client* cl) {
  if (cl == NULL) return;
  for (int pl = 0; pl < PLAYERS_MAX; pl++) {
    if (cl->game != NULL && cl->game->clients[pl] == cl) {
      cl->game->clients[pl] = NULL;
      return;
    };
  }
}

void ShuffleClients(Game* game) {
  uint8_t rnd_player = rand() & 1;
  if (rnd_player == 1) {
    client* temp = game->clients[1];
    game->clients[1] = game->clients[0];
    game->clients[0] = temp;
  }
}

int main() {
  ServerInit();
  client clients[MAX_CLIENTS] = {0};
  fd_set readfds;
  SOCKET max_sock;
  int activity;
  socklen_t client_len;
  struct sockaddr_in client_addr;
  Game games[MAX_CLIENTS / PLAYERS_MAX] = {0};
  while (1) {
    FD_ZERO(&readfds);

    // Add server socket
    FD_SET(server_sock, &readfds);
    max_sock = server_sock;

    // Add client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
      SOCKET client_sock = clients[i].sock;

      if (client_sock > 0) FD_SET(client_sock, &readfds);

      if (client_sock > max_sock) max_sock = client_sock;
    }

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    activity = select(max_sock + 1, &readfds, NULL, NULL, &tv);

#ifdef WIN32
#define eintr WSAEINTR
#else
#define eintr EINTR
#endif
    if (activity < 0 && WSAGetLastError() != eintr) {
      perror("select");
    }

    // New connection
    if (FD_ISSET(server_sock, &readfds)) {
      client_len = sizeof(client_addr);

      SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);

      // if (client_sock < 0) {
      //   perror("accept");
      //   continue;
      // }

      printf("New connection: socket fd %d, IP %s, port %d\n", (int)client_sock,
             inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

      // Store client socket
      for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
          clients[i].sock = client_sock;
          break;
        }
      }
    }

    // Handle client activity
    for (int c = 0; c < MAX_CLIENTS; c++) {
      client* cl = &clients[c];
      Game* game = cl->game;

      if (cl->state == CS_POOL) {
        if (IsGameFull(game)) {
          ShuffleClients(game);

          char send_buf[BUFLEN] = {0};
          game_packet packet = {0};
          packet.type = PT_GAME_DATA;
          packet.turn_area = -1;  // can place anywhere at the start

          for (int c_idx = 0; c_idx < PLAYERS_MAX; c_idx++) {
            packet.turn = game->turn ^ c_idx;
            memcpy(&send_buf, &packet, sizeof(packet));

            printf("send connecting packet to FD: %d\n", (int)game->clients[c_idx]->sock);

            send(game->clients[c_idx]->sock, send_buf, sizeof(packet), 0);
            game->clients[c_idx]->state = CS_PLAYING;
          }
        }
      }

      if (FD_ISSET(cl->sock, &readfds)) {
        int bytes_read = recv(cl->sock, buf, BUFLEN - 1, 0);

        // Client disconnected
        if (bytes_read <= 0) {
          printf("Client disconnected: FD %d Bytes Read: %d\n", (int)cl->sock, bytes_read);

          for (int c_idx = 0; c_idx < PLAYERS_MAX; c_idx++) {
            closesocket(game->clients[c_idx]->sock);
            memset(game->clients[c_idx], 0, sizeof(*cl));
          }
        } else {
          printf("Received from fd %d: ", (int)cl->sock);
          // for (int b = 0; b < bytes_read; b++) printf(" %02x", buf[b]);
          printf("\n");
          switch (cl->state) {
            case CS_EMPTY:
              connect_packet c_packet;
              memcpy(&c_packet, buf, sizeof(c_packet));
              if (c_packet.password == PASSWORD) {
                strcpy(cl->name, c_packet.name);
                cl->state = CS_POOL;
                AddToNewGame(games, cl);
              } else {
                printf("Password didnt match IP: %d", (int)cl->sock);
                closesocket(cl->sock);
              }
              break;
            case CS_POOL:
              // player sending when in pool?
              break;
            case CS_PLAYING:
              PlayerMove move;
              memcpy(&move, buf, sizeof(move));
              printf("Server: Recieved player move from FD: %d B: %d, S: %d\n", (int)cl->sock,
                     move.big_grid_idx, move.small_grid_idx);
              CalcPlayerMove(game, move);
              // player move;
              printf("Server: Sending game packet to game clients: ");
              char send_buf[BUFLEN] = {0};
              game_packet g_packet = MakePacket(game);
              for (int c_idx = 0; c_idx < PLAYERS_MAX; c_idx++) {
                g_packet.turn = game->turn ^ c_idx;
                memcpy(&send_buf, &g_packet, sizeof(g_packet));
                if (send(game->clients[c_idx]->sock, send_buf, sizeof(g_packet), 0) < 0) {
                  perror("Send failed");
                };
              }
              break;
          }
        }
      }
    }
  }
  ServerShutdown();
};