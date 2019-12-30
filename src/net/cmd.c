#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>
#include <stdio.h>

#include "cmd.h"
#include "cmd_def.h"
#include "../log.h"
#include "../main.h"

#define ARG_MAX (20)

static volatile SceUID g_cmd_thid;
static volatile int g_server_sockfd;
static volatile int g_client_sockfd;
static volatile bool g_cmd_run = false;

static volatile bool g_cmd_client_connected = false;

static void cmd_parse(char *cmd, size_t cmd_size, char *command, char *arg) {
    for (unsigned int i = 0; i < cmd_size; i++) {
        if (cmd[i] == ' ' || cmd[i] == '\n') {
            strncpy(command, cmd, i);
            command[i] = '\0';

            if (i < cmd_size - 1) {
                strncpy(arg, &cmd[i + 1], cmd_size - i - 1);
                arg[cmd_size - i - 1] = '\0';
            }
            break;
        }
    }
}

static void cmd_handle(char* cmd, unsigned int cmd_size) {
    char command[128];
    char arg[256];
    char res_msg[256];

    cmd_parse(cmd, cmd_size, command, arg);
    const vgi_cmd_definition_t* cmd_def = vgi_cmd_get_definition(command);

    if (cmd_def == NULL) {
        snprintf(res_msg, sizeof(res_msg), "Error: Unknown command '%s'\n", command);
        vgi_cmd_send_msg(res_msg);
        return;
    }

    cmd_def->executor(arg);
}

bool vgi_cmd_is_client_connected() {
    return g_cmd_client_connected;
}

void vgi_cmd_send_msg(const char *msg) {
    if (g_client_sockfd < 0)
        return;

    if (!g_cmd_run || !g_cmd_client_connected)
        return;

    sceNetSend(g_client_sockfd, msg, strlen(msg), 0);
}

static int cmd_thread(unsigned int args, void* argp) {
    struct SceNetSockaddrIn serveraddr;
    struct SceNetSockaddrIn clientaddr;
    unsigned int addrlen = sizeof(clientaddr);

    serveraddr.sin_family = SCE_NET_AF_INET;
    serveraddr.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);
    serveraddr.sin_port = sceNetHtons(CMD_PORT);
    g_server_sockfd = sceNetSocket("VGi_cmd_sock", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);

    sceNetBind(g_server_sockfd, (struct SceNetSockaddr*)&serveraddr, sizeof(serveraddr));
    sceNetListen(g_server_sockfd, 128);

    while (g_cmd_run) {
        // wait for new connection
        g_client_sockfd = sceNetAccept(g_server_sockfd, (struct SceNetSockaddr*)&clientaddr, &addrlen);
        if (g_client_sockfd >= 0) {
            vgi_log_printf("connected\n");
            g_cmd_client_connected = true;

            // Welcome msg
            sceNetSend(g_client_sockfd, "VGi " VGi_VERSION "\n", 9, 0);
            sceNetSend(g_client_sockfd, "Say 'help' for list of commands.\n", 33, 0);

            // maintain connection
            while (g_cmd_run) {
                char cmd[100] = { 0 };

                sceNetSend(g_client_sockfd, "\n> ", 3, 0);
                int size = sceNetRecv(g_client_sockfd, cmd, sizeof(cmd), 0);

                if (size > 0) {
                    //vgi_log_printf("received %d bytes: '%s'\n", size, cmd);
                    cmd_handle(cmd, (unsigned int)size);
                } else if (size == 0) {
                    vgi_log_printf("client socket aborted\n");
                    break;
                } else {
                    vgi_log_printf("client socket recv error, 0x%08X\n", size);
                    break;
                }
            }

            g_cmd_client_connected = false;
            sceNetSocketClose(g_client_sockfd);
            g_client_sockfd = -1;
        } else {
            vgi_log_printf("client socked died, 0x%08X\n", g_client_sockfd);
            break;
        }
    }

    g_cmd_run = false;
    g_cmd_client_connected = false;
    sceNetSocketClose(g_server_sockfd);
    g_server_sockfd = -1;

    sceKernelExitDeleteThread(0);
    return 0;
}

void vgi_cmd_start() {
    if (g_cmd_run)
        return;

    vgi_log_printf("vgi_cmd_start():begin\n");

    g_cmd_thid = sceKernelCreateThread("VGi_cmd_thread", cmd_thread, 0x40, 0x10000, 0, 0, NULL);
    if (g_cmd_thid < 0) {
        vgi_log_printf("Failed to create cmd thread: 0x%08X.\n", g_cmd_thid);
        return;
    }

    g_cmd_run = true;
    sceKernelStartThread(g_cmd_thid, 0, NULL);
}

void vgi_cmd_stop() {
    if (!g_cmd_run)
        return;

    vgi_log_printf("vgi_cmd_stop():begin\n");

    g_cmd_run = false;

    if (g_server_sockfd >= 0)
        sceNetSocketClose(g_server_sockfd); // Kill sceNetAccept

    sceKernelWaitThreadEnd(g_cmd_thid, NULL, NULL);
}
