#include <pspctrl.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspiofilemgr.h>
#include <pspkernel.h>

#include <math.h>
#include <stdint.h>
#include <string.h>

PSP_MODULE_INFO("PSPBlox", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static const int SCREEN_W = 480;
static const int SCREEN_H = 272;
static const int BUF_W = 512;
static const int WORLD_W = 16;
static const int WORLD_H = 4;
static const int WORLD_D = 16;
static const char *SAVE_DIR = "ms0:/PSP/SAVEDATA/PSPBLOX";
static const char *SAVE_PATH = "ms0:/PSP/SAVEDATA/PSPBLOX/world.dat";

static unsigned int __attribute__((aligned(16))) displayList[262144];
static unsigned char world[WORLD_W][WORLD_H][WORLD_D];

struct Vertex {
    unsigned int color;
    float x, y, z;
};

#define V(c, x, y, z) { c, x, y, z }
static const Vertex cubeVertices[] = {
    // Front
    V(GU_RGBA(76, 174, 78, 255), -0.5f,-0.5f, 0.5f), V(GU_RGBA(76, 174, 78, 255), 0.5f,-0.5f, 0.5f), V(GU_RGBA(76, 174, 78, 255), 0.5f, 0.5f, 0.5f),
    V(GU_RGBA(76, 174, 78, 255), -0.5f,-0.5f, 0.5f), V(GU_RGBA(76, 174, 78, 255), 0.5f, 0.5f, 0.5f), V(GU_RGBA(76, 174, 78, 255), -0.5f, 0.5f, 0.5f),
    // Back
    V(GU_RGBA(54, 126, 58, 255), 0.5f,-0.5f,-0.5f), V(GU_RGBA(54, 126, 58, 255),-0.5f,-0.5f,-0.5f), V(GU_RGBA(54, 126, 58, 255),-0.5f, 0.5f,-0.5f),
    V(GU_RGBA(54, 126, 58, 255), 0.5f,-0.5f,-0.5f), V(GU_RGBA(54, 126, 58, 255),-0.5f, 0.5f,-0.5f), V(GU_RGBA(54, 126, 58, 255), 0.5f, 0.5f,-0.5f),
    // Left
    V(GU_RGBA(65, 145, 68, 255),-0.5f,-0.5f,-0.5f), V(GU_RGBA(65, 145, 68, 255),-0.5f,-0.5f, 0.5f), V(GU_RGBA(65, 145, 68, 255),-0.5f, 0.5f, 0.5f),
    V(GU_RGBA(65, 145, 68, 255),-0.5f,-0.5f,-0.5f), V(GU_RGBA(65, 145, 68, 255),-0.5f, 0.5f, 0.5f), V(GU_RGBA(65, 145, 68, 255),-0.5f, 0.5f,-0.5f),
    // Right
    V(GU_RGBA(88, 190, 88, 255), 0.5f,-0.5f, 0.5f), V(GU_RGBA(88, 190, 88, 255), 0.5f,-0.5f,-0.5f), V(GU_RGBA(88, 190, 88, 255), 0.5f, 0.5f,-0.5f),
    V(GU_RGBA(88, 190, 88, 255), 0.5f,-0.5f, 0.5f), V(GU_RGBA(88, 190, 88, 255), 0.5f, 0.5f,-0.5f), V(GU_RGBA(88, 190, 88, 255), 0.5f, 0.5f, 0.5f),
    // Top
    V(GU_RGBA(120, 210, 105, 255),-0.5f, 0.5f, 0.5f), V(GU_RGBA(120, 210, 105, 255), 0.5f, 0.5f, 0.5f), V(GU_RGBA(120, 210, 105, 255), 0.5f, 0.5f,-0.5f),
    V(GU_RGBA(120, 210, 105, 255),-0.5f, 0.5f, 0.5f), V(GU_RGBA(120, 210, 105, 255), 0.5f, 0.5f,-0.5f), V(GU_RGBA(120, 210, 105, 255),-0.5f, 0.5f,-0.5f),
    // Bottom
    V(GU_RGBA(91, 73, 48, 255),-0.5f,-0.5f,-0.5f), V(GU_RGBA(91, 73, 48, 255), 0.5f,-0.5f,-0.5f), V(GU_RGBA(91, 73, 48, 255), 0.5f,-0.5f, 0.5f),
    V(GU_RGBA(91, 73, 48, 255),-0.5f,-0.5f,-0.5f), V(GU_RGBA(91, 73, 48, 255), 0.5f,-0.5f, 0.5f), V(GU_RGBA(91, 73, 48, 255),-0.5f,-0.5f, 0.5f)
};
#undef V

struct SaveData {
    char magic[8];
    unsigned char blocks[WORLD_W][WORLD_H][WORLD_D];
};

static float playerX = 8.0f;
static float playerZ = 8.0f;
static float yaw = 0.0f;

static void initializeWorld() {
    memset(world, 0, sizeof(world));
    for (int x = 0; x < WORLD_W; ++x) {
        for (int z = 0; z < WORLD_D; ++z) {
            world[x][0][z] = 1;
        }
    }
    // A few original starter props to make the tiny world less empty.
    world[5][1][5] = 1;
    world[5][2][5] = 1;
    world[11][1][10] = 1;
}

static void saveWorld() {
    sceIoMkdir("ms0:/PSP/SAVEDATA", 0777);
    sceIoMkdir(SAVE_DIR, 0777);
    SaveData save;
    memcpy(save.magic, "PBLOX01", 8);
    memcpy(save.blocks, world, sizeof(world));
    SceUID fd = sceIoOpen(SAVE_PATH, PSP_O_WRONLY | PSP_O_CREAT | PSP_O_TRUNC, 0777);
    if (fd >= 0) {
        sceIoWrite(fd, &save, sizeof(save));
        sceIoClose(fd);
    }
}

static void loadWorld() {
    SceUID fd = sceIoOpen(SAVE_PATH, PSP_O_RDONLY, 0777);
    if (fd < 0) return;
    SaveData save;
    int bytes = sceIoRead(fd, &save, sizeof(save));
    sceIoClose(fd);
    if (bytes == (int)sizeof(save) && memcmp(save.magic, "PBLOX01", 8) == 0) {
        memcpy(world, save.blocks, sizeof(world));
    }
}

static void drawBlock(int x, int y, int z) {
    sceGumPushMatrix();
    ScePspFVector3 pos = { x - 7.5f, y + 0.5f, z - 7.5f };
    sceGumTranslate(&pos);
    sceGumDrawArray(GU_TRIANGLES,
                    GU_COLOR_8888 | GU_VERTEX_32BITF | GU_TRANSFORM_3D,
                    sizeof(cubeVertices) / sizeof(cubeVertices[0]), 0, cubeVertices);
    sceGumPopMatrix();
}

static void renderFrame() {
    sceGuStart(GU_DIRECT, displayList);
    sceGuClearColor(GU_RGBA(112, 184, 232, 255));
    sceGuClearDepth(0);
    sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

    sceGumMatrixMode(GU_PROJECTION);
    sceGumLoadIdentity();
    sceGumPerspective(55.0f, 16.0f / 9.0f, 0.5f, 80.0f);

    float fx = sinf(yaw);
    float fz = cosf(yaw);
    ScePspFVector3 camera = { playerX - fx * 5.0f, 4.2f, playerZ - fz * 5.0f };
    ScePspFVector3 lookAt = { playerX - 7.5f, 1.0f, playerZ - 7.5f };
    ScePspFVector3 up = { 0.0f, 1.0f, 0.0f };
    // GU world coordinates are centered around the map.
    camera.x -= 7.5f;
    camera.z -= 7.5f;
    sceGumMatrixMode(GU_VIEW);
    sceGumLoadIdentity();
    sceGumLookAt(&camera, &lookAt, &up);

    sceGumMatrixMode(GU_MODEL);
    sceGumLoadIdentity();
    sceGuDisable(GU_TEXTURE_2D);
    sceGuDisable(GU_LIGHTING);
    sceGuEnable(GU_DEPTH_TEST);
    sceGuDepthFunc(GU_GEQUAL);
    sceGuFrontFace(GU_CW);
    sceGuShadeModel(GU_SMOOTH);

    for (int x = 0; x < WORLD_W; ++x) {
        for (int z = 0; z < WORLD_D; ++z) {
            for (int y = 0; y < WORLD_H; ++y) {
                if (world[x][y][z]) drawBlock(x, y, z);
            }
        }
    }

    sceGuFinish();
    sceGuSync(0, 0);
    sceDisplayWaitVblankStart();
    sceGuSwapBuffers();
}

static void updatePlayer(const SceCtrlData &pad) {
    float sx = ((int)pad.Lx - 128) / 128.0f;
    float sy = ((int)pad.Ly - 128) / 128.0f;
    if (fabsf(sx) < 0.18f) sx = 0.0f;
    if (fabsf(sy) < 0.18f) sy = 0.0f;

    if (pad.Buttons & PSP_CTRL_LTRIGGER) yaw -= 0.055f;
    if (pad.Buttons & PSP_CTRL_RTRIGGER) yaw += 0.055f;
    float fx = sinf(yaw), fz = cosf(yaw);
    float rightX = cosf(yaw), rightZ = -sinf(yaw);
    playerX += (fx * -sy + rightX * sx) * 0.12f;
    playerZ += (fz * -sy + rightZ * sx) * 0.12f;
    if (playerX < 1.0f) playerX = 1.0f;
    if (playerX > WORLD_W - 2.0f) playerX = WORLD_W - 2.0f;
    if (playerZ < 1.0f) playerZ = 1.0f;
    if (playerZ > WORLD_D - 2.0f) playerZ = WORLD_D - 2.0f;
}

static void editWorld(unsigned int buttons) {
    float fx = sinf(yaw), fz = cosf(yaw);
    int tx = (int)floorf(playerX + fx * 1.5f);
    int tz = (int)floorf(playerZ + fz * 1.5f);
    if (tx < 0 || tx >= WORLD_W || tz < 0 || tz >= WORLD_D) return;

    if (buttons & PSP_CTRL_CROSS) {
        if (world[tx][1][tz] == 0) world[tx][1][tz] = 1;
    }
    if (buttons & PSP_CTRL_SQUARE) {
        if (world[tx][1][tz] != 0) world[tx][1][tz] = 0;
        else if (world[tx][2][tz] != 0) world[tx][2][tz] = 0;
    }
}

int main() {
    initializeWorld();
    loadWorld();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    sceGuInit();
    sceGuStart(GU_DIRECT, displayList);
    sceGuDrawBuffer(GU_PSM_8888, (void *)0, BUF_W);
    sceGuDispBuffer(SCREEN_W, SCREEN_H, (void *)0x88000, BUF_W);
    sceGuDepthBuffer((void *)0x110000, BUF_W);
    sceGuOffset(2048 - (SCREEN_W / 2), 2048 - (SCREEN_H / 2));
    sceGuViewport(2048, 2048, SCREEN_W, SCREEN_H);
    sceGuDepthRange(65535, 0);
    sceGuScissor(0, 0, SCREEN_W, SCREEN_H);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuEnable(GU_CULL_FACE);
    sceGuFinish();
    sceGuSync(0, 0);
    sceGuDisplay(GU_TRUE);

    SceCtrlData pad;
    unsigned int previous = 0;
    int running = 1;
    while (running) {
        sceCtrlReadBufferPositive(&pad, 1);
        updatePlayer(pad);
        unsigned int pressed = pad.Buttons & ~previous;
        if (pressed & PSP_CTRL_CROSS) editWorld(PSP_CTRL_CROSS);
        if (pressed & PSP_CTRL_SQUARE) editWorld(PSP_CTRL_SQUARE);
        if (pressed & PSP_CTRL_SELECT) saveWorld();
        if (pressed & PSP_CTRL_START) running = 0;
        previous = pad.Buttons;
        renderFrame();
    }

    sceGuTerm();
    sceKernelExitGame();
    return 0;
}
