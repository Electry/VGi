
#include <vitasdk.h>
#include <taihen.h>
#include <stdbool.h>

#include "../gui.h"
#include "../main.h"
#include "../net/cmd.h"

#undef SCE_GXM_MAX_UNIFORM_BUFFERS
#define SCE_GXM_MAX_UNIFORM_BUFFERS 14

typedef enum {
    UNIBUF_NOP,
    UNIBUF_WAITING_FOR_PROGRAM,
    UNIBUF_WAITING_FOR_BUFFER,
    UNIBUF_WAITING_FOR_DUMP,
} vgi_unibuf_dump_state_t;

typedef struct {
    vgi_unibuf_dump_state_t state;
    const SceGxmProgram *program;
    bool is_vertex;
    void *default_buffer;
    void *buffer[SCE_GXM_MAX_UNIFORM_BUFFERS];
} vgi_unibuf_dump_t;

typedef struct {
    const void *vf_program;
    const SceGxmProgram *program;
    bool is_vertex;
    bool is_fragment;
    uint32_t ts;
} vgi_program_t;

static vgi_program_t g_programs[MAX_PROGRAMS] = {0};
static int g_programs_count = 0;

static vgi_unibuf_dump_t g_unibuf_dump = {0};

static const char *get_category_text(SceGxmParameterCategory category) {
    switch (category) {
        case SCE_GXM_PARAMETER_CATEGORY_ATTRIBUTE: return "attribute";
        case SCE_GXM_PARAMETER_CATEGORY_UNIFORM: return "uniform";
        case SCE_GXM_PARAMETER_CATEGORY_SAMPLER: return "sampler";
        case SCE_GXM_PARAMETER_CATEGORY_AUXILIARY_SURFACE: return "surface";
        case SCE_GXM_PARAMETER_CATEGORY_UNIFORM_BUFFER: return "uniform buffer";
        default: return "unknown";
    }
}

static void dump_gxm_unibuf() {
    if (g_unibuf_dump.state != UNIBUF_WAITING_FOR_DUMP)
        return;

    g_unibuf_dump.state = UNIBUF_NOP;
    char msg[128];

    vgi_cmd_send_msg("Dumping now...\n");

    int param_count = sceGxmProgramGetParameterCount(g_unibuf_dump.program);
    if (param_count <= 0) {
        snprintf(msg, sizeof(msg), "Program 0x%lX has no parameters.\n", (uint32_t)g_unibuf_dump.program);
        vgi_cmd_send_msg(msg);
        return;
    }

    for (int i = 0; i < param_count; i++) {
        const SceGxmProgramParameter *param = sceGxmProgramGetParameter(g_unibuf_dump.program, i);
        const char *name = sceGxmProgramParameterGetName(param);
        uint32_t index = sceGxmProgramParameterGetResourceIndex(param);
        uint32_t size = sceGxmProgramParameterGetArraySize(param);
        SceGxmParameterCategory category = sceGxmProgramParameterGetCategory(param);

        snprintf(msg, sizeof(msg), "'%s' (%s) (index=0x%lX) (size=%ld)",
                name == NULL ? "<unk>" : name, get_category_text(category), index, size);
        vgi_cmd_send_msg(msg);

        if (category == SCE_GXM_PARAMETER_CATEGORY_UNIFORM) {
            uint32_t container_index = sceGxmProgramParameterGetContainerIndex(param);
            uint32_t vaddr;

            if (container_index == SCE_GXM_MAX_UNIFORM_BUFFERS) {
                vaddr = (uint32_t)g_unibuf_dump.default_buffer;
            } else if (container_index < SCE_GXM_MAX_UNIFORM_BUFFERS) {
                vaddr = (uint32_t)g_unibuf_dump.buffer[container_index];
            } else {
                snprintf(msg, sizeof(msg), "- unknown container index: %ld\n", container_index);
                vgi_cmd_send_msg(msg);
                continue;
            }
            if (!vaddr) {
                vgi_cmd_send_msg(" - FAIL: buffer is null!\n");
                continue;
            }

            snprintf(msg, sizeof(msg), " - 0x%lX: container: %ld\n", (vaddr + index), container_index);
            vgi_cmd_send_msg(msg);

            for (int el = 0; el < size + 1; el++) {
                uint32_t value = *(uint32_t *)(vaddr + index + (el * 4));

                float float_repr;
                memcpy(&float_repr, &value, sizeof(float));

                snprintf(msg, sizeof(msg), "  0x%08lX, %20ld, %f\n", value, value, float_repr);
                vgi_cmd_send_msg(msg);
            }

            if (size > 10)
                vgi_cmd_send_msg("  ...\n");
        }
        else {
            vgi_cmd_send_msg("\n");
        }
    }

    vgi_cmd_send_msg("Done!\n");
    vgi_cmd_send_msg("> ");
}

