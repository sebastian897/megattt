#pragma once
#include <stdbool.h>

#include "common.h"

void ClientInit(void);
void Send(const char* buf, int size);
bool ClientReceive(char* buf);
void ClientShutdown(void);
