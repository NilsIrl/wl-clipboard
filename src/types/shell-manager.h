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

#ifndef TYPES_SHELL_MANAGER_H
#define TYPES_SHELL_MANAGER_H

#include "includes/shell-protocols.h"
#include "types/shell.h"

struct shell_manager {
    /* These fields are initialized by the implementation */

    struct wl_shell *wl_shell;

#ifdef HAVE_XDG_SHELL
    struct xdg_wm_base *xdg_wm_base;
#endif

#ifdef HAVE_WLR_LAYER_SHELL
    struct zwlr_layer_shell_v1 *zwlr_layer_shell_v1;
#endif
};

void init_shell_manager(struct shell_manager *self);

int shell_manager_has_shell(struct shell_manager *self);

struct shell *shell_manager_find_shell(struct shell_manager *self);

void shell_manager_add_wl_shell(
    struct shell_manager *self,
    struct wl_shell *wl_shell
);

#ifdef HAVE_XDG_SHELL
void shell_manager_add_xdg_shell(
    struct shell_manager *self,
    struct xdg_wm_base *xdg_wm_base
);
#endif

#ifdef HAVE_WLR_LAYER_SHELL
void shell_manager_add_zwlr_layer_shell_v1(
    struct shell_manager *self,
    struct zwlr_layer_shell_v1 *zwlr_layer_shell_v1
);
#endif

#endif /* TYPES_SHELL_MANAGER_H */
