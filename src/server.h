#pragma once

void Broadcast(const char* buf, int size);
void ServerShutdown(void);
void ServerInit(void);
void Receive(void);
int ReceiveMultiple(void);
