#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "font_8x16.h"

#define SCREEN_W 1080
#define SCREEN_H 2400

static uint32_t *pixels = NULL;
static int screen_pitch = 0;

static void force_usb_peripheral(void) {
    const char *paths[] = {
        "/sys/devices/platform/soc/a600000.ssusb",
        "/sys/bus/platform/devices/a600000.ssusb",
        NULL
    };
    for (int i = 0; paths[i]; i++) {
        char buf[128];
        snprintf(buf, sizeof(buf), "%s/usb_compliance_mode", paths[i]);
        int fd = open(buf, O_WRONLY);
        if (fd >= 0) { write(fd, "Y\n", 2); close(fd); }

        snprintf(buf, sizeof(buf), "%s/bus_vote", paths[i]);
        fd = open(buf, O_WRONLY);
        if (fd >= 0) { write(fd, "max\n", 4); close(fd); }

        snprintf(buf, sizeof(buf), "%s/mode", paths[i]);
        fd = open(buf, O_WRONLY);
        if (fd >= 0) { write(fd, "peripheral\n", 11); close(fd); }
    }
}

static void set_backlight(void) {
    int fd;
    fd = open("/sys/class/backlight/backlight/brightness", O_WRONLY);
    if (fd >= 0) { write(fd, "3500\n", 5); close(fd); }
    fd = open("/sys/class/backlight/backlight/bl_power", O_WRONLY);
    if (fd >= 0) { write(fd, "0\n", 2); close(fd); }
    fd = open("/sys/class/backlight/panel0-backlight/brightness", O_WRONLY);
    if (fd >= 0) { write(fd, "300\n", 4); close(fd); }
    fd = open("/sys/class/backlight/panel/brightness", O_WRONLY);
    if (fd >= 0) { write(fd, "255\n", 4); close(fd); }
}

static void draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (!pixels) return;
    int x2 = x + w, y2 = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x2 > SCREEN_W) x2 = SCREEN_W;
    if (y2 > SCREEN_H) y2 = SCREEN_H;
    for (int py = y; py < y2; py++) {
        uint32_t *row = &pixels[py * (screen_pitch / 4) + x];
        for (int px = 0; px < (x2 - x); px++) {
            row[px] = color;
        }
    }
}

static void draw_rect_outline(int x, int y, int w, int h, int thick, uint32_t color) {
    draw_rect(x, y, w, thick, color);
    draw_rect(x, y + h - thick, w, thick, color);
    draw_rect(x, y, thick, h, color);
    draw_rect(x + w - thick, y, thick, h, color);
}

static void draw_char(unsigned char c, int x, int y, int scale, uint32_t color) {
    const unsigned char *glyph = &fontdata_8x16[c * 16];
    for (int row = 0; row < 16; row++) {
        unsigned char bits = glyph[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                draw_rect(x + col * scale, y + row * scale, scale, scale, color);
            }
        }
    }
}

static void draw_string(const char *str, int x, int y, int scale, uint32_t color) {
    int cur_x = x;
    while (*str) {
        draw_char((unsigned char)*str, cur_x, y, scale, color);
        cur_x += 8 * scale;
        str++;
    }
}

static void draw_string_centered(const char *str, int y, int scale, uint32_t color) {
    int len = strlen(str);
    int total_w = len * 8 * scale;
    int x = (SCREEN_W - total_w) / 2;
    draw_string(str, x, y, scale, color);
}

