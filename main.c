#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
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

int a_round(int x) {
    return (x + 3) & ~3;
}

int main(void) {
    int fd;

    char sock_path[256];

    char *wayland_display;
    char *runtime_dir;

    wayland_display = getenv("WAYLAND_DISPLAY");
    runtime_dir = getenv("XDG_RUNTIME_DIR");

    snprintf(sock_path, sizeof(sock_path), "%s/%s", runtime_dir, wayland_display);

    struct sockaddr_un addr;

    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&addr, 0, sizeof(addr));

    addr.sun_family = AF_UNIX;

    strcpy(addr.sun_path, sock_path);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    /*
        object_id = 1
        size/opcode = (12 << 16) | 1
        new_id = 2
    */

    u32 packet[3] = {0};

    packet[0] = 1;
    packet[1] = (12 << 16) | 1;
    packet[2] = 2;

    write(fd, packet, sizeof(packet));

    int buf_cap = 4096;
    void *buffer = malloc(buf_cap);
    void *buf_ptr = buffer;

    int n = read(fd, (unsigned char *)buffer, buf_cap);

    while (buf_ptr < buffer + n) {
        int object_id = *(u32 *)buf_ptr;
        buf_ptr += sizeof(u32);

        int opcode = *(u16 *)buf_ptr;
        buf_ptr += sizeof(u16);

        int msg_size = *(u16 *)buf_ptr;
        buf_ptr += sizeof(u16);

        // printf("object_id: %u\n", object_id);
        // printf("opcode: %u\n", opcode);
        // printf("payload_size: %u\n", msg_size);

        int name = *(u32 *)buf_ptr;
        buf_ptr += sizeof(u32);

        int str_size = *(u32 *)buf_ptr;
        buf_ptr += sizeof(u32);

        printf("Global: ");
        printf("%d", name);
        printf(",");

        char *str_ptr = buf_ptr;

        for (int i = 0; i < str_size; i++) {
            printf("%c", *str_ptr);
            str_ptr += 1;
        }
        printf(",");

        buf_ptr += a_round(str_size);

        printf("v%u", *(u32 *)buf_ptr);
        buf_ptr += sizeof(u32);

        printf("\n");
    }

    free(buffer);
}
