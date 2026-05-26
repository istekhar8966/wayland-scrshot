#define _GNU_SOURCE

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

/* Unsigned integer types */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

/* Signed integer types */
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/* Floating point types */
typedef float f32;
typedef double f64;

#define ROUND_UP_4(x) (((x) + 3) & ~3)

// wayland message format!
// [4 bytes: object_id] [4 bytes: (size << 16 | opcode)] [arguments...]

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

void send_get_registry(int fd, u32 new_registry_id) {
    u32 message[3] = {
        1,
        (12 << 16) | 1,
        new_registry_id,
    };

    write(fd, message, sizeof(message));
}

void write_wayland_message(
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
        return;
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

    if (object_id == 1 && opcode == 2) {
        printf("Sent get_registry request...\n");
    }

    free(message);
}

void process_event(u8 **ptr, u8 *end) {
    // header
    u32 object_id = *(u32 *)*ptr;
    *ptr += sizeof(u32);
    u32 size_opcode = *(u32 *)*ptr;
    *ptr += sizeof(u32);

    u16 size = size_opcode >> 16;
    u16 opcode = size_opcode & 0xFFFF;

    printf("[Event] object=%u, opcode=%u, size=%u\n", object_id, opcode, size);

    if (opcode == 0) {
        u32 name = *(u32 *)*ptr;
        *ptr += sizeof(u32);

        u32 len = *(u32 *)*ptr;
        *ptr += sizeof(u32);

        printf("    -> Global name=%u", name);

        printf(", interface='");
        for (int i = 0; i < len && (*ptr + 1) < end; i++) {
            if (*ptr[i] > 33 && *ptr[i] < 127) {
                printf("%c", *ptr[i]);
            }
        }
        printf("'");

        *ptr += ROUND_UP_4(len);
        u32 version = *(u32 *)*ptr;
        *ptr += sizeof(u32);
        printf(", version=%u\n", version);
    }
}

void print_buffer(const void *buf, size_t len) {
    const unsigned char *p = buf;

    for (size_t i = 0; i < len; i++) {

        if (p[i] >= 32 && p[i] < 127) {
            printf("%c", p[i]);
        } else {
            printf(".");
        }
    }

    printf("\n");
}

void read_and_process_events(int fd) {
    u8 buffer[4096];
    size_t n = read(fd, buffer, 500 * sizeof(u32));

    if (n <= 0)
        return;

    u8 *ptr = buffer;
    u8 *end = buffer + n;

    while (ptr + 8 <= end) {
        process_event(&ptr, end);
    }
}

int main() {
    int fd = connect_to_wayland();

    u32 args[1] = {2};
    write_wayland_message(fd, 1, 2, &args, sizeof(args));

    read_and_process_events(fd);

    close(fd);
}
