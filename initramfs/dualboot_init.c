#define _GNU_SOURCE
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <linux/input.h>
#include <linux/loop.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "font_8x16.h"

#define SCREEN_W 1080
#define SCREEN_H 2400

static uint32_t *pixels = NULL;
static int screen_pitch = 0;
static void populate_input_devs(void);

void klog(const char *msg) {
    int fd = open("/dev/kmsg", O_WRONLY);
    if (fd >= 0) {
        write(fd, msg, strlen(msg));
        close(fd);
    }
}

void vibrate(int ms) {
    int vib = open("/sys/class/timed_output/vibrator/enable", O_WRONLY);
    if (vib >= 0) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d\n", ms);
        write(vib, buf, strlen(buf));
        close(vib);
    }
}

void set_backlight(void) {
    int fd = open("/sys/class/backlight/backlight/brightness", O_WRONLY);
    if (fd >= 0) {
        write(fd, "3500\n", 5);
        close(fd);
    }
    fd = open("/sys/class/backlight/backlight/bl_power", O_WRONLY);
    if (fd >= 0) {
        write(fd, "0\n", 2);
        close(fd);
    }
    fd = open("/sys/class/backlight/panel0-backlight/brightness", O_WRONLY);
    if (fd >= 0) {
        write(fd, "300\n", 4);
        close(fd);
    }
    fd = open("/sys/class/backlight/panel/brightness", O_WRONLY);
    if (fd >= 0) {
        write(fd, "255\n", 4);
        close(fd);
    }
}

static inline void put_pixel(int x, int y, uint32_t color) {
    if (pixels && x >= 0 && x < SCREEN_W && y >= 0 && y < SCREEN_H) {
        pixels[y * (screen_pitch / 4) + x] = color;
    }
}

void draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (!pixels) return;
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

static void render_debian_splash(void) {
    if (!pixels) return;
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
    draw_string("* USB Network : 192.168.7.2", 110, 900, 3, 0xFF38BDF8);
    draw_string("* SSH Access  : ssh root@192.168.7.2", 110, 980, 3, 0xFF38BDF8);
    draw_string("* Password    : root", 110, 1060, 3, 0xFFF8FAFC);
    draw_string("* Serial TTY  : /dev/ttyACM0 (115200)", 110, 1140, 3, 0xFF38BDF8);

    draw_rect(100, 1240, 880, 4, 0xFF334155);
    draw_string_centered("Headless Remote Mode Active", 1340, 3, 0xFFFCD34D);
    draw_string_centered("Connect via USB cable to PC", 1420, 3, 0xFF94A3B8);
}

