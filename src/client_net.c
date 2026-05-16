#include "client_net.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

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
char buf[BUFLEN];

void ClientInit(void) {
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
  server_adr.sin_addr.s_addr = inet_addr(SERVER);

  if (connect(sock, (struct sockaddr*)&server_adr, sizeof(server_adr)) < 0) {
    perror("connect");
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

void ClientReceive(void) {
  printf("Client: Waiting for msg \n");
  while (true) {
    ssize_t bytes_received = recv(sock, buf, BUFLEN - 1, 0);
    if (bytes_received < 0) {
      perror("recv failed");
      exit(1);
    } else if (bytes_received > 0) {
      buf[bytes_received] = '\0';
      printf("Received: %s\n", buf);
      break;
    }
  }
}

void ClientShutdown(void) {
  // Cleanup
  closesocket(sock);
#ifdef WIN32
  WSACleanup();
#endif
}
