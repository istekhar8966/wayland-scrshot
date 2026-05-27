#include "wayland.h"
#include "types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

int connect_to_wayland() {
    char path[256];
    const char *display = getenv("WAYLAND_DISPLAY");
    const char *runtime = getenv("XDG_RUNTIME_DIR");

    if (!display)
        display = "wayland-0";
    snprintf(path, sizeof(path), "%s/%s", runtime, display);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strcpy(addr.sun_path, path);

    if ((connect(fd, (struct sockaddr *)&addr, sizeof(addr))) != 0) {
        perror("wayland connection failed");
    } else {
        printf("Connected to socket: '%s'\n", path);
    }

    return fd;
}

int write_wayland_message(
    int fd,
    u32 object_id,
    u16 opcode,
    const void *args,
    u32 args_size) {

    // [4 bytes: object_id] [4 bytes: (size << 16 | opcode)] [arguments...]

    u32 total_size = 8 + args_size;

    void *message = malloc(total_size);
    if (message == NULL) {
        perror("malloc failed!");
    }
    u8 *ptr = (u8 *)message;

    // Write header
    *(u32 *)ptr = object_id;
    ptr += sizeof(u32);

    // size + opecode  (size << 16 | opcode)
    *(u32 *)ptr = (total_size << 16 | opcode);
    ptr += sizeof(u32);

    // Write args
    if (args_size > 0 && args != NULL) {
        memcpy(ptr, args, args_size);
    }

    // Write
    int n = write(fd, message, total_size);
    if (n == -1) {
        perror("write failed");
    }

    if (object_id == 1 && opcode == 1) {
        printf("Sent get_registry request...\n");
    }

    free(message);
    return n;
}
