#ifndef _CMD_H_
#define _CMD_H_
#include <stdbool.h>

#define CMD_PORT 1339

void vgi_cmd_start();
void vgi_cmd_stop();

void vgi_cmd_send_msg(const char *msg);
bool vgi_cmd_is_client_connected();

#endif
