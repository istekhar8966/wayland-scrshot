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

int round_4(int x);
int create_shm_buf();
int socket_connect();
void get_registry(int fd, int new_id);
void bind_registry(int fd, u32 global_name, char *interface, u32 version, u32 new_id);
void send_packet(int fd, u32 object_id, u16 opcode, u32 new_id);

int main(void) {
    int object_id = 1;

    int fd = socket_connect();

    int registry_object_id = object_id + 1;

    // get_registry(fd, registry_object_id);

    int wl_comp_obj_id = object_id + 2;
    bind_registry(fd, 3, "wl_compositor", 6, wl_comp_obj_id);

    u32 create_surface_obj_id = object_id + 3;
    send_packet(fd, wl_comp_obj_id, 0, create_surface_obj_id);

    return 0;
}

int round_4(int x) {
    return (x + 3) & ~3;
}

int create_shm_buf() {
    int mem_fd = memfd_create("memfd_buffer", 0);
    int fd_buf_size = 1920 * 1080 * 4;

    ftruncate(mem_fd, fd_buf_size);

    void *pixels = mmap(NULL, fd_buf_size, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, 0);

    if (pixels == MAP_FAILED) {
        perror("mmap Failed!");
        exit(1);
    }

    return mem_fd;
}

int socket_connect() {

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
    } else {
        printf("connected to WAYLAND_SOCKET!\n");
    }

    return fd;
}

void get_registry(int fd, int new_id) {

    /*
        object_id = 1
        size/opcode = (12 << 16) | 1
        new_id = 2
    */

    u32 packet[3] = {0};

    packet[0] = 1;
    packet[1] = (12 << 16) | 1;
    packet[2] = new_id;

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

        buf_ptr += round_4(str_size);

        printf("v%u", *(u32 *)buf_ptr);
        buf_ptr += sizeof(u32);

        printf("\n");
    }

    free(buffer);
}

void bind_registry(int fd, u32 global_name, char *interface, u32 version, u32 new_id) {
    void *buf_ptr = calloc(1, 4096);
    if (buf_ptr == NULL) {
        perror("calloc: memory allocation failed");
    }
    void *mv_ptr = buf_ptr;

    u32 object_id = 2;

    // first 4 bytes OBJECT_ID
    memcpy(mv_ptr, &object_id, sizeof(u32));
    mv_ptr += sizeof(u32);

    void *msg_size_ptr = mv_ptr;
    mv_ptr += sizeof(u32);

    memcpy(mv_ptr, &global_name, sizeof(u32));
    mv_ptr += sizeof(u32);

    int str_size = strlen(interface) + 1;

    memcpy(mv_ptr, &str_size, sizeof(u32));
    mv_ptr += sizeof(u32);

    memcpy(mv_ptr, interface, str_size);
    mv_ptr += str_size;

    u32 padding = round_4(str_size) - str_size;
    mv_ptr += padding;

    memcpy(mv_ptr, &version, sizeof(u32));
    mv_ptr += sizeof(u32);

    memcpy(mv_ptr, &new_id, sizeof(u32));
    mv_ptr += sizeof(u32);

    u16 msg_size = ((char *)mv_ptr - (char *)buf_ptr);
    u32 opcode_msg_size = (msg_size << 16) | 0;
    memcpy(msg_size_ptr, &opcode_msg_size, sizeof(u32));

    if (write(fd, buf_ptr, msg_size) == -1) {
        perror("write");
    } else {
        printf("bind registry succes!\n");
    }

    free(buf_ptr);
}

void send_packet(int fd, u32 object_id, u16 opcode, u32 new_id) {
    void *packet_ptr = calloc(4, 3);
    void *mv_ptr = packet_ptr;

    memcpy(mv_ptr, &object_id, sizeof(u32));
    mv_ptr += sizeof(u32);

    u32 opcode_msg_size;
    void *opcode_msg_size_ptr = mv_ptr;
    mv_ptr += sizeof(u32);

    u32 create_surface_obj_id = new_id;
    memcpy(mv_ptr, &create_surface_obj_id, sizeof(u32));
    mv_ptr += sizeof(u32);

    u16 msg_size = (char *)mv_ptr - (char *)packet_ptr;

    *(u32 *)opcode_msg_size_ptr = (msg_size << 16) | 0;

    if (write(fd, packet_ptr, msg_size) == -1) {
        perror("write");
    } else {
        printf("request sent for object_id: %d,opcode: %d\n", object_id, opcode);
    }
}
