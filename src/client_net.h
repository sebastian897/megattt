#pragma once
#include <stdbool.h>

#include "common.h"

void ClientInit(const char* server_ip);
void Send(const char* buf, int size);
bool ClientReceive(char* buf);
void ClientShutdown(void);
