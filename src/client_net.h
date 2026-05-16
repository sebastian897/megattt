#pragma once
#include "common.h"

void ClientInit(void);
void Send(const char* buf, int size);
void ClientReceive(void);
void ClientShutdown(void);
extern char buf[BUFLEN];
