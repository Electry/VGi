#include <vitasdk.h>
#include <taihen.h>
#include <stdio.h>

#include "main.h"
#include "log.h"

#define LOG_PATH         "ux0:data/VitaGrafix/VGi_log.txt"
#define LOG_BUFFER_SIZE  256

static uint32_t g_buffer_size = 0;
static char     g_buffer[LOG_BUFFER_SIZE];

void vgi_log_prepare() {
    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (fd >= 0)
        sceIoClose(fd);
}

void vgi_log_flush() {
    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
    if (fd < 0)
        return;

    sceIoWrite(fd, g_buffer, g_buffer_size);
    sceIoClose(fd);

    g_buffer_size = 0;
    memset(g_buffer, 0, LOG_BUFFER_SIZE);
}

void vgi_log_printf(const char *format, ...) {
    va_list args;
    va_start(args, format);

    char buffer[256];
    vsnprintf(buffer, sizeof(buffer), format, args);

    size_t print_len = strlen(buffer);
    if (g_buffer_size + print_len >= LOG_BUFFER_SIZE)
        vgi_log_flush(); // Flush buffer

    strncpy(&g_buffer[g_buffer_size], buffer, print_len + 1);
    g_buffer_size += print_len;

    va_end(args);
}

void vgi_log_read(char *dest, int size) {
    vgi_log_flush();

    SceUID fd = sceIoOpen(LOG_PATH, SCE_O_RDONLY | SCE_O_CREAT, 0777);
    if (fd < 0)
        return;

    long long fsize = sceIoLseek(fd, 0, SCE_SEEK_END);
    if (fsize > size)
        sceIoLseek(fd, -size, SCE_SEEK_CUR);
    else
        sceIoLseek(fd, 0, SCE_SEEK_SET);

    int read = sceIoRead(fd, dest, size - 1);
    dest[read] = '\0';

    sceIoClose(fd);
}
