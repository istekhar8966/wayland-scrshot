#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "types.h"

void process_event(u8 **ptr, u8 *end);
void print_buffer(const void *buf, size_t len);
void read_and_process_events(int fd);
void bind_global(int fd, u32 name, char *interface, u32 version, u32 new_id);

#endif