void render_ui(int selected_os, int seconds_left) {
    if (!pixels) return;
    draw_rect(0, 0, SCREEN_W, SCREEN_H, 0xFF0A0F1D);

    draw_string_centered("BOOT SELECTOR", 160, 4, 0xFF38BDF8);
    draw_string_centered("Select Operating System", 240, 3, 0xFF94A3B8);
    draw_rect(100, 300, 880, 3, 0xFF1E293B);

    // Card 1: Debian Linux (Y: 360 to 860, H=500)
    int c1_bg = (selected_os == 0) ? 0xFF0284C7 : 0xFF162032;
    int c1_border = (selected_os == 0) ? 0xFFFFFFFF : 0xFF334155;
    int c1_thick = (selected_os == 0) ? 10 : 3;
    draw_rect(60, 360, 960, 500, c1_bg);
    draw_rect_outline(60, 360, 960, 500, c1_thick, c1_border);

    if (selected_os == 0) {
        draw_rect(60, 360, 960, 90, 0xFF0369A1);
        draw_string("[ * ACTIVE * ] 1. DEBIAN LINUX", 90, 390, 4, 0xFFFFFFFF);
        draw_string("Native Debian 13 (Trixie)", 90, 500, 3, 0xFFFFFFFF);
        draw_string("Partition 30 (85 GB ext4) - SSH/ACM", 90, 580, 3, 0xFFF0FDF4);
        draw_rect(90, 700, 900, 80, 0xFF0C4A6E);
        draw_string_centered(">>> PRESS POWER TO BOOT DEBIAN <<<", 725, 3, 0xFFFDE047);
    } else {
        draw_string("  1   DEBIAN LINUX", 110, 440, 4, 0xFF64748B);
        draw_string("Native Debian 13 (Trixie)", 110, 530, 3, 0xFF475569);
        draw_string("Partition 30 (85 GB ext4)", 110, 610, 2, 0xFF334155);
    }

    // Card 2: Android 13 (Y: 920 to 1420, H=500)
    int c2_bg = (selected_os == 1) ? 0xFF16A34A : 0xFF162032;
    int c2_border = (selected_os == 1) ? 0xFFFFFFFF : 0xFF334155;
    int c2_thick = (selected_os == 1) ? 10 : 3;
    draw_rect(60, 920, 960, 500, c2_bg);
    draw_rect_outline(60, 920, 960, 500, c2_thick, c2_border);

    if (selected_os == 1) {
        draw_rect(60, 920, 960, 90, 0xFF15803D);
        draw_string("[ * ACTIVE * ] 2. ANDROID 13", 90, 950, 4, 0xFFFFFFFF);
        draw_string("crDroid 9.12 Mobile OS", 90, 1060, 3, 0xFFFFFFFF);
        draw_string("Partition 29 (32 GB f2fs)", 90, 1140, 3, 0xFFF0FDF4);
        draw_rect(90, 1260, 900, 80, 0xFF14532D);
        draw_string_centered(">>> PRESS POWER TO BOOT ANDROID <<<", 1285, 3, 0xFFFDE047);
    } else {
        draw_string("  2   ANDROID 13", 110, 1000, 4, 0xFF64748B);
        draw_string("crDroid 9.12 Mobile OS", 110, 1090, 3, 0xFF475569);
        draw_string("Partition 29 (32 GB f2fs)", 110, 1170, 2, 0xFF334155);
    }

    draw_rect(60, 1480, 960, 340, 0xFF111827);
    draw_rect_outline(60, 1480, 960, 340, 2, 0xFF1E293B);
    draw_string("CONTROLS:", 90, 1520, 3, 0xFFFCD34D);
    draw_string("* Volume Up / Down : Switch Selection", 90, 1590, 3, 0xFFF1F5F9);
    draw_string("* Power Button     : Boot Selected OS", 90, 1660, 3, 0xFFF1F5F9);
    draw_string("* Touch Screen     : Tap Card to Select", 90, 1730, 3, 0xFFF1F5F9);

    if (seconds_left >= 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Auto-booting in %d s...", seconds_left);
        draw_string_centered(buf, 1920, 3, 0xFFFCD34D);

        int bar_w = 880;
        int filled = (bar_w * seconds_left) / 6;
        if (filled < 0) filled = 0;
        draw_rect(100, 1980, bar_w, 24, 0xFF1E293B);
        draw_rect(100, 1980, filled, 24, 0xFF38BDF8);
    } else {
        draw_string_centered("Manual Selection Mode", 1920, 3, 0xFF34D399);
    }
}

