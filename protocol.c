#include "types.h"
#include "wayland.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ROUND_UP_4(x) (((x) + 3) & ~3)

void process_event(u8 **ptr, u8 *end) {
    // header
    u32 object_id = *(u32 *)(*ptr);
    *ptr += sizeof(u32);
    u32 size_opcode = *(u32 *)(*ptr);
    *ptr += sizeof(u32);

    u16 size = size_opcode >> 16;
    u16 opcode = size_opcode & 0xFFFF;

    printf("[Event] object=%u, opcode=%u, size=%u\n", object_id, opcode, size);

    if (object_id == 2 && opcode == 0) {
        u32 name = *(u32 *)(*ptr);
        *ptr += sizeof(u32);

        u32 len = *(u32 *)(*ptr);
        *ptr += sizeof(u32);

        printf("    -> Global name=%u", name);

        printf(", interface='");
        for (u32 i = 0; i < len && ((*ptr) + i) < end; i++) {
            if ((*ptr)[i] > 33 && (*ptr)[i] < 127) {
                printf("%c", (*ptr)[i]);
            }
        }
        printf("'");

        *ptr += ROUND_UP_4(len);
        u32 version = *(u32 *)(*ptr);
        *ptr += sizeof(u32);
        printf(", version=%u\n", version);
    }
    printf("\n");
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
    size_t n = read(fd, buffer, sizeof(buffer));

    if (n <= 0)
        return;

    u8 *ptr = buffer;
    u8 *end = ptr + n;

    while (ptr + 8 <= end) {
        process_event(&ptr, end);
    }
}

void bind_global(
    int fd,
    u32 name,
    char *interface,
    u32 version,
    u32 new_id

) {
    u32 interface_len = strlen(interface) + 1;
    u32 padding = ROUND_UP_4(interface_len);

    u32 total_size = 12 + padding;
    u8 *args = calloc(total_size, 1);
    u8 *ptr = args;

    memcpy(ptr, &name, sizeof(u32));
    ptr += sizeof(u32);

    memcpy(ptr, &interface_len, sizeof(u32));
    ptr += sizeof(u32);

    memcpy(ptr, interface, interface_len);
    ptr += padding;

    memcpy(ptr, &version, sizeof(u32));
    ptr += sizeof(u32);

    memcpy(ptr, &new_id, sizeof(u32));
    ptr += sizeof(u32);

    int n = write_wayland_message(fd, 2, 0, args, total_size);
    if (n != -1) {
        printf("Global bind request sent for name=%u, interface=%s, version=%u.\n", name, interface, version);
    }
}
