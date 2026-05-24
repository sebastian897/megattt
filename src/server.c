// #include "server.h"

#include <arpa/inet.h>
#include <bits/types.h>
#include <bits/types/struct_timeval.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "buffer_control.h"
#include "common.h"
#include "winsock_compat.h"

#define ARRAY_LENGTH(x) ((int)(sizeof(x) / sizeof((x)[0])))

static const uint16_t cell_patterns[] = {7, 56, 448, 73, 146, 292, 273, 84};

#define MAX_CLIENTS 128

// typedef struct client {
//   struct sockaddr_in addr;
//   bool active;
// } client;

// static client clients[PLAYERS_MAX] = {0};
static SOCKET server_sock;
#ifdef WIN32
static int slen = sizeof(struct sockaddr_in);
#else
static socklen_t slen = sizeof(struct sockaddr_in);
#endif

char buf[BUFLEN];

void ServerInit(void) {
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

  // int opt = 1;
  // if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
  //   perror("setsockopt");
  //   close(server_sock);
  //   exit(EXIT_FAILURE);
  // }

  // Prepare server address
  server.sin_family = AF_INET;
  const char* ip_str = SERVER;
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
    close(server_sock);
    exit(EXIT_FAILURE);
  }
}

void ServerShutdown(void) {
  closesocket(server_sock);
#ifdef WIN32
  WSACleanup();
#endif
}

void Accept(void) {
  // Accept one client connection
  struct sockaddr_in address;
  socklen_t addrlen = sizeof(address);
  int client_fd = accept(server_sock, (struct sockaddr*)&address, &addrlen);
  if (client_fd < 0) {
    perror("accept failed");
    close(server_sock);
    exit(EXIT_FAILURE);
  }

  char addr_buf[50];
  inet_ntop(AF_INET, &address.sin_addr.s_addr, addr_buf, 50);
  printf("Client connected from %s\n", addr_buf);
  // Receive data
  ssize_t bytes_received = recv(client_fd, buf, BUFLEN - 1, 0);
  if (bytes_received < 0) {
    perror("recv failed");
  } else {
    buf[bytes_received] = '\0';
    printf("Received: %s\n", buf);
  }
  ssize_t err = send(client_fd, "oi oi", 6, 0);
  printf("Server: Error from bytes sent: %ld", err);
  if (err == SOCKET_ERROR) {
    perror("server: send failed");
    exit(1);
  }
}

// void Broadcast(char* buf, int size) {
//   for (signed char i = 0; i < PLAYERS_MAX; i++) {
//     if (!clients[i].active) continue;
//     memcpy(buf, &i, 1);
//     if (sendto(server_sock, buf, size, 0, (struct sockaddr*)&clients[i].addr, slen) ==
//         SOCKET_ERROR) {
//       printf("Server: sendto() failed: %d\n", WSAGetLastError());
//     } else {
//       printf("Server: Response sent to client.\n");
//     }
//   }
// }

// void RecordClient(struct sockaddr_in* cl) {
//   printf("Server: Recording client:\n");
//   bool found = false;
//   int slot = -1;
//   for (int i = 0; i < PLAYERS_MAX; i++) {
//     if (!clients[i].active) {
//       if (slot == -1) {
//         slot = i;
//       }
//       continue;
//     }
//     if (clients[i].addr.sin_addr.s_addr == cl->sin_addr.s_addr &&
//         clients[i].addr.sin_port == cl->sin_port) {
//       found = true;
//       break;
//     }
//   }
//   if (!found && slot != -1) {
//     clients[slot].addr.sin_addr.s_addr = cl->sin_addr.s_addr;
//     clients[slot].addr.sin_port = cl->sin_port;
//     clients[slot].addr.sin_family = cl->sin_family;
//     clients[slot].active = true;
//   }
//   printf("Server: Received packet from %s:%d\n", inet_ntoa(cl->sin_addr), ntohs(cl->sin_port));
// }

// void Receive(void) {
//   struct sockaddr_in cl;
//   // Receive a packet
//   int recv_len = recvfrom(server_sock, buf, BUFLEN - 1, 0, (struct sockaddr*)&cl, &slen);
//   if (recv_len == SOCKET_ERROR) {
//     printf("1Server: recvfrom() failed: %d\n", WSAGetLastError());
//     closesocket(server_sock);
// #ifdef WIN32
//     WSACleanup();
// #endif
//     exit(1);
//   }
//   RecordClient(&cl);
// }

// int ReceiveMultiple(void) {
// #define SILENCE_TIMEOUT_MS 1
//   int ret;
//   struct sockaddr_in cl;
//   int packets = 0;
//   fd_set readfds;
//   FD_ZERO(&readfds);
//   FD_SET(server_sock, &readfds);

//   // Set timeout
//   struct timeval timeout;
//   timeout.tv_sec = 0;
//   timeout.tv_usec = SILENCE_TIMEOUT_MS * 1000;  // NOLINT convert ms to µs

//   // Collect packets until no activity for SILENCE_TIMEOUT_MS
//   while (1) {
//     ret = select(server_sock + 1, &readfds, NULL, NULL, &timeout);
//     if (ret == SOCKET_ERROR) {
//       printf("select() failed: %d\n", WSAGetLastError());
//       exit(1);
//     } else if (ret == 0) {
//       // No data received in timeout
//       // printf("Silence timeout reached (%d ms)\n", SILENCE_TIMEOUT_MS);
//       return packets;
//     }
//     printf("select() returned = %d\n", ret);