int select_os_gui(void) {
    klog("[DUALBOOT] select_os_gui starting...\n");

    int drm_fd = -1;
    drmModeRes *res = NULL;
    drmModeConnector *conn = NULL;

    // Loop up to 5 seconds waiting for DRM and DSI panel connector
    for (int wait = 0; wait < 50; wait++) {
        if (drm_fd < 0) {
            drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
        }
        if (drm_fd >= 0) {
            // Disable Virtual-1 auxiliary connector
            int vf = open("/sys/class/drm/card0-Virtual-1/status", O_WRONLY);
            if (vf >= 0) { write(vf, "off\n", 4); close(vf); }

            // Trigger DSI hardware probe
            int df = open("/sys/class/drm/card0-DSI-1/status", O_WRONLY);
            if (df >= 0) { write(df, "detect\n", 7); close(df); }

            set_backlight();

            drmSetMaster(drm_fd);
            res = drmModeGetResources(drm_fd);
            if (res && res->count_connectors > 0) {
                // Look for DSI connector
                for (int i = 0; i < res->count_connectors; i++) {
                    drmModeConnector *c = drmModeGetConnector(drm_fd, res->connectors[i]);
                    if (!c) continue;
                    if (c->connector_type == DRM_MODE_CONNECTOR_DSI && c->count_modes > 0) {
                        conn = c;
                        break;
                    }
                    drmModeFreeConnector(c);
                }
                if (conn) break; // Found physical AMOLED screen!
            }
            if (res) { drmModeFreeResources(res); res = NULL; }
        }
        usleep(100000);
    }

    if (!conn || !res || drm_fd < 0) {
        klog("[DUALBOOT] WARNING: DSI display not ready, defaulting to Debian Linux\n");
        if (conn) drmModeFreeConnector(conn);
        if (res) drmModeFreeResources(res);
        if (drm_fd >= 0) close(drm_fd);
        return 0; // Default to Linux
    }

    set_backlight();

    drmModeModeInfo mode = conn->modes[0];

    // Find CRTC
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
    if (!crtc_id && res->count_crtcs > 0) {
        crtc_id = res->crtcs[0];
    }

    struct drm_mode_create_dumb create[2];
    uint32_t fb_id[2] = {0, 0};
    uint32_t *pixels_buf[2] = {NULL, NULL};
    int pitches[2] = {0, 0};
    int cur_buf = 0;

    for (int b = 0; b < 2; b++) {
        memset(&create[b], 0, sizeof(create[b]));
        create[b].width = mode.hdisplay;
        create[b].height = mode.vdisplay;
        create[b].bpp = 32;
        if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create[b]) == 0) {
            drmModeAddFB(drm_fd, mode.hdisplay, mode.vdisplay, 24, 32, create[b].pitch, create[b].handle, &fb_id[b]);
            struct drm_mode_map_dumb map;
            memset(&map, 0, sizeof(map));
            map.handle = create[b].handle;
            if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map) == 0) {
                void *m = mmap(0, create[b].size, PROT_READ | PROT_WRITE, MAP_SHARED, drm_fd, map.offset);
                if (m != MAP_FAILED) {
                    pixels_buf[b] = (uint32_t *)m;
                    pitches[b] = create[b].pitch;
                }
            }
        }
    }

#define FLIP_UI(os, sec) do { \
    int next_b = (fb_id[1] != 0) ? (1 - cur_buf) : 0; \
    pixels = pixels_buf[next_b]; \
    screen_pitch = pitches[next_b]; \
    render_ui(os, sec); \
    if (fb_id[next_b] != 0 && crtc_id != 0) { \
        drmModeSetCrtc(drm_fd, crtc_id, fb_id[next_b], 0, 0, &conn->connector_id, 1, &mode); \
    } \
    cur_buf = next_b; \
} while(0)

