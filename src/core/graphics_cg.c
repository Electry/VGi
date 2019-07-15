
#include <vitasdk.h>
#include <taihen.h>

#include "../osd.h"
#include "../main.h"

typedef struct {
    uint8_t active;
    SceGxmProgramParameter *parameter;
    const SceGxmProgram *program;
    char name[128];
} VGi_ProgramParameter;

static VGi_ProgramParameter g_programParameters[MAX_PROGRAM_PARAMETERS] = {0};
static uint32_t g_programParametersCount = 0;

static const SceGxmProgramParameter *sceGxmProgramFindParameterByName_patched(
        const SceGxmProgram *program,
        const char *name) {
    SceGxmProgramParameter *ret = TAI_CONTINUE(SceGxmProgramParameter *,
            g_hookrefs[HOOK_SCE_GXM_PROGRAM_FIND_PARAMETER_BY_NAME],
            program, name);

    for (int i = 0; i < MAX_PROGRAM_PARAMETERS; i++) {
        if ((ret != NULL && g_programParameters[i].parameter == ret) ||
                !strncmp(g_programParameters[i].name, name, 128) || // Existing (reinit)
                !g_programParameters[i].active) { // New
            if (!g_programParameters[i].active)
                g_programParametersCount++;

            g_programParameters[i].active = 1;
            g_programParameters[i].parameter = ret;
            g_programParameters[i].program = program;
            strncpy(g_programParameters[i].name, name, 128);
            break;
        }
    }

    return ret;
}

void drawGraphicsCg(int minX, int minY, int maxX, int maxY) {
    int x = minX, y = minY;
    osdDrawStringF(x, y, "Captured Cg Program Parameters (%d):", g_programParametersCount);
    osdDrawStringF(x += 10, y += osdGetLineHeight() * 1.5f,
                    "  program   parameter  name");

    // Scrollable section
    y += osdGetLineHeight() * 0.5f;
    drawScrollIndicator(maxX - osdGetTextWidth("xx"), y + osdGetLineHeight(), 0, g_scroll);

    for (int i = g_scroll; i < MAX_PROGRAM_PARAMETERS; i++) {
        if (y > maxY - osdGetLineHeight()) {
            drawScrollIndicator(maxX - osdGetTextWidth("xx"), maxY - osdGetLineHeight() * 2,
                                1, g_programParametersCount - i);
            break;
        }

        if (g_programParameters[i].active) {
            if (g_programParameters[i].parameter != NULL)
                osdSetTextColor(255, 255, 255, 255);
            else
                osdSetTextColor(255, 0, 0, 255);

            osdDrawStringF(x, y += osdGetLineHeight(), "0x%-10X 0x%-10X %s",
                    g_programParameters[i].program,
                    g_programParameters[i].parameter,
                    g_programParameters[i].name);
        }
    }
}

void setupGraphicsCg() {
    hookFunctionImport(0x277794C4, HOOK_SCE_GXM_PROGRAM_FIND_PARAMETER_BY_NAME, sceGxmProgramFindParameterByName_patched);
}