//     if (FD_ISSET(server_sock, &readfds)) {
//       int bytesReceived = recvfrom(server_sock, buf, BUFLEN - 1, 0, (struct sockaddr*)&cl,
//       &slen); if (bytesReceived == SOCKET_ERROR) {
//         printf("2Server: recvfrom() failed: %d\n", WSAGetLastError());
//         closesocket(server_sock);
// #ifdef WIN32
//         WSACleanup();
// #endif
//         exit(1);
//       }
//       RecordClient(&cl);
//       StoreInputs(buf);
//       packets++;
//     }
//   }
//   return packets;
// }

uint8_t CalcBigGridState(BigGrid* grid) {
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
      if ((player_pattern[player] & cell_patterns[pat]) == cell_patterns[pat]) {
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

typedef struct client {
  SOCKET sock;
  char name[16];
  client_state state;
} client;

typedef struct Game {
  BigGrid grid;
  char code[5];
  uint8_t turn;
  client clients[MAX_CLIENTS / PLAYERS_MAX];
} Game;

void CalcPlayerMove(Game* game, PlayerMove move) {
  game->grid.grids[move.big_grid_idx].cells[move.small_grid_idx].state = game->turn + 1;
  game->turn = (game->turn + 1) % 2;
}

game_packet MakePacket(Game* game) {
  return (game_packet){PT_GAME_DATA, game->grid, game->turn};
}

void ClearPriorityArr(int* arr[MAX_CLIENTS/PLAYERS_MAX]) {
  for (int i = 0; i < MAX_CLIENTS/PLAYERS_MAX; i++) {
    *arr[i] = -1;
  }
}

int FindNextAvailableIdx(int arr[MAX_CLIENTS/PLAYERS_MAX]) {
  for (int i = 0; i < MAX_CLIENTS/PLAYERS_MAX; i++) {
    if (arr[i] < 0) {
      return i;
    }
    return -1;
  }
}

void AddToNewGame(Game* games[MAX_CLIENTS/PLAYERS_MAX], client cl) {
  int first_empty_game = -1;
  for (int g =0; g<MAX_CLIENTS/PLAYERS_MAX; g++) {
    bool full = true;
    bool not_empty = false;
    int first_empty_client_slot = -1;
    for (int cl_idx = 0; cl_idx < PLAYERS_MAX; cl_idx++) {
      if (games[g]->clients[cl_idx].name == "") {
        full = false;
        if (first_empty_client_slot < 0) {
          first_empty_client_slot = cl_idx;
        }
      } else {
        not_empty = true;
      }
    }
    if (!full) {
      if (!not_empty) {
        games[g]->clients[first_empty_client_slot] = cl;
        return;
      }
      if (first_empty_game <0 ){
        first_empty_game = g;
      }
    }
  }
  if (first_empty_game < 0) {
    perror("All games are full");
  } else {
    games[first_empty_game]->clients[0] = cl;
  }
  return;
}

bool IsGameFull(Game* game) {
  for (int cl = 0; cl < PLAYERS_MAX; cl++) {
    if (game->clients[cl].name == "") {
      return false;
    }
  }
  return true;
}

int main() {
  ServerInit();
  client clients[MAX_CLIENTS] = {0};
  fd_set readfds;
  int max_sock, activity;
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

    // Wait for activity
    // printf("Server: Selecting\n");
    activity = select(max_sock + 1, &readfds, NULL, NULL, NULL);
    // printf("Server: Selection ended\n");

    if (activity < 0 && errno != EINTR) {
      perror("select");
    }

    // New connection
    if (FD_ISSET(server_sock, &readfds)) {
      client_len = sizeof(client_addr);

      SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);

      if (client_sock < 0) {
        perror("accept");
        continue;
      }

      printf("New connection: socket fd %d, IP %s, port %d\n", client_sock,
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
    for (int g = 0; g < MAX_CLIENTS / PLAYERS_MAX; g++) {
      for (int c = 0; c < PLAYERS_MAX; c++) {
        Game* game = &games[g];
        client* cl = &game->clients[c];

        if (cl->state == CS_POOL) {
          if (IsGameFull(game)) {
            printf("Server: Sending connecting packet");
            char send_buf[BUFLEN] = {0};
            game_packet packet = {0};
            packet.type = PT_CONNECT;
            memcpy(&send_buf, &packet, sizeof(packet));
            for (int c_idx = 0; c_idx < PLAYERS_MAX; c_idx++) {
              send(game->clients[c_idx].sock, send_buf, sizeof(packet), 0);
            }
          }
        }

        if (FD_ISSET(cl->sock, &readfds)) {
          int bytes_read = read(cl->sock, buf, BUFLEN - 1);

          // Client disconnected
          if (bytes_read <= 0) {
            getpeername(cl->sock, (struct sockaddr*)&client_addr, &client_len);

            printf("Client disconnected: IP %s, port %d\n", inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));

            close(cl->sock);
            memset(cl, 0, sizeof(*cl));
          } else {
            buf[bytes_read] = '\0';
            switch (cl->state) {
              case CS_EMPTY:
                printf("Server: Added player to new game");
                strncpy(buf, cl->name, sizeof(cl->name));
                cl->state = CS_POOL;
                AddToNewGame(&game, *cl);
                break;
              case CS_POOL:
                // player sending when in pool?
                break;
              case CS_PLAYING:
                printf("Server: Recieved player move");
                PlayerMove move;
                memcpy(&move, buf, sizeof(move));
                CalcPlayerMove(game, move);
                // player move;
                printf("Server: Sending game packet to game clients");
                char send_buf[BUFLEN] = {0};
                game_packet packet = MakePacket(game);
                memcpy(&send_buf, &packet, sizeof(packet));
                for (int c_idx = 0; c_idx < PLAYERS_MAX; c_idx++) {
                  send(game->clients[c_idx].sock, send_buf, sizeof(packet), 0);
                }
                break;
            }
            printf("Received from fd %d: %s\n", cl->sock, buf);
          }
        }
      }
    }
  }
  close(server_sock);
};