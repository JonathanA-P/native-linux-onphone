#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "font_8x16.h"

#define SCREEN_W 1080
#define SCREEN_H 2400

static uint32_t *pixels = NULL;
static int screen_pitch = 0;

static inline void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        pixels[y * (screen_pitch / 4) + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    int x2 = x + w;
    int y2 = y + h;
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

void draw_rect_outline(int x, int y, int w, int h, int thick, uint32_t color) {
    draw_rect(x, y, w, thick, color);                     // Top
    draw_rect(x, y + h - thick, w, thick, color);         // Bottom
    draw_rect(x, y, thick, h, color);                     // Left
    draw_rect(x + w - thick, y, thick, h, color);         // Right
}

void draw_char(unsigned char c, int x, int y, int scale, uint32_t color) {
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

void draw_string(const char *str, int x, int y, int scale, uint32_t color) {
    int cur_x = x;
    while (*str) {
        draw_char((unsigned char)*str, cur_x, y, scale, color);
        cur_x += 8 * scale;
        str++;
    }
}

void draw_string_centered(const char *str, int y, int scale, uint32_t color) {
    int len = strlen(str);
    int total_w = len * 8 * scale;
    int x = (SCREEN_W - total_w) / 2;
    draw_string(str, x, y, scale, color);
}

void render_ui(int selected_os, int seconds_left) {
    // Background
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 0xFF0E131F);

    // Header Title
    draw_string_centered("SAMSUNG GALAXY A70", 180, 4, 0xFF38BDF8);
    draw_string_centered("DUAL BOOT LOADER", 260, 3, 0xFF94A3B8);
    draw_rect(100, 320, 880, 3, 0xFF1E293B);

    // Card 1: Debian Linux (Y: 420 to 860, H=440)
    int c1_bg = (selected_os == 0) ? 0xFF16253B : 0xFF121722;
    int c1_border = (selected_os == 0) ? 0xFF38BDF8 : 0xFF334155;
    int c1_thick = (selected_os == 0) ? 8 : 3;
    draw_rect(70, 420, 940, 440, c1_bg);
    draw_rect_outline(70, 420, 940, 440, c1_thick, c1_border);

    if (selected_os == 0) {
        draw_string("[ 1 ] DEBIAN LINUX", 110, 470, 4, 0xFF38BDF8);
        draw_string("Phosh GNOME Mobile Desktop", 110, 560, 3, 0xFFE2E8F0);
        draw_string("Kernel 4.14 / Dedicated 55GB Rootfs", 110, 630, 2, 0xFF94A3B8);
        draw_string(">> SELECTED <<", 110, 750, 4, 0xFF38BDF8);
    } else {
        draw_string("  1   DEBIAN LINUX", 110, 470, 4, 0xFF64748B);
        draw_string("Phosh GNOME Mobile Desktop", 110, 560, 3, 0xFF475569);
        draw_string("Kernel 4.14 / Dedicated 55GB Rootfs", 110, 630, 2, 0xFF334155);
    }

    // Card 2: Android 13 (Y: 920 to 1360, H=440)
    int c2_bg = (selected_os == 1) ? 0xFF122C20 : 0xFF121722;
    int c2_border = (selected_os == 1) ? 0xFF34D399 : 0xFF334155;
    int c2_thick = (selected_os == 1) ? 8 : 3;
    draw_rect(70, 920, 940, 440, c2_bg);
    draw_rect_outline(70, 920, 940, 440, c2_thick, c2_border);

    if (selected_os == 1) {
        draw_string("[ 2 ] ANDROID 13", 110, 970, 4, 0xFF34D399);
        draw_string("crDroid 9.12 Mobile OS", 110, 1060, 3, 0xFFE2E8F0);
        draw_string("LineageOS Android Platform (56GB)", 110, 1130, 2, 0xFF94A3B8);
        draw_string(">> SELECTED <<", 110, 1250, 4, 0xFF34D399);
    } else {
        draw_string("  2   ANDROID 13", 110, 970, 4, 0xFF64748B);
        draw_string("crDroid 9.12 Mobile OS", 110, 1060, 3, 0xFF475569);
        draw_string("LineageOS Android Platform (56GB)", 110, 1130, 2, 0xFF334155);
    }

    // Instruction Box
    draw_rect(70, 1440, 940, 360, 0xFF141923);
    draw_rect_outline(70, 1440, 940, 360, 2, 0xFF1E293B);
    draw_string("CONTROLS:", 110, 1480, 3, 0xFF94A3B8);
    draw_string("* Vol Up / Down : Switch System", 110, 1550, 2, 0xFFCBD5E1);
    draw_string("* Power Button  : Boot Selected OS", 110, 1610, 2, 0xFFCBD5E1);
    draw_string("* Touch Screen  : Tap to Select / Confirm", 110, 1670, 2, 0xFFCBD5E1);

    // Countdown / Timeout status
    if (seconds_left >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Auto-booting Debian in %d s...", seconds_left);
        draw_string_centered(buf, 1920, 3, 0xFFFCD34D);

        // Progress bar
        int bar_w = 880;
        int filled = (bar_w * seconds_left) / 5;
        if (filled < 0) filled = 0;
        draw_rect(100, 2000, bar_w, 24, 0xFF1E293B);
        draw_rect(100, 2000, filled, 24, 0xFF38BDF8);
    } else {
        draw_string_centered("Manual Selection Mode", 1920, 3, 0xFF94A3B8);
    }
}

int main(int argc, char **argv) {
    // Disable Virtual-1 connector if active
    int vf = open("/sys/class/drm/card0-Virtual-1/status", O_WRONLY);
    if (vf >= 0) {
        write(vf, "off\n", 4);
        close(vf);
    }

    // Turn on backlight
    int bl = open("/sys/class/backlight/panel0-backlight/brightness", O_WRONLY);
    if (bl < 0) bl = open("/sys/class/backlight/panel/brightness", O_WRONLY);
    if (bl >= 0) {
        write(bl, "250\n", 4);
        close(bl);
    }

    int drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) {
        perror("open /dev/dri/card0");
        return 0; // Fallback default to Linux
    }
    drmSetMaster(drm_fd);

    drmModeRes *res = drmModeGetResources(drm_fd);
    if (!res) {
        close(drm_fd);
        return 0;
    }

    drmModeConnector *conn = NULL;
    for (int i = 0; i < res->count_connectors; i++) {
        conn = drmModeGetConnector(drm_fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->connector_type == DRM_MODE_CONNECTOR_DSI) {
            break;
        }
        if (conn) drmModeFreeConnector(conn);
        conn = NULL;
    }

    if (!conn) {
        drmModeFreeResources(res);
        close(drm_fd);
        return 0;
    }

    drmModeModeInfo mode = conn->modes[0];
    uint32_t crtc_id = res->crtcs[0];

    struct drm_mode_create_dumb create = {
        .width = mode.hdisplay,
        .height = mode.vdisplay,
        .bpp = 32
    };
    drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);

    uint32_t fb_id = 0;
    drmModeAddFB(drm_fd, mode.hdisplay, mode.vdisplay, 24, 32, create.pitch, create.handle, &fb_id);

    struct drm_mode_map_dumb map = { .handle = create.handle };
    drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
    pixels = (uint32_t *)mmap(0, create.size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map.offset);
    screen_pitch = create.pitch;

    drmModeSetCrtc(drm_fd, crtc_id, fb_id, 0, 0, &conn->connector_id, 1, &mode);

    // Open input devices:
    // event0: qpnp_pon (Power=116, Vol-=114)
    // event5: gpio-keys (Vol+=115)
    // event2: sec_touchscreen
    int fd_pon = open("/dev/input/event0", O_RDONLY | O_NONBLOCK);
    int fd_volup = open("/dev/input/event5", O_RDONLY | O_NONBLOCK);
    int fd_touch = open("/dev/input/event2", O_RDONLY | O_NONBLOCK);

    struct pollfd pfds[3];
    int num_pfds = 0;
    if (fd_pon >= 0) { pfds[num_pfds].fd = fd_pon; pfds[num_pfds++].events = POLLIN; }
    if (fd_volup >= 0) { pfds[num_pfds].fd = fd_volup; pfds[num_pfds++].events = POLLIN; }
    if (fd_touch >= 0) { pfds[num_pfds].fd = fd_touch; pfds[num_pfds++].events = POLLIN; }

    int selected_os = 0; // 0 = Debian Linux, 1 = Android
    int seconds_left = 5;
    time_t last_time = time(NULL);

    render_ui(selected_os, seconds_left);

    int touch_x = -1, touch_y = -1;
    int chosen = -1;

    while (chosen < 0) {
        int ret = poll(pfds, num_pfds, 100);

        time_t now = time(NULL);
        if (seconds_left >= 0 && now != last_time) {
            seconds_left -= (now - last_time);
            last_time = now;
            if (seconds_left <= 0) {
                chosen = selected_os;
                break;
            }
            render_ui(selected_os, seconds_left);
        }

        if (ret <= 0) continue;

        for (int i = 0; i < num_pfds; i++) {
            if (!(pfds[i].revents & POLLIN)) continue;

            struct input_event evs[32];
            int n = read(pfds[i].fd, evs, sizeof(evs));
            if (n <= 0) continue;
            int count = n / sizeof(struct input_event);

            for (int k = 0; k < count; k++) {
                struct input_event *ev = &evs[k];

                // Keys: Power (116), Vol Down (114), Vol Up (115)
                if (ev->type == EV_KEY && ev->value == 1) { // Key Press
                    seconds_left = -1; // Cancel timeout on user activity

                    if (ev->code == KEY_VOLUMEDOWN || ev->code == KEY_VOLUMEUP) {
                        selected_os = 1 - selected_os;
                        render_ui(selected_os, seconds_left);
                    } else if (ev->code == KEY_POWER) {
                        chosen = selected_os;
                        break;
                    }
                }

                // Touchscreen events
                if (ev->type == EV_ABS) {
                    if (ev->code == ABS_MT_POSITION_X) touch_x = ev->value;
                    if (ev->code == ABS_MT_POSITION_Y) touch_y = ev->value;
                } else if (ev->type == EV_SYN && ev->code == SYN_REPORT) {
                    if (touch_x >= 0 && touch_y >= 0) {
                        seconds_left = -1; // Cancel timeout

                        // Tap on Card 1 (Debian Linux)
                        if (touch_x >= 70 && touch_x <= 1010 && touch_y >= 420 && touch_y <= 860) {
                            if (selected_os == 0) {
                                chosen = 0; // Tapped again -> boot!
                                break;
                            } else {
                                selected_os = 0;
                                render_ui(selected_os, seconds_left);
                            }
                        }
                        // Tap on Card 2 (Android 13)
                        else if (touch_x >= 70 && touch_x <= 1010 && touch_y >= 920 && touch_y <= 1360) {
                            if (selected_os == 1) {
                                chosen = 1; // Tapped again -> boot!
                                break;
                            } else {
                                selected_os = 1;
                                render_ui(selected_os, seconds_left);
                            }
                        }
                        touch_x = -1;
                        touch_y = -1;
                    }
                }
            }
            if (chosen >= 0) break;
        }
    }

    // Cleanup DRM before leaving
    munmap(pixels, create.size);
    drmModeRmFB(drm_fd, fb_id);
    drmModeFreeConnector(conn);
    drmModeFreeResources(res);
    drmDropMaster(drm_fd);
    close(drm_fd);

    if (fd_pon >= 0) close(fd_pon);
    if (fd_volup >= 0) close(fd_volup);
    if (fd_touch >= 0) close(fd_touch);

    printf("Boot selector chosen: %d (%s)\n", chosen, chosen == 0 ? "Debian Linux" : "Android");
    return chosen;
}
