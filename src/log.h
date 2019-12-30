#ifndef _LOG_H_
#define _LOG_H_

void vgi_log_prepare();
void vgi_log_flush();
void vgi_log_printf(const char *format, ...);
void vgi_log_read(char *dest, int size);

#endif
