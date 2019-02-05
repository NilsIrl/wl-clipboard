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

#include "includes/shell-protocols.h"
#include "types/shell-manager.h"
#include "types/shell.h"
#include "util/misc.h"

#include <stdlib.h>
#include <string.h>  // memset


void init_shell_manager(struct shell_manager *self) {
    memset(self, 0, sizeof(struct shell_manager));
}

struct shell *shell_manager_find_shell(struct shell_manager *self) {
    struct shell *shell = malloc(sizeof(struct shell));

#ifdef HAVE_WLR_LAYER_SHELL
    if (self->zwlr_layer_shell_v1 != NULL) {
        shell->proxy = (struct wl_proxy *) self->zwlr_layer_shell_v1;
        init_zwlr_layer_shell_v1(shell);
        return shell;
    }
#endif

    if (self->wl_shell != NULL) {
        shell->proxy = (struct wl_proxy *) self->wl_shell;
        init_wl_shell(shell);
        return shell;
    }

#ifdef HAVE_XDG_SHELL
    if (self->xdg_wm_base != NULL) {
        shell->proxy = (struct wl_proxy *) self->xdg_wm_base;
        init_xdg_shell(shell);
        return shell;
    }
#endif

    bail("Missing a shell implementation");
}

void shell_manager_add_wl_shell(
    struct shell_manager *self,
    struct wl_shell *wl_shell
) {
    self->wl_shell = wl_shell;
}

#ifdef HAVE_XDG_SHELL

void shell_manager_add_xdg_shell(
    struct shell_manager *self,
    struct xdg_wm_base *xdg_wm_base
) {
    self->xdg_wm_base = xdg_wm_base;
}

#endif

#ifdef HAVE_WLR_LAYER_SHELL

void shell_manager_add_zwlr_layer_shell_v1(
    struct shell_manager *self,
    struct zwlr_layer_shell_v1 *zwlr_layer_shell_v1
) {
    self->zwlr_layer_shell_v1 = zwlr_layer_shell_v1;
}

#endif

