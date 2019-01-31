/* wl-clipboard
 *
 * Copyright © 2019 Sergey Bugaev <bugaevc@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "boilerplate.h"

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, "wl_data_device_manager") == 0) {
        data_device_manager = wl_registry_bind(
            registry,
            name,
            &wl_data_device_manager_interface,
            1
        );
    } else if (strcmp(interface, "wl_seat") == 0) {
        struct wl_seat *new_seat = wl_registry_bind(
            registry,
            name,
            &wl_seat_interface,
            2
        );
        process_new_seat(new_seat);
    } else if (strcmp(interface, "wl_compositor") == 0) {
        compositor = wl_registry_bind(
            registry,
            name,
            &wl_compositor_interface,
            3
        );
    } else if (strcmp(interface, "wl_shm") == 0) {
        shm = wl_registry_bind(
            registry,
            name,
            &wl_shm_interface,
            1
        );
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell = wl_registry_bind(
            registry,
            name,
            &wl_shell_interface,
            1
        );
    }
#ifdef HAVE_XDG_SHELL
    else if (strcmp(interface, "xdg_wm_base") == 0) {
        xdg_wm_base = wl_registry_bind(
            registry,
            name,
            &xdg_wm_base_interface,
            1
        );
    }
#endif
#ifdef HAVE_WLR_LAYER_SHELL
    else if (strcmp(interface, "zwlr_layer_shell_v1") == 0) {
        layer_shell = wl_registry_bind(
            registry,
            name,
            &zwlr_layer_shell_v1_interface,
            1
        );
    }
#endif
#ifdef HAVE_GTK_PRIMARY_SELECTION
    else if (strcmp(interface, "gtk_primary_selection_device_manager") == 0) {
        gtk_primary_selection_device_manager = wl_registry_bind(
            registry,
            name,
            &gtk_primary_selection_device_manager_interface,
            1
        );
    }
#endif
#ifdef HAVE_WP_PRIMARY_SELECTION
    else if (strcmp(interface, "zwp_primary_selection_device_manager_v1") == 0) {
        primary_selection_device_manager = wl_registry_bind(
            registry,
            name,
            &zwp_primary_selection_device_manager_v1_interface,
            1
        );
    }
#endif
#ifdef HAVE_WLR_DATA_CONTROL
    else if (strcmp(interface, "zwlr_data_control_manager_v1") == 0) {
        if (version > 2) {
            // the maximum verison wl-clipboard knows about
            version = 2;
        }
        data_control_manager = wl_registry_bind(
            registry,
            name,
            &zwlr_data_control_manager_v1_interface,
            version
        );
        data_control_version = version;
    }
#endif
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}

const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};

void keyboard_keymap_handler
(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t format,
    int fd,
    uint32_t size
) {
    close(fd);
}

void keyboard_enter_handler
(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    struct wl_surface *surface,
    struct wl_array *keys
) {
    struct wl_seat *this_seat = (struct wl_seat *) data;
    // when we get to here, global seat is already initialized
    if (this_seat != seat) {
        return;
    }
    if (action_on_popup_surface_getting_focus != NULL) {
        action_on_popup_surface_getting_focus(serial);
    }
}

void keyboard_leave_handler
(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    struct wl_surface *surface
) {}

void keyboard_key_handler
(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    uint32_t time,
    uint32_t key,
    uint32_t state
) {}

void keyboard_modifiers_handler
(
    void *data,
    struct wl_keyboard *keyboard,
    uint32_t serial,
    uint32_t mods_depressed,
    uint32_t mods_latched,
    uint32_t mods_locked,
    uint32_t group
) {}

const struct wl_keyboard_listener keayboard_listener = {
    .keymap = keyboard_keymap_handler,
    .enter = keyboard_enter_handler,
    .leave = keyboard_leave_handler,
    .key = keyboard_key_handler,
    .modifiers = keyboard_modifiers_handler,
};

void seat_capabilities_handler
(
    void *data,
    struct wl_seat *this_seat,
    uint32_t capabilities
) {
    // stash the capabilities of this seat for later
    void *user_data = (void *) (unsigned long) capabilities;
    wl_seat_set_user_data(this_seat, user_data);

    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        struct wl_keyboard *keyboard = wl_seat_get_keyboard(this_seat);
        wl_keyboard_add_listener(keyboard, &keayboard_listener, this_seat);
    }
}

void seat_name_handler
(
    void *data,
    struct wl_seat *this_seat,
    const char *name
) {
    if (requested_seat_name == NULL) {
        return;
    }
    if (strcmp(name, requested_seat_name) == 0) {
        seat = this_seat;
    }
}

const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities_handler,
    .name = seat_name_handler
};

#define UNSET_CAPABILITIES ((void *) (uint32_t) 35)

void process_new_seat(struct wl_seat *new_seat) {
    if (seat != NULL) {
        wl_seat_destroy(new_seat);
        return;
    }
    if (requested_seat_name == NULL) {
        seat = new_seat;
    }
    wl_seat_add_listener(new_seat, &seat_listener, UNSET_CAPABILITIES);
}

int ensure_seat_has_keyboard() {
    void *user_data = wl_seat_get_user_data(seat);
    while (user_data == UNSET_CAPABILITIES) {
        wl_display_roundtrip(display);
        user_data = wl_seat_get_user_data(seat);
    }

    uint32_t capabilities = (uint32_t) (unsigned long) user_data;
    if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
        return 1;
    }

    if (action_on_no_keyboard != NULL) {
        action_on_no_keyboard();
    }
    return 0;
}

#undef UNSET_CAPABILITIES

void shell_surface_ping
(
    void *data,
    struct wl_shell_surface *shell_surface,
    uint32_t serial
) {
    wl_shell_surface_pong(shell_surface, serial);
}

void shell_surface_configure
(
    void *data,
    struct wl_shell_surface *shell_surface,
    uint32_t edges,
    int32_t width,
    int32_t height
) {}

void shell_surface_popup_done
(
    void *data,
    struct wl_shell_surface *shell_surface
) {}

const struct wl_shell_surface_listener shell_surface_listener = {
    .ping = shell_surface_ping,
    .configure = shell_surface_configure,
    .popup_done = shell_surface_popup_done
};

#ifdef HAVE_XDG_SHELL

void xdg_toplevel_configure_handler
(
    void *data,
    struct xdg_toplevel *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states
) {}

void xdg_toplevel_close_handler
(
    void *data,
    struct xdg_toplevel *xdg_toplevel
) {}

const struct xdg_toplevel_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler
};

void xdg_surface_configure_handler
(
    void *data,
    struct xdg_surface *xdg_surface,
    uint32_t serial
) {
    xdg_surface_ack_configure(xdg_surface, serial);
}

const struct xdg_surface_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler
};

void xdg_wm_base_ping_handler
(
    void *data,
    struct xdg_wm_base *xdg_wm_base,
    uint32_t serial
) {
    xdg_wm_base_pong(xdg_wm_base, serial);
}

const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler
};

#endif

#ifdef HAVE_WLR_LAYER_SHELL

void layer_surface_configure_handler
(
    void *data,
    struct zwlr_layer_surface_v1 *layer_surface,
    uint32_t serial,
    uint32_t width,
    uint32_t height
) {
    zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
}

void layer_surface_closed_handler
(
    void *data,
    struct zwlr_layer_surface_v1 *layer_surface
) {}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
    .configure = layer_surface_configure_handler,
    .closed = layer_surface_closed_handler
};

#endif

#ifdef HAVE_WLR_DATA_CONTROL

void data_control_manager_primary_selection
(
    void *data,
    struct zwlr_data_control_manager_v1 *data_control_manager
) {
    data_control_supports_primary_selection = 1;
}

const struct zwlr_data_control_manager_v1_listener
data_control_manager_listener = {
    .primary_selection = data_control_manager_primary_selection
};

#endif

void init_wayland_globals() {
    display = wl_display_connect(NULL);
    if (display == NULL) {
        bail("Failed to connect to a Wayland server");
    }

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    if (
        data_device_manager == NULL ||
        compositor == NULL ||
        shm == NULL ||
        (shell == NULL
#ifdef HAVE_XDG_SHELL
         && xdg_wm_base == NULL
#endif
#ifdef HAVE_WLR_LAYER_SHELL
         && layer_shell == NULL
#endif
        )
    ) {
        bail("Missing a required global object");
    }

    if (seat == NULL && requested_seat_name != NULL) {
        wl_display_roundtrip(display);
    }
    if (seat == NULL) {
        if (requested_seat_name == NULL) {
            bail("No seat available");
        }
        bail("Cannot find the requested seat");
    }

    data_device = wl_data_device_manager_get_data_device(
        data_device_manager,
        seat
    );
#ifdef HAVE_GTK_PRIMARY_SELECTION
    if (gtk_primary_selection_device_manager != NULL) {
        gtk_primary_selection_device =
            gtk_primary_selection_device_manager_get_device(
                gtk_primary_selection_device_manager,
                seat
            );
    }
#endif
#ifdef HAVE_WP_PRIMARY_SELECTION
    if (primary_selection_device_manager != NULL) {
        primary_selection_device =
            zwp_primary_selection_device_manager_v1_get_device(
                primary_selection_device_manager,
                seat
            );
    }
#endif
#ifdef HAVE_WLR_DATA_CONTROL
    if (data_control_manager != NULL) {
        zwlr_data_control_manager_v1_add_listener(
            data_control_manager,
            &data_control_manager_listener,
            NULL
        );
        data_control_device =
            zwlr_data_control_manager_v1_get_data_device(
                data_control_manager,
                seat
            );
    }
#endif
}

void ensure_has_primary_selection() {
#ifdef HAVE_WLR_DATA_CONTROL
    if (data_control_version >=
        ZWLR_DATA_CONTROL_MANAGER_V1_PRIMARY_SELECTION_SINCE_VERSION) {

        if (!data_control_supports_primary_selection) {
            wl_display_roundtrip(display);
        }
        if (data_control_supports_primary_selection) {
            return;
        }
    }
#endif
#ifdef HAVE_GTK_PRIMARY_SELECTION
    if (gtk_primary_selection_device_manager != NULL) {
        return;
    }
#endif
#ifdef HAVE_WP_PRIMARY_SELECTION
    if (primary_selection_device_manager != NULL) {
        return;
    }
#endif

#if defined(HAVE_GTK_PRIMARY_SELECTION) || defined(HAVE_WP_PRIMARY_SELECTION)
    bail("Primary selection is not supported on this compositor");
#else
    bail("wl-clipboard was built without primary selection support");
#endif
}

int create_anonymous_file() {
#ifdef HAVE_MEMFD
    return syscall(SYS_memfd_create, "buffer", 0);
#endif
#ifdef HAVE_SHM_ANON
    return shm_open(SHM_ANON, O_RDWR | O_CREAT, 0600);
#endif
    return fileno(tmpfile());
}

void popup_tiny_invisible_surface() {
    // HACK:
    // pop up a tiny invisible surface to get the keyboard focus,
    // otherwise we won't be notified of the selection

    if (!ensure_seat_has_keyboard()) {
        return;
    }

    // make sure that we get the keyboard
    // object before creating the surface,
    // so that we get the enter event
    wl_display_dispatch(display);

    surface = wl_compositor_create_surface(compositor);

#ifdef HAVE_WLR_LAYER_SHELL
    if (layer_shell != NULL) {
        // use wlr-layer-shell
        layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            layer_shell,
            surface,
            NULL,  // output
            ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
            "wl-clipboard"  // namespace
        );
        zwlr_layer_surface_v1_add_listener(
            layer_surface,
            &layer_surface_listener,
            NULL
        );
        zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface, 1);
        // signal that the surface is ready to be configured
        wl_surface_commit(surface);
        // wait for the surface to be configured
        wl_display_roundtrip(display);
    } else
#endif
    if (shell != NULL) {
        // use wl_shell
        shell_surface = wl_shell_get_shell_surface(shell, surface);
        wl_shell_surface_set_toplevel(shell_surface);
        wl_shell_surface_set_title(shell_surface, "wl-clipboard");
    } else {
#ifdef HAVE_XDG_SHELL
        // use xdg-shell
        xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
        xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
        xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
        xdg_toplevel = xdg_surface_get_toplevel(xdg_surface);
        xdg_toplevel_add_listener(xdg_toplevel, &xdg_toplevel_listener, NULL);
        xdg_toplevel_set_title(xdg_toplevel, "wl-clipboard");
        // signal that the surface is ready to be configured
        wl_surface_commit(surface);
        // wait for the surface to be configured
        wl_display_roundtrip(display);
#else
        bail("Unreachable: HAVE_XDG_SHELL undefined and no wl_shell");
#endif
    }

    if (surface == NULL) {
        // it's possible that we've been given focus without us
        // ever commiting a buffer, in which case the handlers
        // may have already destroyed the surface; there's no
        // way or need for us to commit a buffer in that case
        return;
    }

    int width = 1;
    int height = 1;
    int stride = width * 4;
    int size = stride * height;  // bytes

    // open an anonymous file and write some zero bytes to it
    int fd = create_anonymous_file();
    ftruncate(fd, size);

    // turn it into a shared memory pool
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);

    // allocate the buffer in that pool
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
        0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
    // zeros in ARGB8888 mean fully transparent

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, width, height);
    wl_surface_commit(surface);
}

void destroy_popup_surface() {
#ifdef HAVE_WLR_LAYER_SHELL
    if (layer_surface != NULL) {
        zwlr_layer_surface_v1_destroy(layer_surface);
        layer_surface = NULL;
    }
#endif
    if (shell_surface != NULL) {
        wl_shell_surface_destroy(shell_surface);
        shell_surface = NULL;
    }
#ifdef HAVE_XDG_SHELL
    if (xdg_toplevel != NULL) {
        xdg_toplevel_destroy(xdg_toplevel);
        xdg_toplevel = NULL;
    }
    if (xdg_surface != NULL) {
        xdg_surface_destroy(xdg_surface);
        xdg_surface = NULL;
    }
#endif
    if (surface != NULL) {
        wl_surface_destroy(surface);
        surface = NULL;
    }
}

static uint32_t global_serial;

void callback_done
(
    void *data,
    struct wl_callback *callback,
    uint32_t serial
) {
    global_serial = serial;
}

const struct wl_callback_listener callback_listener = {
    .done = callback_done
};

uint32_t get_serial() {
    struct wl_callback *callback = wl_display_sync(display);
    wl_callback_add_listener(callback, &callback_listener, NULL);

    while (global_serial == 0) {
        wl_display_dispatch(display);
    }

    return global_serial;
}
