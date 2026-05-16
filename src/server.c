#include <bits/types.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer_control.h"
#include "common.h"
#include "winsock_compat.h"

#define SERVER "192.168.40.98"
#define PORT 5150

#define ARRAY_LENGTH(x) ((int)(sizeof(x) / sizeof((x)[0])))

static const uint16_t cell_patterns[] = {7, 56, 448, 73, 146, 292, 273, 84};

#define PLAYERS_MAX 2

#define BUFLEN 1024

typedef struct client {
  struct sockaddr_in addr;
  bool active;
} client;

static client clients[PLAYERS_MAX] = {0};
static SOCKET sock;
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
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock == INVALID_SOCKET) {
    printf("Server: Socket creation failed: %d\n", WSAGetLastError());
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }

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
  if (bind(sock, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
    printf("Server: Bind failed: %d\n", WSAGetLastError());
    closesocket(sock);
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }

  printf("Server: Waiting for UDP packet on port %s:%d...\n", ip_str, PORT);
}

void ServerShutdown(void) {
  closesocket(sock);
#ifdef WIN32
  WSACleanup();
#endif
}

void Broadcast(char* buf, int size) {
  for (signed char i = 0; i < PLAYERS_MAX; i++) {
    if (!clients[i].active) continue;
    memcpy(buf, &i, 1);
    if (sendto(sock, buf, size, 0, (struct sockaddr*)&clients[i].addr, slen) == SOCKET_ERROR) {
      printf("Server: sendto() failed: %d\n", WSAGetLastError());
    } else {
      printf("Server: Response sent to client.\n");
    }
  }
}

void RecordClient(struct sockaddr_in* cl) {
  printf("Server: Recording client:\n");
  bool found = false;
  int slot = -1;
  for (int i = 0; i < PLAYERS_MAX; i++) {
    if (!clients[i].active) {
      if (slot == -1) {
        slot = i;
      }
      continue;
    }
    if (clients[i].addr.sin_addr.s_addr == cl->sin_addr.s_addr &&
        clients[i].addr.sin_port == cl->sin_port) {
      found = true;
      break;
    }
  }
  if (!found && slot != -1) {
    clients[slot].addr.sin_addr.s_addr = cl->sin_addr.s_addr;
    clients[slot].addr.sin_port = cl->sin_port;
    clients[slot].addr.sin_family = cl->sin_family;
    clients[slot].active = true;
  }
  printf("Server: Received packet from %s:%d\n", inet_ntoa(cl->sin_addr), ntohs(cl->sin_port));
}

void Receive(void) {
  struct sockaddr_in cl;
  // Receive a packet
  int recv_len = recvfrom(sock, buf, BUFLEN - 1, 0, (struct sockaddr*)&cl, &slen);
  if (recv_len == SOCKET_ERROR) {
    printf("1Server: recvfrom() failed: %d\n", WSAGetLastError());
    closesocket(sock);
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }
  RecordClient(&cl);
}

int ReceiveMultiple(void) {
#define SILENCE_TIMEOUT_MS 1
  int ret;
  struct sockaddr_in cl;
  int packets = 0;
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  // Set timeout
  struct timeval timeout;
  timeout.tv_sec = 0;
  timeout.tv_usec = SILENCE_TIMEOUT_MS * 1000;  // convert ms to µs

  // Collect packets until no activity for SILENCE_TIMEOUT_MS
  while (1) {
    ret = select(sock + 1, &readfds, NULL, NULL, &timeout);
    if (ret == SOCKET_ERROR) {
      printf("select() failed: %d\n", WSAGetLastError());
      exit(1);
    } else if (ret == 0) {
      // No data received in timeout
      // printf("Silence timeout reached (%d ms)\n", SILENCE_TIMEOUT_MS);
      return packets;
    }
    printf("select() returned = %d\n", ret);

    if (FD_ISSET(sock, &readfds)) {
      int bytesReceived = recvfrom(sock, buf, BUFLEN - 1, 0, (struct sockaddr*)&cl, &slen);
      if (bytesReceived == SOCKET_ERROR) {
        printf("2Server: recvfrom() failed: %d\n", WSAGetLastError());
        closesocket(sock);
#ifdef WIN32
        WSACleanup();
#endif
        exit(1);
      }
      RecordClient(&cl);
      StoreInputs(buf);
      packets++;
    }
  }
  return packets;
}

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

int main() {

};