int main(void) {
    int drm_fd = -1;
    drmModeRes *res = NULL;
    drmModeConnector *conn = NULL;

    for (int wait = 0; wait < 50; wait++) {
        if (drm_fd < 0) {
            drm_fd = open("/dev/dri/card0", O_RDWR);
        }
        if (drm_fd >= 0) {
            int vf = open("/sys/class/drm/card0-Virtual-1/status", O_WRONLY);
            if (vf >= 0) { write(vf, "off\n", 4); close(vf); }

            int df = open("/sys/class/drm/card0-DSI-1/status", O_WRONLY);
            if (df >= 0) { write(df, "detect\n", 7); close(df); }

            set_backlight();
            drmSetMaster(drm_fd);
            res = drmModeGetResources(drm_fd);
            if (res && res->count_connectors > 0) {
                for (int i = 0; i < res->count_connectors; i++) {
                    drmModeConnector *c = drmModeGetConnector(drm_fd, res->connectors[i]);
                    if (!c) continue;
                    if (c->connector_type == DRM_MODE_CONNECTOR_DSI && c->count_modes > 0) {
                        conn = c;
                        break;
                    }
                    drmModeFreeConnector(c);
                }
                if (conn) break;
            }
            if (res) { drmModeFreeResources(res); res = NULL; }
        }
        usleep(100000);
    }

    if (!conn || !res || drm_fd < 0) {
        return 1;
    }

    set_backlight();
    drmModeModeInfo mode = conn->modes[0];

    uint32_t crtc_id = 0;
    if (conn->encoder_id) {
        drmModeEncoder *enc = drmModeGetEncoder(drm_fd, conn->encoder_id);
        if (enc) {
            if (enc->crtc_id) crtc_id = enc->crtc_id;
            else if (enc->possible_crtcs && res->count_crtcs > 0) {
                for (int c = 0; c < res->count_crtcs; c++) {
                    if (enc->possible_crtcs & (1 << c)) {
                        crtc_id = res->crtcs[c];
                        break;
                    }
                }
            }
            drmModeFreeEncoder(enc);
        }
    }
    if (!crtc_id && res->count_crtcs > 0) crtc_id = res->crtcs[0];

    struct drm_mode_create_dumb create;
    memset(&create, 0, sizeof(create));
    create.width = mode.hdisplay;
    create.height = mode.vdisplay;
    create.bpp = 32;
    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) return 1;

    uint32_t fb_id = 0;
    drmModeAddFB(drm_fd, mode.hdisplay, mode.vdisplay, 24, 32, create.pitch, create.handle, &fb_id);
    struct drm_mode_map_dumb map;
    memset(&map, 0, sizeof(map));
    map.handle = create.handle;
    if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) return 1;

    void *m = mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map.offset);
    if (m == MAP_FAILED) return 1;

    pixels = (uint32_t *)m;
    screen_pitch = create.pitch;

    // Render Debian Splash Dashboard BEFORE setting CRTC so initial frame has content
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 0xFF0A0F1D);
    draw_rect(60, 200, 960, 1600, 0xFF162032);
    draw_rect_outline(60, 200, 960, 1600, 6, 0xFF0284C7);

    draw_string_centered("DEBIAN LINUX 13", 280, 5, 0xFF38BDF8);
    draw_string_centered("ARM64 Bare Metal (Snapdragon 675)", 370, 3, 0xFF94A3B8);
    draw_rect(100, 440, 880, 4, 0xFF0284C7);

    draw_string("[+] STATUS: SYSTEMD RUNNING", 110, 520, 3, 0xFF22C55E);
    draw_string("[+] KERNEL: Linux 4.14 Native", 110, 600, 3, 0xFFF8FAFC);
    draw_string("[+] ROOTFS: Partition 30 (85 GB ext4)", 110, 680, 3, 0xFFF8FAFC);

    draw_rect(100, 760, 880, 4, 0xFF334155);

    draw_string("REMOTE ACCESS DETAILS:", 110, 820, 3, 0xFFFCD34D);
    draw_string("* ADB Access  : adb shell", 110, 900, 3, 0xFF38BDF8);
    draw_string("* SSH Forward : adb forward tcp:2222 tcp:22", 110, 980, 3, 0xFF38BDF8);
    draw_string("* SSH Access  : ssh root@localhost -p 2222", 110, 1060, 3, 0xFF38BDF8);
    draw_string("* Auth Method : Password root / SSH Key", 110, 1140, 3, 0xFFF8FAFC);

    draw_rect(100, 1240, 880, 4, 0xFF334155);
    draw_string_centered("Interactive Shell & SSH Ready", 1340, 3, 0xFF22C55E);
    draw_string_centered("Connected via Type-C to PC", 1420, 3, 0xFF94A3B8);

    drmModeSetCrtc(drm_fd, crtc_id, fb_id, 0, 0, &conn->connector_id, 1, &mode);
    set_backlight();
    force_usb_peripheral();

    // Keep process alive to hold DRM dumb buffer and backlight on
    while (1) {
        sleep(60);
        set_backlight();
    }
    return 0;
}
