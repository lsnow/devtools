/*
 * SECTION: radeon.c
 * @Title: radeon.c
 * @Short_Description:  
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <inttypes.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <libdrm/radeon_drm.h>
#include <unistd.h>

#define RENDER_PATH "/dev/dri/renderD"
#define CARD_PATH "/dev/dri/card"

#define NAME_SIZE 16

static int no_print = 0;

static int radeon_open (unsigned card)
{
    int fd;

    char render_path[22] = {0};
    char card_path[15] = {0};
    sprintf(render_path, "%s", RENDER_PATH);
    sprintf(card_path, "%s", CARD_PATH);
    sprintf(render_path + strlen(RENDER_PATH), "%d", 128 + card);
    card_path[strlen(CARD_PATH)] = '0' + card;

    fd = open(render_path, O_RDONLY);
    if (fd < 0)
    {
        perror(render_path);
        fd = open(card_path, O_RDONLY);
        if (fd < 0)
            perror(card_path);
        else
            fprintf(stderr, "%s: fd: %d\n", card_path, fd);
    }
    else
        fprintf(stderr, "%s: fd: %d\n", render_path, fd);

    return fd;
}

static int get_radeon_info (int fd, unsigned command, void *data)
{
    struct drm_radeon_info info;
    info.request = command;
    info.pad = 0;
    info.value = (__u64)data;

    if (ioctl(fd, DRM_IOCTL_RADEON_INFO, &info))
    {
        perror("DRM_IOCTL_RADEON_INFO");
        return 0;
    }
    return 1;
}

static __u64 get_gtt_usage (int fd)
{
    __u64 gtt_usage = 0;
    get_radeon_info(fd, RADEON_INFO_GTT_USAGE, (void *)&gtt_usage);

    if (!no_print)
        fprintf(stderr, "GTT usage: %llu MB\n", gtt_usage / 1024 / 1024);
    return gtt_usage;
}

static __u64 get_vram_usage (int fd)
{
    __u64 vram_usage = 0;
    get_radeon_info(fd, RADEON_INFO_VRAM_USAGE, (void *)&vram_usage);

    if (!no_print)
        fprintf(stderr, "VRAM usage: %llu MB\n", vram_usage / 1024 / 1024);
    return vram_usage;
}

static __u64 get_vram_size (int fd)
{
    struct drm_radeon_gem_info gem_info;
    if (ioctl(fd, DRM_IOCTL_RADEON_GEM_INFO, &gem_info))
    {
        perror("DRM_IOCTL_RADEON_GEM_INFO");
        return 0;
    }
    fprintf(stderr, "VRAM size: %llu MB\n", gem_info.vram_size / 1024 / 1024);
    fprintf(stderr, "GART size: %llu MB\n", gem_info.gart_size / 1024 / 1024);
    return gem_info.vram_size;
}

static float get_gpu_temp (int fd)
{
    __u64 temp = 0;
    get_radeon_info(fd, RADEON_INFO_CURRENT_GPU_TEMP, (void *)&temp);

    if (!no_print)
        fprintf(stderr, "gpu temp: %.2f \n", temp / 1000.0);
    return temp / 1000.0;
}

static void get_driver_name (int fd)
{
    struct drm_version v;
    memset(&v, 0, sizeof(v));
    char driver_name[NAME_SIZE] = {0}; //(char *)malloc(NAME_SIZE);
    v.name = driver_name;
    v.name_len = NAME_SIZE;
    if (ioctl(fd, DRM_IOCTL_VERSION, &v))
    {
        perror("DRM_IOCTL_VERSION");
        return;
    }
    fprintf(stderr, "driver name: %s\n", driver_name);
}

int main (int argc, char **argv)
{
    int fd;
    int card = 0;
    if (argc > 1)
        card = atoi(argv[1]);

    fd = radeon_open(card);
    if (fd < 0)
        return -1;

    get_driver_name (fd);
    get_vram_size (fd);

    no_print = 1;

    fprintf(stderr, "VRAM-usage   GTT-usage   temp\n");

    while (1)
    {
        __u64 vram_usage = get_vram_usage (fd) / 1024 / 1024;
        __u64 gtt_usage = get_gtt_usage (fd) / 1024 / 1024;
        float temp = get_gpu_temp (fd);
        fprintf(stderr, "%10llu   %9llu   %.2f\n", vram_usage, gtt_usage, temp);

        sleep(30);
    }

    return 0;
}
