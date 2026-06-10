#include "client_net.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "winsock_compat.h"

#ifdef WIN32
WSADATA wsa;
#endif
static SOCKET sock;
static struct sockaddr_in server_adr;
// #ifdef WIN32
// static int slen = sizeof(client);
// #else
// static socklen_t slen = sizeof(server_adr);
// #endif

void ClientInit(const char* server_ip) {
#ifdef WIN32
  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
    printf("Client: WSAStartup failed: %d\n", WSAGetLastError());
    exit(1);
  }
#endif

  printf("Client: Creating Socket\n");
  // Create socket
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
    printf("Client: Socket creation failed: %d\n", WSAGetLastError());
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }

  // Setup client address
  memset(&server_adr, 0, sizeof(server_adr));
  server_adr.sin_family = AF_INET;
  server_adr.sin_port = htons(PORT);
  server_adr.sin_addr.s_addr = inet_addr(server_ip);

  if (connect(sock, (struct sockaddr*)&server_adr, sizeof(server_adr)) < 0) {
    printf("Client: connect() failed: %d\n", WSAGetLastError());
    exit(1);
  }
}

void Send(const char* msg, int size) {
  // Send message
  printf("sending messgage: %s\n", msg);
  if (send(sock, msg, size, 0) == SOCKET_ERROR) {
    printf("Client: sendto() failed: %d\n", WSAGetLastError());
    closesocket(sock);
#ifdef WIN32
    WSACleanup();
#endif
    exit(1);
  }
}

bool ClientReceive(char* buf) {
  // char* buf = malloc(BUFLEN);

  // printf("Client: Waiting for msg \n");

  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sock, &readfds);

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 0;  // non-blocking poll

  int select_rv = select(sock + 1, &readfds, NULL, NULL, &tv);

  if (select_rv < 0) {
    printf("Client: select() failed: %d\n", WSAGetLastError());
    return false;
  }

  if (select_rv > 0 && FD_ISSET(sock, &readfds)) {
    int bytes_received = recv(sock, buf, BUFLEN, 0);

    if (bytes_received < 0) {
      printf("Client: recv() failed: %d\n", WSAGetLastError());
      return false;
    }

    if (bytes_received > 0) {
      printf("Recieved packet\n");
      // for (int b = 0; b < bytes_received; b++) {
      //   printf(" %02x", buf[b]);
      // }
      return true;
    }
  }
  return false;
}

void ClientShutdown(void) {
  // Cleanup
  closesocket(sock);
#ifdef WIN32
  WSACleanup();
#endif
}
