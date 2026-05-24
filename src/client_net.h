#pragma once
#include "common.h"

void ClientInit(void);
void Send(const char* buf, int size);
char * ClientReceive(void);
void ClientShutdown(void);
