#ifndef _CMD_DEF_H_
#define _CMD_DEF_H
#include <stdint.h>

#define RES_MSG_LEN 128

typedef void vgi_cmd_executor_t(char *arg);

typedef struct {
    char *name;
    vgi_cmd_executor_t *executor;
} vgi_cmd_definition_t;

const vgi_cmd_definition_t *vgi_cmd_get_definition(char *cmd_name);
uint32_t vgi_cmd_parse_address(char *arg, char **end);
uint32_t vgi_cmd_addr_to_offset(uint32_t addr);

void vgi_cmd_mem_read_dword(char *arg);
void vgi_cmd_mem_write_dword(char *arg);
void vgi_cmd_mem_read_range_dword(char *arg);

void vgi_cmd_mem_inject_data(char *arg);
void vgi_cmd_mem_release_data(char *arg);

void vgi_cmd_mem_search(char *arg);

void vgi_cmd_hook_dump_args_hook(char *arg);
void vgi_cmd_hook_dump_args_release(char *arg);

void vgi_cmd_gxm_listparams(char *arg);
void vgi_cmd_gxm_dumpparams(char *arg);

#endif