#define FLIP_SPLASH() do { \
    int next_b = (fb_id[1] != 0) ? (1 - cur_buf) : 0; \
    pixels = pixels_buf[next_b]; \
    screen_pitch = pitches[next_b]; \
    render_debian_splash(); \
    if (fb_id[next_b] != 0 && crtc_id != 0) { \
        drmModeSetCrtc(drm_fd, crtc_id, fb_id[next_b], 0, 0, &conn->connector_id, 1, &mode); \
    } \
    cur_buf = next_b; \
} while(0)

    set_backlight();
    klog("[DUALBOOT] Dual-boot menu rendering on screen!\n");

    populate_input_devs();

    // Open all input event devices
    int event_fds[32];
    int num_events = 0;
    for (int i = 0; i < 32; i++) {
        char devpath[32];
        snprintf(devpath, sizeof(devpath), "/dev/input/event%d", i);
        int fd = open(devpath, O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
            event_fds[num_events++] = fd;
        }
    }

    struct pollfd pfds[32];
    for (int i = 0; i < num_events; i++) {
        pfds[i].fd = event_fds[i];
        pfds[i].events = POLLIN;
    }

    int selected_os = 0; // 0 = Linux, 1 = Android
    int seconds_left = 6;
    time_t last_time = time(NULL);

    FLIP_UI(selected_os, seconds_left);

    int touch_x = -1, touch_y = -1;
    int chosen = -1;

    while (chosen < 0) {
        // Dynamically add newly registered input devices
        if (num_events < 16) {
            for (int i = num_events; i < 16; i++) {
                char devpath[32];
                snprintf(devpath, sizeof(devpath), "/dev/input/event%d", i);
                int fd = open(devpath, O_RDONLY | O_NONBLOCK);
                if (fd >= 0) {
                    event_fds[num_events] = fd;
                    pfds[num_events].fd = fd;
                    pfds[num_events].events = POLLIN;
                    num_events++;
                }
            }
        }

        int ret = poll(pfds, num_events, 100);

        time_t now = time(NULL);
        if (seconds_left >= 0 && now != last_time) {
            seconds_left -= (now - last_time);
            last_time = now;
            if (seconds_left <= 0) {
                chosen = selected_os;
                break;
            }
            FLIP_UI(selected_os, seconds_left);
        }

        if (ret <= 0) continue;

        for (int i = 0; i < num_events; i++) {
            if (!(pfds[i].revents & POLLIN)) continue;

            struct input_event evs[32];
            int n = read(pfds[i].fd, evs, sizeof(evs));
            if (n <= 0) continue;
            int count = n / sizeof(struct input_event);

            for (int k = 0; k < count; k++) {
                struct input_event *ev = &evs[k];

                if (ev->type == EV_KEY && (ev->value == 1 || ev->value == 2)) {
                    seconds_left = -1;
                    if (ev->code == KEY_VOLUMEDOWN || ev->code == KEY_DOWN) {
                        selected_os = (selected_os == 0) ? 1 : 0;
                        vibrate(100);
                        FLIP_UI(selected_os, seconds_left);
                    } else if (ev->code == KEY_VOLUMEUP || ev->code == KEY_UP) {
                        selected_os = (selected_os == 1) ? 0 : 1;
                        vibrate(100);
                        FLIP_UI(selected_os, seconds_left);
                    } else if (ev->code == KEY_POWER || ev->code == KEY_ENTER) {
                        vibrate(200);
                        chosen = selected_os;
                        break;
                    }
                }

                if (ev->type == EV_ABS) {
                    if (ev->code == ABS_MT_POSITION_X) touch_x = ev->value;
                    if (ev->code == ABS_MT_POSITION_Y) touch_y = ev->value;
                } else if (ev->type == EV_SYN && ev->code == SYN_REPORT) {
                    if (touch_x >= 0 && touch_y >= 0) {
                        seconds_left = -1;
                        if (touch_x >= 70 && touch_x <= 1010 && touch_y >= 360 && touch_y <= 860) {
                            if (selected_os == 0) {
                                vibrate(200);
                                chosen = 0;
                                break;
                            } else {
                                selected_os = 0;
                                vibrate(100);
                                FLIP_UI(selected_os, seconds_left);
                            }
                        } else if (touch_x >= 70 && touch_x <= 1010 && touch_y >= 920 && touch_y <= 1420) {
                            if (selected_os == 1) {
                                vibrate(200);
                                chosen = 1;
                                break;
                            } else {
                                selected_os = 1;
                                vibrate(100);
                                FLIP_UI(selected_os, seconds_left);
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

    if (chosen != 0) {
        for (int b = 0; b < 2; b++) {
            if (pixels_buf[b]) munmap(pixels_buf[b], create[b].size);
            if (fb_id[b]) drmModeRmFB(drm_fd, fb_id[b]);
        }
        pixels = NULL;
        if (conn) drmModeFreeConnector(conn);
        if (res) drmModeFreeResources(res);
        drmDropMaster(drm_fd);
        close(drm_fd);
    } else {
        FLIP_SPLASH();
        set_backlight();
        drmDropMaster(drm_fd);
        close(drm_fd);
    }

    for (int i = 0; i < num_events; i++) {
        close(event_fds[i]);
    }

    return chosen;
}

void boot_android(void) {
    klog("[DUALBOOT] Preparing Android 13 handoff...\n");
    vibrate(100);

    // If init.magisk exists, restore it as /init
    if (access("/init.magisk", F_OK) == 0) {
        unlink("/init");
        rename("/init.magisk", "/init");
        chmod("/init", 0750);
    }

    // Unmount early filesystems so Magisk init starts clean
    umount2("/dev", MNT_DETACH);
    umount2("/sys", MNT_DETACH);
    umount2("/proc", MNT_DETACH);

    char *a_argv[] = {"/init", NULL};
    char *a_envp[] = {"PATH=/sbin:/system/bin:/system/xbin", "TERM=linux", NULL};

    execve("/init", a_argv, a_envp);
    klog("[DUALBOOT] FATAL: execve /init failed!\n");
    while (1) {
        sleep(10);
    }
}

static void populate_block_devs(const char *root) {
    char dirpath[64];
    snprintf(dirpath, sizeof(dirpath), "%s/dev/block", root);
    mkdir(dirpath, 0755);

    DIR *d = opendir("/sys/class/block");
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.') continue;
        char devfile[128];
        snprintf(devfile, sizeof(devfile), "/sys/class/block/%s/dev", de->d_name);
        int fd = open(devfile, O_RDONLY);
        if (fd < 0) continue;
        char buf[32] = {0};
        int r = read(fd, buf, sizeof(buf) - 1);
        close(fd);
        if (r <= 0) continue;
        int maj = 0, min = 0;
        if (sscanf(buf, "%d:%d", &maj, &min) == 2) {
            char p1[128], p2[128];
            snprintf(p1, sizeof(p1), "%s/dev/%s", root, de->d_name);
            snprintf(p2, sizeof(p2), "%s/dev/block/%s", root, de->d_name);
            unlink(p1);
            unlink(p2);
            mknod(p1, S_IFBLK | 0660, makedev(maj, min));
            mknod(p2, S_IFBLK | 0660, makedev(maj, min));
        }
    }
    closedir(d);
}

static void populate_input_devs(void) {
    mkdir("/dev/input", 0755);
    DIR *d = opendir("/sys/class/input");
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (strncmp(de->d_name, "event", 5) != 0) continue;
            char devfile[128];
            snprintf(devfile, sizeof(devfile), "/sys/class/input/%s/dev", de->d_name);
            int fd = open(devfile, O_RDONLY);
            if (fd < 0) continue;
            char buf[32] = {0};
            int r = read(fd, buf, sizeof(buf) - 1);
            close(fd);
            if (r <= 0) continue;
            int maj = 0, min = 0;
            if (sscanf(buf, "%d:%d", &maj, &min) == 2) {
                char p[64];
                snprintf(p, sizeof(p), "/dev/input/%s", de->d_name);
                unlink(p);
                mknod(p, S_IFCHR | 0660, makedev(maj, min));
            }
        }
        closedir(d);
    }
    for (int i = 0; i < 32; i++) {
        char p[32];
        snprintf(p, sizeof(p), "/dev/input/event%d", i);
        if (access(p, F_OK) != 0) {
            mknod(p, S_IFCHR | 0660, makedev(13, 64 + i));
        }
    }
}

static int check_misc_flag(void) {
    populate_block_devs("");
    int fd = open("/dev/block/sda10", O_RDWR);
    if (fd < 0) fd = open("/dev/sda10", O_RDWR);
    if (fd < 0) return -1;

    char buf[16] = {0};
    if (lseek(fd, 2048, SEEK_SET) == 2048) {
        if (read(fd, buf, 16) == 16) {
            if (strncmp(buf, "BOOT_LINUX", 10) == 0) {
                memset(buf, 0, 16);
                lseek(fd, 2048, SEEK_SET);
                write(fd, buf, 16);
                close(fd);
                return 0; // Linux
            } else if (strncmp(buf, "BOOT_ANDROID", 12) == 0) {
                memset(buf, 0, 16);
                lseek(fd, 2048, SEEK_SET);
                write(fd, buf, 16);
                close(fd);
                return 1; // Android
            }
        }
    }
    close(fd);
    return -1;
}

static int check_hw_keys(void) {
    populate_input_devs();
    for (int i = 0; i < 32; i++) {
        char p[32];
        snprintf(p, sizeof(p), "/dev/input/event%d", i);
        int fd = open(p, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        uint8_t key_b[KEY_MAX / 8 + 1] = {0};
        if (ioctl(fd, EVIOCGKEY(sizeof(key_b)), key_b) >= 0) {
            if (key_b[KEY_VOLUMEDOWN / 8] & (1 << (KEY_VOLUMEDOWN % 8))) {
                close(fd);
                return 0; // Vol Down -> Linux
            }
            if (key_b[KEY_VOLUMEUP / 8] & (1 << (KEY_VOLUMEUP % 8))) {
                close(fd);
                return 1; // Vol Up -> Android
            }
        }
        close(fd);
    }
    return -1;
}

static void force_usb_peripheral(void) {
    const char *paths[] = {
        "/sys/devices/platform/soc/a600000.ssusb",
        "/sys/bus/platform/devices/a600000.ssusb",
        "/sysroot/sys/devices/platform/soc/a600000.ssusb",
        "/sysroot/sys/bus/platform/devices/a600000.ssusb",
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

void boot_linux(void) {
    klog("[DUALBOOT] Booting Debian Linux natively from partition 30...\n");
    vibrate(100);

    mkdir("/sysroot", 0755);

    int mnt_res = -1;
    for (int wait = 0; wait < 60; wait++) {
        populate_block_devs("");
        mnt_res = mount("/dev/block/sda30", "/sysroot", "ext4", MS_NOATIME, NULL);
        if (mnt_res == 0) break;
        mnt_res = mount("/dev/sda30", "/sysroot", "ext4", MS_NOATIME, NULL);
        if (mnt_res == 0) break;
        usleep(100000);
    }

    if (mnt_res != 0) {
        klog("[DUALBOOT] ERROR: Failed to mount /dev/block/sda30! Falling back to Android.\n");
        boot_android();
        return;
    }

    klog("[DUALBOOT] Mounted /dev/block/sda30 on /sysroot successfully!\n");
    vibrate(150);

    mkdir("/sysroot/dev", 0755);
    mkdir("/sysroot/proc", 0755);
    mkdir("/sysroot/sys", 0755);

    mount("proc", "/sysroot/proc", "proc", 0, NULL);
    mount("sysfs", "/sysroot/sys", "sysfs", 0, NULL);
    force_usb_peripheral();

    if (mount("devtmpfs", "/sysroot/dev", "devtmpfs", 0, "mode=0755") != 0) {
        mount("tmpfs", "/sysroot/dev", "tmpfs", 0, "mode=0755");
        mknod("/sysroot/dev/null", S_IFCHR | 0666, makedev(1, 3));
        mknod("/sysroot/dev/zero", S_IFCHR | 0666, makedev(1, 5));
        mknod("/sysroot/dev/urandom", S_IFCHR | 0666, makedev(1, 9));
        mknod("/sysroot/dev/kmsg", S_IFCHR | 0666, makedev(1, 11));
        mknod("/sysroot/dev/console", S_IFCHR | 0600, makedev(5, 1));
        mknod("/sysroot/dev/tty", S_IFCHR | 0666, makedev(5, 0));
        mknod("/sysroot/dev/ptmx", S_IFCHR | 0666, makedev(5, 2));
        mkdir("/sysroot/dev/pts", 0755);
        mkdir("/sysroot/dev/shm", 01777);
        mkdir("/sysroot/dev/dri", 0755);
        mknod("/sysroot/dev/dri/card0", S_IFCHR | 0666, makedev(226, 0));
        mknod("/sysroot/dev/dri/renderD128", S_IFCHR | 0666, makedev(226, 128));
        mkdir("/sysroot/dev/input", 0755);
        for (int i = 0; i < 32; i++) {
            char p[32];
            snprintf(p, sizeof(p), "/sysroot/dev/input/event%d", i);
            mknod(p, S_IFCHR | 0660, makedev(13, 64 + i));
        }
        populate_block_devs("/sysroot");
    }

    // Remount /sysroot/dev to clear MS_NODEV so unprivileged daemons (messagebus, polkitd) can access /dev/null
    mount(NULL, "/sysroot/dev", NULL, MS_REMOUNT, NULL);

    // Ensure mount propagation is private so moving root is permitted
    mount(NULL, "/", NULL, MS_PRIVATE | MS_REC, NULL);
    mount(NULL, "/sysroot", NULL, MS_PRIVATE | MS_REC, NULL);

    // Native switch_root
    if (chdir("/sysroot") != 0) {
        klog("[DUALBOOT] chdir /sysroot failed: %m\n");
    }
    if (mount(".", "/", NULL, MS_MOVE, NULL) != 0) {
        klog("[DUALBOOT] MS_MOVE failed: %m\n");
    }
    if (chroot(".") != 0) {
        klog("[DUALBOOT] chroot failed: %m\n");
    }
    chdir("/");
    mount(NULL, "/dev", NULL, MS_REMOUNT, NULL);

    // Open /dev/console on fd 0, 1, 2
    int cfd = open("/dev/console", O_RDWR);
    if (cfd >= 0) {
        dup2(cfd, 0);
        dup2(cfd, 1);
        dup2(cfd, 2);
        if (cfd > 2) close(cfd);
    }

    char *sw_argv[] = {"/lib/systemd/systemd", NULL};
    char *sw_envp[] = {
        "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin",
        "HOME=/root",
        "TERM=linux",
        NULL
    };

    klog("[DUALBOOT] Executing /lib/systemd/systemd...\n");
    execve("/lib/systemd/systemd", sw_argv, sw_envp);

    klog("[DUALBOOT] Executing /usr/lib/systemd/systemd...\n");
    execve("/usr/lib/systemd/systemd", sw_argv, sw_envp);

    char *sw2_argv[] = {"/sbin/init", NULL};
    klog("[DUALBOOT] Executing /sbin/init...\n");
    execve("/sbin/init", sw2_argv, sw_envp);

    char *sh_argv[] = {"/bin/sh", NULL};
    klog("[DUALBOOT] Executing /bin/sh...\n");
    execve("/bin/sh", sh_argv, sw_envp);

    klog("[DUALBOOT] FATAL: All init executables failed to start! PID 1 rescue loop...\n");
    while (1) {
        pid_t p = fork();
        if (p == 0) {
            execve("/bin/sh", sh_argv, sw_envp);
            _exit(1);
        }
        waitpid(p, NULL, 0);
        sleep(2);
    }
}

int main(int argc, char **argv) {
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("tmpfs", "/dev", "tmpfs", 0, "mode=0755");

    mknod("/dev/null", S_IFCHR | 0666, makedev(1, 3));
    mknod("/dev/zero", S_IFCHR | 0666, makedev(1, 5));
    mknod("/dev/kmsg", S_IFCHR | 0666, makedev(1, 11));
    mknod("/dev/console", S_IFCHR | 0600, makedev(5, 1));
    mknod("/dev/tty", S_IFCHR | 0666, makedev(5, 0));

    mkdir("/dev/dri", 0755);
    mknod("/dev/dri/card0", S_IFCHR | 0666, makedev(226, 0));
    mknod("/dev/dri/controlD64", S_IFCHR | 0666, makedev(226, 64));
    mknod("/dev/dri/renderD128", S_IFCHR | 0666, makedev(226, 128));

    populate_input_devs();
    populate_block_devs("");

    if (access("/bin/busybox", X_OK) == 0) {
        pid_t pid = fork();
        if (pid == 0) {
            char *m_argv[] = {"/bin/busybox", "mdev", "-s", NULL};
            execve("/bin/busybox", m_argv, NULL);
            _exit(1);
        }
        waitpid(pid, NULL, 0);
    }

    klog("[DUALBOOT] Native ELF init starting...\n");

    // 1. Check persistent boot flag from misc partition
    int flag = check_misc_flag();
    if (flag == 0) {
        klog("[DUALBOOT] BOOT_LINUX flag set. Booting Linux...\n");
        boot_linux();
        return 0;
    } else if (flag == 1) {
        klog("[DUALBOOT] BOOT_ANDROID flag set. Booting Android...\n");
        boot_android();
        return 0;
    }

    vibrate(200);

    // 2. Show visual selection UI
    int choice = select_os_gui();
    klog(choice == 0 ? "[DUALBOOT] Selected: Debian Linux\n" : "[DUALBOOT] Selected: Android 13\n");

    if (choice <= 0) {
        boot_linux();
    } else {
        boot_android();
    }

    klog("[DUALBOOT] Emergency halt.\n");
    while (1) {
        sleep(10);
    }
    return 0;
}