static int sceGxmSetVertexDefaultUniformBuffer_patched(
        SceGxmContext *context,
        const void *bufferData
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_SET_VERTEX_DEFAULT_UNIFORM_BUFFER],
                    context, bufferData);
    if (!ret) {
        if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_BUFFER && g_unibuf_dump.is_vertex) {
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;
            g_unibuf_dump.default_buffer = (void *)bufferData;

            char msg[128];
            snprintf(msg, sizeof(msg), "Buffer found: 0x%lX (set)\n", (uint32_t)g_unibuf_dump.buffer);
            vgi_cmd_send_msg(msg);
        }
    }

    return ret;
}

static int sceGxmReserveVertexDefaultUniformBuffer_patched(
        SceGxmContext *context,
        void **uniformBuffer
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_RESERVE_VERTEX_DEFAULT_UNIFORM_BUFFER],
                    context, uniformBuffer);
    if (!ret) {
        if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_BUFFER && g_unibuf_dump.is_vertex) {
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;
            g_unibuf_dump.default_buffer = *uniformBuffer;

            char msg[128];
            snprintf(msg, sizeof(msg), "Buffer found: 0x%lX (reserve)\n", (uint32_t)g_unibuf_dump.buffer);
            vgi_cmd_send_msg(msg);
        }
    }

    return ret;
}

static int sceGxmSetFragmentDefaultUniformBuffer_patched(
        SceGxmContext *context,
        const void *bufferData
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_SET_FRAGMENT_DEFAULT_UNIFORM_BUFFER],
                    context, bufferData);
    if (!ret) {
        if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_BUFFER && !g_unibuf_dump.is_vertex) {
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;
            g_unibuf_dump.default_buffer = (void *)bufferData;

            char msg[128];
            snprintf(msg, sizeof(msg), "Buffer found: 0x%lX (set)\n", (uint32_t)g_unibuf_dump.buffer);
            vgi_cmd_send_msg(msg);
        }
    }

    return ret;
}

static int sceGxmReserveFragmentDefaultUniformBuffer_patched(
        SceGxmContext *context,
        void **uniformBuffer
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_RESERVE_FRAGMENT_DEFAULT_UNIFORM_BUFFER],
                    context, uniformBuffer);
    if (!ret) {
        if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_BUFFER && !g_unibuf_dump.is_vertex) {
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;
            g_unibuf_dump.default_buffer = *uniformBuffer;

            char msg[128];
            snprintf(msg, sizeof(msg), "Buffer found: 0x%lX (reserve)\n", (uint32_t)g_unibuf_dump.buffer);
            vgi_cmd_send_msg(msg);
        }
    }

    return ret;
}

static int sceGxmSetVertexUniformBuffer_patched(
        SceGxmContext *context,
        unsigned int bufferIndex,
        const void *bufferData
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_SET_VERTEX_UNIFORM_BUFFER],
                    context, bufferIndex, bufferData);
    if (!ret && bufferIndex >= 0 && bufferIndex < SCE_GXM_MAX_UNIFORM_BUFFERS) {
        if ((g_unibuf_dump.state == UNIBUF_WAITING_FOR_BUFFER
                || g_unibuf_dump.state == UNIBUF_WAITING_FOR_DUMP)
                && g_unibuf_dump.is_vertex) {
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;
            g_unibuf_dump.buffer[bufferIndex] = (void *)bufferData;
            dump_gxm_unibuf();
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;

            char msg[128];
            snprintf(msg, sizeof(msg), "Buffer #%d found: 0x%lX (set)\n", bufferIndex, (uint32_t)g_unibuf_dump.buffer[bufferIndex]);
            vgi_cmd_send_msg(msg);
        }
    }

    return ret;
}

