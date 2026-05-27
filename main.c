#define _GNU_SOURCE

#include "protocol.h"
#include "types.h"
#include "wayland.h"
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#define ROUND_UP_4(x) (((x) + 3) & ~3)
#define WL_DISPLAY_ID 1
#define WL_REGISTRY_ID 2
#define WL_COMPOSITOR_ID 3
#define WL_SURFACE_ID 4
#define WL_SHM_ID 5

int main() {
    int fd = connect_to_wayland();

    u32 args[1] = {WL_REGISTRY_ID};
    write_wayland_message(fd, 1, 1, &args, sizeof(args));

    read_and_process_events(fd);

    char *interface = "wl_compositor";
    bind_global(fd, 5, interface, 6, WL_COMPOSITOR_ID);

    read_and_process_events(fd);

    close(fd);
}
