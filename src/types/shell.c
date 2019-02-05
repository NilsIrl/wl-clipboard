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

#include "types/shell-surface.h"
#include "types/shell.h"

#include "includes/shell-protocols.h"

#include <stdlib.h>

extern struct wl_display *display;

struct shell_surface *shell_create_shell_surface(
    struct shell *self,
    struct wl_surface *surface
) {
    return self->do_create_shell_surface(self, surface);
}


/* Core Wayland implementation */

static struct shell_surface *wl_shell_create_shell_surface(
    struct shell *self,
    struct wl_surface *surface
) {
    struct wl_shell *shell = (struct wl_shell *) self->proxy;
    struct wl_shell_surface *wl_shell_surface =
            wl_shell_get_shell_surface(shell, surface);

    struct shell_surface *shell_surface = malloc(sizeof(struct shell_surface));
    shell_surface->proxy = (struct wl_proxy *) wl_shell_surface;
    init_wl_shell_surface(shell_surface);
    return shell_surface;
}

void init_wl_shell(struct shell *self) {
    self->do_create_shell_surface = wl_shell_create_shell_surface;
}


/* xdg-shell implementation */

#ifdef HAVE_XDG_SHELL

static struct shell_surface *xdg_shell_create_shell_surfacce(
    struct shell *self,
    struct wl_surface *surface
) {
    struct xdg_wm_base *wm_base = (struct xdg_wm_base *) self->proxy;

    struct shell_surface *shell_surface = malloc(sizeof(struct shell_surface));

    struct xdg_surface *xdg_surface =
        xdg_wm_base_get_xdg_surface(wm_base, surface);
    shell_surface->proxy = (struct wl_proxy *) xdg_surface;
    init_xdg_surface(shell_surface);

    // Signal that the surface is ready to be configured.
    wl_surface_commit(surface);
    // Wait for the surface to be configured.
    wl_display_roundtrip(display);

    return shell_surface;
}

static void xdg_wm_base_ping_handler
(
    void *data,
    struct xdg_wm_base *wm_base,
    uint32_t serial
) {
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener xdg_wm_base_listener = {
    .ping = xdg_wm_base_ping_handler
};

void init_xdg_shell(struct shell *self) {
    struct xdg_wm_base *wm_base = (struct xdg_wm_base *) self->proxy;
    xdg_wm_base_add_listener(wm_base, &xdg_wm_base_listener, self);
    self->do_create_shell_surface = xdg_shell_create_shell_surfacce;
}

#endif /* HAVE_XDG_SHELL */


/* wlr-layer-shell implementation */

#ifdef HAVE_WLR_LAYER_SHELL

static struct shell_surface *zwlr_layer_shell_v1_create_shell_surfacce(
    struct shell *self,
    struct wl_surface *surface
) {
    struct zwlr_layer_shell_v1 *layer_shell =
        (struct zwlr_layer_shell_v1 *) self->proxy;

    struct shell_surface *shell_surface = malloc(sizeof(struct shell_surface));

    struct zwlr_layer_surface_v1 *layer_surface =
        zwlr_layer_shell_v1_get_layer_surface(
            layer_shell,
            surface,
            NULL,  // output
            ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
            "wl-clipboard"  // namespace
        );
    shell_surface->proxy = (struct wl_proxy *) layer_surface;
    init_zwlr_layer_surface_v1(shell_surface);

    // Signal that the surface is ready to be configured.
    wl_surface_commit(surface);
    // Wait for the surface to be configured.
    wl_display_roundtrip(display);

    return shell_surface;
}

void init_zwlr_layer_shell_v1(struct shell *self) {
    self->do_create_shell_surface = zwlr_layer_shell_v1_create_shell_surfacce;
}

#endif /* HAVE_WLR_LAYER_SHELL */