static int sceGxmSetFragmentUniformBuffer_patched(
        SceGxmContext *context,
        unsigned int bufferIndex,
        const void *bufferData
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_SET_FRAGMENT_UNIFORM_BUFFER],
                    context, bufferIndex, bufferData);
    if (!ret && bufferIndex >= 0 && bufferIndex < SCE_GXM_MAX_UNIFORM_BUFFERS) {
        if ((g_unibuf_dump.state == UNIBUF_WAITING_FOR_BUFFER
                || g_unibuf_dump.state == UNIBUF_WAITING_FOR_DUMP)
                && !g_unibuf_dump.is_vertex) {
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;
            g_unibuf_dump.buffer[bufferIndex] = (void *)bufferData;
            dump_gxm_unibuf();
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_DUMP;

            char msg[128];
            snprintf(msg, sizeof(msg), "Buffer #%d found: 0x%lX (set)\n", bufferIndex, (uint32_t)g_unibuf_dump.buffer[bufferIndex]);
            vgi_cmd_send_msg(msg);
        }
    }

    return ret;
}

static int sceGxmEndScene_patched(
    SceGxmContext *immediateContext,
    const SceGxmNotification *vertexNotification,
    const SceGxmNotification *fragmentNotification
) {
    int ret = TAI_CONTINUE(int, g_hookrefs[HOOK_SCE_GXM_END_SCENE],
                    immediateContext, vertexNotification, fragmentNotification);
    if (!ret) {
        if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_DUMP) {
            dump_gxm_unibuf();
        }
    }

    return ret;
}

static void sceGxmSetVertexProgram_patched(
        SceGxmContext *context,
        const SceGxmVertexProgram *vertexProgram
) {
    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_VERTEX_PROGRAM],
                    context, vertexProgram);

    if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_PROGRAM) {
        if (g_unibuf_dump.program == sceGxmVertexProgramGetProgram(vertexProgram)) {
            g_unibuf_dump.is_vertex = true;
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_BUFFER;
            vgi_cmd_send_msg("\nProgram set (vertex)\n");
        }
    } else if (g_unibuf_dump.state != UNIBUF_NOP && g_unibuf_dump.is_vertex) {
        dump_gxm_unibuf();
    }

    for (int i = 0; i < MAX_PROGRAM_PARAMETERS; i++) {
        if (g_programs[i].vf_program == (void *)vertexProgram || // Existing
                g_programs[i].vf_program == NULL) { // New
            if (g_programs[i].vf_program == NULL)
                g_programs_count++;

            g_programs[i].vf_program = (void *)vertexProgram;
            g_programs[i].program = sceGxmVertexProgramGetProgram(vertexProgram);
            g_programs[i].ts = sceKernelGetProcessTimeWide() / 1000000;
            g_programs[i].is_vertex = true;
            break;
        }
    }
}

static void sceGxmSetFragmentProgram_patched(
        SceGxmContext *context,
        const SceGxmFragmentProgram *fragmentProgram
) {
    TAI_CONTINUE(void, g_hookrefs[HOOK_SCE_GXM_SET_FRAGMENT_PROGRAM],
                    context, fragmentProgram);

    if (g_unibuf_dump.state == UNIBUF_WAITING_FOR_PROGRAM) {
        if (g_unibuf_dump.program == sceGxmFragmentProgramGetProgram(fragmentProgram)) {
            g_unibuf_dump.is_vertex = false;
            g_unibuf_dump.state = UNIBUF_WAITING_FOR_BUFFER;
            vgi_cmd_send_msg("\nProgram set (fragment)\n");
        }
    } else if (g_unibuf_dump.state != UNIBUF_NOP && !g_unibuf_dump.is_vertex) {
        dump_gxm_unibuf();
    }

    for (int i = 0; i < MAX_PROGRAM_PARAMETERS; i++) {
        if (g_programs[i].vf_program == (void *)fragmentProgram || // Existing
                g_programs[i].vf_program == NULL) { // New
            if (g_programs[i].vf_program == NULL)
                g_programs_count++;

            g_programs[i].vf_program = (void *)fragmentProgram;
            g_programs[i].program = sceGxmFragmentProgramGetProgram(fragmentProgram);
            g_programs[i].ts = sceKernelGetProcessTimeWide() / 1000000;
            g_programs[i].is_fragment = true;
            break;
        }
    }
}

void vgi_dump_graphics_unibuf(const SceGxmProgram *program) {
    g_unibuf_dump.default_buffer = NULL;
    for (int i = 0; i < SCE_GXM_MAX_UNIFORM_BUFFERS; i++)
        g_unibuf_dump.buffer[i] = NULL;
    g_unibuf_dump.program = program;
    g_unibuf_dump.state = UNIBUF_WAITING_FOR_PROGRAM;
}

