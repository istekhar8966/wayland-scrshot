#ifndef WAYLAND_H
#define WAYLAND_H

#include "types.h"

int connect_to_wayland();
int write_wayland_message(
    int fd,
    u32 object_id,
    u16 opcode,
    const void *args,
    u32 args_size);

#endif // WAYLAND_H
