#include <vitasdk.h>
#include <stdbool.h>
#include <stdlib.h>

#include "net.h"
#include "cmd.h"
#include "../log.h"

static volatile bool g_net_run = false;
static volatile SceUID g_net_thid = -1;
static void * volatile g_pnetbuf = NULL;

static void *netctl_callback(int event_type, void *arg) {
    vgi_log_printf("netctl cb: %d\n", event_type);

    if (event_type == 1 || event_type == 2) {
        vgi_cmd_stop();
    }
    else if (event_type == 3) {
        vgi_cmd_start();
    }

    vgi_log_flush();

    return NULL;
}

static int net_thread(unsigned int args, void* argp) {
    int ret;
    int netctl_cb_id = -1;

    ret = sceNetCtlInetRegisterCallback(netctl_callback, NULL, &netctl_cb_id);
    vgi_log_printf("sceNetCtlInetRegisterCallback: 0x%08X\n", ret);
    if (ret != 0)
        goto EXIT_THREAD;

    while (g_net_run) {
        sceNetCtlCheckCallback();
        sceKernelDelayThread(1000 * 1000);
    }

    sceNetCtlInetUnregisterCallback(netctl_cb_id);

EXIT_THREAD:
    sceKernelExitDeleteThread(0);
    return 0;
}

bool vgi_net_is_running() {
    return g_net_run;
}

void vgi_net_start() {
    if (g_net_run)
        return;

    int ret;

    // Load PRX
    ret = sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    vgi_log_printf("sceSysmoduleLoadModule(SCE_SYSMODULE_NET): 0x%08X\n", ret);
    if (ret != 0)
        return;

    int size = 48 * 1024;
    g_pnetbuf = malloc(size);
    if (!g_pnetbuf) {
        vgi_log_printf("Failed to allocate %d bytes for SceNet!\n", size);
        return;
    }

    // Init SceNet
    SceNetInitParam netInitParam;
    netInitParam.memory = g_pnetbuf;
    netInitParam.size = size;
    netInitParam.flags = 0;
    ret = sceNetInit(&netInitParam);
    vgi_log_printf("sceNetInit: 0x%08X\n", ret);
    if (ret != 0) {
        free(g_pnetbuf);
        if (ret == 0x80410110) { // EBUSY
            goto SKIP_SCE_NET_INIT;
        }
        return;
    }

    // Init SceNetCtl
    ret = sceNetCtlInit();
    vgi_log_printf("sceNetCtlInit: 0x%08X\n", ret);
    if (ret != 0)
        return;

SKIP_SCE_NET_INIT: ;

    // Check for existing connection
    int inet_state = 0;
    if (sceNetCtlInetGetState(&inet_state) == 0) {
        if (inet_state == 3)
            vgi_cmd_start();
    }

    g_net_thid = sceKernelCreateThread("VGi_net_thread", net_thread, 0x40, 0x10000, 0, 0, NULL);
    if (g_net_thid < 0) {
        vgi_log_printf("Failed to create net thread: 0x%08X.\n", g_net_thid);
        return;
    }

    g_net_run = true;
    sceKernelStartThread(g_net_thid, 0, NULL);
}

void vgi_net_stop() {
    if (!g_net_run)
        return;

    vgi_cmd_stop();

    g_net_run = false;
    sceKernelWaitThreadEnd(g_net_thid, NULL, NULL);

    sceNetCtlTerm();
    sceNetTerm();

    if (g_pnetbuf)
        free(g_pnetbuf);
}