void vgi_dump_graphics_cg() {
    char msg[128];
    snprintf(msg, sizeof(msg), "GXP Programs (%d/%d):\n", g_programs_count, MAX_PROGRAMS);
    vgi_cmd_send_msg(msg);

    uint32_t time_now = sceKernelGetProcessTimeWide() / 1000000;

    for (int i = 0; i < MAX_PROGRAMS; i++) {
        if (g_programs[i].program) {
            snprintf(msg, sizeof(msg),
                    "  0x%-8lX  p=0x%-8lX  %s  %2ldsec\n",
                    (uint32_t)g_programs[i].vf_program,
                    (uint32_t)g_programs[i].program,
                    g_programs[i].is_vertex && g_programs[i].is_fragment ? "VF" : (g_programs[i].is_vertex ? " V" : " F"),
                    (time_now - g_programs[i].ts) > 1 ? time_now - g_programs[i].ts : 0);
            vgi_cmd_send_msg(msg);
        }
    }
}

void vgi_draw_graphics_cg(int xoff, int yoff, int x2off, int y2off) {
    uint32_t time_now = sceKernelGetProcessTimeWide() / 1000000;

    vgi_gui_printf(GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 0),
            "GXP Programs (%d):", g_programs_count);
    vgi_gui_printf(GUI_ANCHOR_LX(xoff + 10, 0), GUI_ANCHOR_TY(yoff, 1.5f),
            "   vfpr        program     type   last used");

    GUI_SCROLLABLE(MAX_PROGRAMS, g_programs_count, 1,
                    GUI_ANCHOR_LX(xoff, 0), GUI_ANCHOR_TY(yoff, 3),
                    GUI_ANCHOR_RX(x2off, 0), GUI_ANCHOR_BY(y2off, 0)) {
        if (g_programs[i].program) {
            vgi_gui_printf(GUI_ANCHOR_LX(xoff, 1), GUI_ANCHOR_TY(yoff, 3 + (i - g_scroll)),
                    "0x%-10X 0x%-10X   %s     %2d sec",
                    g_programs[i].vf_program,
                    g_programs[i].program,
                    g_programs[i].is_vertex && g_programs[i].is_fragment ? "VF" : (g_programs[i].is_vertex ? " V" : " F"),
                    (time_now - g_programs[i].ts) > 1 ? time_now - g_programs[i].ts : 0);
        }
    }
}

void vgi_setup_graphics_cg() {
    HOOK_FUNCTION_IMPORT(0x97118913, HOOK_SCE_GXM_RESERVE_VERTEX_DEFAULT_UNIFORM_BUFFER, sceGxmReserveVertexDefaultUniformBuffer_patched);
    HOOK_FUNCTION_IMPORT(0x7B1FABB6, HOOK_SCE_GXM_RESERVE_FRAGMENT_DEFAULT_UNIFORM_BUFFER, sceGxmReserveFragmentDefaultUniformBuffer_patched);
    HOOK_FUNCTION_IMPORT(0xC697CAE5, HOOK_SCE_GXM_SET_VERTEX_DEFAULT_UNIFORM_BUFFER, sceGxmSetVertexDefaultUniformBuffer_patched);
    HOOK_FUNCTION_IMPORT(0xA824EB24, HOOK_SCE_GXM_SET_FRAGMENT_DEFAULT_UNIFORM_BUFFER, sceGxmSetFragmentDefaultUniformBuffer_patched);
    HOOK_FUNCTION_IMPORT(0xC68015E4, HOOK_SCE_GXM_SET_VERTEX_UNIFORM_BUFFER, sceGxmSetVertexUniformBuffer_patched);
    HOOK_FUNCTION_IMPORT(0xEA0FC310, HOOK_SCE_GXM_SET_FRAGMENT_UNIFORM_BUFFER, sceGxmSetFragmentUniformBuffer_patched);
    HOOK_FUNCTION_IMPORT(0xFE300E2F, HOOK_SCE_GXM_END_SCENE, sceGxmEndScene_patched);
    HOOK_FUNCTION_IMPORT(0x31FF8ABD, HOOK_SCE_GXM_SET_VERTEX_PROGRAM, sceGxmSetVertexProgram_patched);
    HOOK_FUNCTION_IMPORT(0xAD2F48D9, HOOK_SCE_GXM_SET_FRAGMENT_PROGRAM, sceGxmSetFragmentProgram_patched);
}
