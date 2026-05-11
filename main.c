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

    int mem_fd = memfd_create("memfd_buffer", 0);
    int fd_buf_size = 1920 * 1080 * 4;

    ftruncate(mem_fd, fd_buf_size);

    void *pixels = mmap(NULL, fd_buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);

    if (pixels == MAP_FAILED) {
        perror("mmap Failed!");
        exit(1);
    }

    /*
    packet_format!
     *
    [object_id] = 2
    [opcode/size] = (size << 16) | opcode
    [global_name]
    [string_length]
    [string bytes]
    [padding]
    [version]
    [new_object_id]
    *
    */

    void *packet_pointer = malloc(4096);
    void *new_ptr = packet_pointer;

    u32 object_id = 2;

    memcpy(new_ptr, &object_id, sizeof(u32));
    new_ptr += sizeof(u32);

    u16 opcode = 0;
    memcpy(new_ptr, &opcode, sizeof(u16));
    new_ptr += sizeof(u16);

    void *msg_size_ptr = new_ptr;
    new_ptr += sizeof(u16);

    u32 global_name = 5;
    memcpy(new_ptr, &global_name, sizeof(u32));
    new_ptr += sizeof(u32);

    char *str = "wl_shm";
    int str_size = strlen(str) + 1;

    memcpy(new_ptr, &str_size, sizeof(u32));
    new_ptr += sizeof(u32);

    memcpy(new_ptr, str, str_size);
    new_ptr += str_size;

    u32 padding = a_round(str_size) - str_size;
    new_ptr += padding;

    u32 version = 2;
    memcpy(new_ptr, &version, sizeof(u32));
    new_ptr += sizeof(u32);

    u32 new_object_id = 3;
    memcpy(new_ptr, &new_object_id, sizeof(u32));
    new_ptr += sizeof(u32);

    u16 msg_size = (char *)new_ptr - (char *)packet_pointer;

    memcpy(msg_size_ptr, &msg_size, sizeof(u16));

    write(fd, packet_pointer, msg_size);

    free(packet_pointer);

    return 0;
}
