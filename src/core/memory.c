#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>

#include "../gui.h"
#include "../main.h"

struct mem_callback_table1 {
    int unk0;
    int unk4;
    int (*user_malloc_init)();
    int (*user_malloc_finalize)();
    void *(*user_malloc)(int);
    void (*user_free)(void*);
    int (*user_calloc)(int,int);
    int (*user_realloc)(int,int);
    int (*user_memalign)(int,int);
    int (*user_reallocalign)(int,int,int);
    int (*user_malloc_stats)(int);
    int (*user_malloc_stats_fast)(int);
    int (*user_malloc_usable_size)(int);
};

struct local_8708FC60_ctx_holder {
    char pad0[8 * 4];
    struct mem_callback_table1 *callback_table1;
    char pad1[5 * 4];
};

struct process_param {
    char pad0[11 * 4];
    struct local_8708FC60_ctx_holder *hldr_2C; // SceLibcparam
};

struct malloc_managed_size {
    size_t max_system_size;
    size_t current_system_size;
    size_t max_inuse_size;
    size_t current_inuse_size;
    char pad0[4 * 4];
};

struct process_param *sceKernelGetProcessParam();
int malloc_stats_fast(struct malloc_managed_size *mmsize);

static vgi_memblock_t g_memblocks[MAX_MEM_BLOCKS] = {0};
static uint32_t g_memblocks_count = 0;
static SceKernelFreeMemorySizeInfo g_start_free_mem = {0};

static bool g_has_custom_heap_impl = false;
static uint32_t g_custom_heap_init_fn_ptr = 0;

static SceUID sceKernelAllocMemBlock_patched(
        const char *name,
        SceKernelMemBlockType type,
        int size,
        void *optp
) {
    SceUID ret = TAI_CONTINUE(SceUID, g_hookrefs[HOOK_SCE_KERNEL_ALLOC_MEM_BLOCK],
                                name, type, size, optp);

    for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
        if (!g_memblocks[i].active) {
            g_memblocks[i].active = 1;
            g_memblocks[i].success = ret >= SCE_OK;
            strncpy(g_memblocks[i].name, name, 128);
            g_memblocks[i].type = type;
            g_memblocks[i].size = size;
            g_memblocks[i].ret = ret;
            sceKernelGetMemBlockBase(ret, &(g_memblocks[i].base));
            g_memblocks_count++;
            break;
        }
    }

    return ret;
}

static int sceKernelFreeMemBlock_patched(SceUID uid) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_KERNEL_FREE_MEM_BLOCK], uid);
    if (ret < SCE_OK)
        return ret;

    for (int i = 0; i < MAX_MEM_BLOCKS; i++) {
        if (g_memblocks[i].active && g_memblocks[i].ret == uid) {
            g_memblocks[i].active = 0;
            g_memblocks_count--;
            break;
        }
    }

    return ret;
}

static void format_readable_size(int bytes, char *out, size_t out_size) {
    if (bytes >= 1024*1024) {
        snprintf(out, out_size, "%d%s", bytes / 1024 / 1024, "MB");
    } else if (bytes >= 1024) {
        snprintf(out, out_size, "%d%s", bytes / 1024, "kB");
    } else {
        snprintf(out, out_size, "%d%s", bytes, "B");
    }
}

const char *vgi_get_memblock_type_text(SceKernelMemBlockType type) {
    switch (type) {
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE:
            return "UNCACHE";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_RW:
            return "RW";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW:
            return "PHYCONT";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_NC_RW:
            return "PHY_NC";
        case SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW:
            return "CDRAM";
        default:
            return "?";
    }
}

vgi_memblock_t *vgi_get_memblocks() {
    return g_memblocks;
}

void vgi_draw_memory(int xoff, int yoff, int x2off, int y2off) {
    SceKernelFreeMemorySizeInfo info;
    info.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&info);

    char buf[16], buf2[16], buf3[16];
    char buf4[32], buf5[16], buf6[16];
    format_readable_size(info.size_user, buf, sizeof(buf));
    format_readable_size(info.size_cdram, buf2, sizeof(buf2));
    format_readable_size(info.size_phycont, buf3, sizeof(buf3));
    format_readable_size(g_start_free_mem.size_user, buf4, sizeof(buf4));
    format_readable_size(g_start_free_mem.size_cdram, buf5, sizeof(buf5));
    format_readable_size(g_start_free_mem.size_phycont, buf6, sizeof(buf6));
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "Free RAM:  %s / %s    VRAM: %s / %s    PHYCONT: %s / %s",
            buf, buf4, buf2, buf5, buf3, buf6);

    struct malloc_managed_size ms;
    malloc_stats_fast(&ms);
    format_readable_size(ms.current_system_size - ms.current_inuse_size, buf, sizeof(buf));
    format_readable_size(ms.current_system_size, buf2, sizeof(buf2));
    format_readable_size(ms.max_system_size, buf3, sizeof(buf3));
    snprintf(buf4, sizeof(buf4), ": 0x%lX", g_custom_heap_init_fn_ptr);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 1),
            "Free heap: %s / %s (max. %s) - %s impl.%s",
            buf, buf2, buf3,
            g_has_custom_heap_impl ? "custom" : "native",
            g_has_custom_heap_impl ? buf4 : "");

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 2),
            "Allocated MemBlocks (%d):", g_memblocks_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 3.5f),
            "  type          UID         base        size");

    GUI_SCROLLABLE(MAX_MEM_BLOCKS, g_memblocks_count, 2,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 5),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_memblocks[i].active) {
            if (g_memblocks[i].success)
                vgi_gui_set_text_color(255, 255, 255, 255);
            else
                vgi_gui_set_text_color(255, 0, 0, 255);

            format_readable_size(g_memblocks[i].size, buf, sizeof(buf));
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 5 + 2 * (i - g_scroll)),
                    "'%s'", g_memblocks[i].name);
            vgi_gui_printf(GUI_ANCHOR_RX(x2off + 40, strlen(buf)), GUI_ANCHOR_TY(yoff, 5 + 2 * (i - g_scroll)),
                    "%s", buf);
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 6 + 2 * (i - g_scroll)),
                    "%-9s 0x%-10X 0x%-10X 0x%X B",
                    vgi_get_memblock_type_text(g_memblocks[i].type),
                    g_memblocks[i].ret,
                    g_memblocks[i].base,
                    g_memblocks[i].size);
        }
    }
}

void vgi_setup_memory() {
    // poll free mem
    g_start_free_mem.size = sizeof(SceKernelFreeMemorySizeInfo);
    sceKernelGetFreeMemorySize(&g_start_free_mem);

    // is using custom malloc?
    struct process_param *pparam = (struct process_param *)sceKernelGetProcessParam(); // 0x2BE3E066
    if (pparam
            && pparam->hldr_2C
            && pparam->hldr_2C->callback_table1
            && pparam->hldr_2C->callback_table1->user_malloc_init) {
        g_has_custom_heap_impl = true;
        g_custom_heap_init_fn_ptr = (uint32_t)pparam->hldr_2C->callback_table1->user_malloc_init;
    }

    HOOK_FUNCTION_IMPORT(0xB9D5EBDE, HOOK_SCE_KERNEL_ALLOC_MEM_BLOCK, sceKernelAllocMemBlock_patched);
    HOOK_FUNCTION_IMPORT(0xA91E15EE, HOOK_SCE_KERNEL_FREE_MEM_BLOCK, sceKernelFreeMemBlock_patched);
}
