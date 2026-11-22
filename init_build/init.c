#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

void write_file(const char *path, const char *val) {
    int fd = open(path, O_WRONLY);
    if (fd >= 0) {
        write(fd, val, strlen(val));
        close(fd);
    }
}

int main() {
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);
    mkdir("/dev", 0755);
    mkdir("/config", 0755);
    mkdir("/tmp", 0777);

    mount("proc", "/proc", "proc", 0, NULL);
    mount("sysfs", "/sys", "sysfs", 0, NULL);
    mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);

    write_file("/dev/kmsg", "[NATIVE-LINUX] Native ELF init starting...\n");
    write_file("/sys/class/timed_output/vibrator/enable", "300\n");
    write_file("/sys/class/backlight/panel0-backlight/brightness", "200\n");

    char *argv[] = {"/bin/busybox", "sh", "/init.sh", NULL};
    char *envp[] = {"PATH=/bin:/sbin:/usr/bin:/usr/sbin", "HOME=/root", "TERM=linux", NULL};

    execve("/bin/busybox", argv, envp);

    // Fallback if /bin/busybox fails
    char *fb_argv[] = {"/bin/sh", "/init.sh", NULL};
    execve("/bin/sh", fb_argv, envp);

    while (1) {
        sleep(1);
    }
    return 0;
}
