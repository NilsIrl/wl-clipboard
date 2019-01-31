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

#include "types/source.h"

const char * const *data_to_copy = NULL;
char *temp_file_to_copy = NULL;
int paste_once = 0;

static void cancelled_callback(struct source *source) {
    // we're done!
    if (temp_file_to_copy != NULL) {
        execlp("rm", "rm", "-r", dirname(temp_file_to_copy), NULL);
        perror("exec rm");
        exit(1);
    } else {
        exit(0);
    }
}

static void pasted_callback(struct source *source) {
    if (paste_once) {
        cancelled_callback(source);
    }
}

static struct source source;

void set_data_selection(uint32_t serial) {
    struct wl_data_source *data_source = (struct wl_data_source *) source.proxy;
    wl_data_device_set_selection(data_device, data_source, serial);
    wl_display_roundtrip(display);
    destroy_popup_surface();
}

void try_setting_data_selection_directly() {
    set_data_selection(get_serial());
}


void complain_about_missing_keyboard() {
    bail("Setting primary selection is not supported without a keyboard");
}

#ifdef HAVE_GTK_PRIMARY_SELECTION

void set_gtk_primary_selection(uint32_t serial) {

    struct gtk_primary_selection_source *gtk_primary_selection_source =
        (struct gtk_primary_selection_source *) source.proxy;

    gtk_primary_selection_device_set_selection(
        gtk_primary_selection_device,
        gtk_primary_selection_source,
        serial
    );

    wl_display_roundtrip(display);
    destroy_popup_surface();
}

#endif

#ifdef HAVE_WP_PRIMARY_SELECTION

void set_primary_selection(uint32_t serial) {

    struct zwp_primary_selection_source_v1 *primary_selection_source =
        (struct gtk_primary_selection_source *) source.proxy;

    zwp_primary_selection_device_v1_set_selection(
        primary_selection_device,
        primary_selection_source,
        serial
    );

    wl_display_roundtrip(display);
    destroy_popup_surface();
}

#endif

#ifdef HAVE_WLR_DATA_CONTROL

void create_data_control_source(char *mime_type) {
    struct zwlr_data_control_source_v1 *data_control_source =
        zwlr_data_control_manager_v1_create_data_source(data_control_manager);

    source.proxy = (struct wl_proxy *) data_control_source;
    init_zwlr_data_control_source_v1(&source);
    source_offer(&source, mime_type);
}

#endif

void init_selection(char *mime_type) {
    source.pasted_callback = pasted_callback;
    source.cancelled_callback = cancelled_callback;
    source.data_to_copy = data_to_copy;
    source.temp_file_to_copy = temp_file_to_copy;

    if (data_control_version) {
#ifdef HAVE_WLR_DATA_CONTROL
        create_data_control_source(mime_type);
        struct zwlr_data_control_source_v1 *data_control_source =
            (struct zwlr_data_control_source_v1 *) source.proxy;

        zwlr_data_control_device_v1_set_selection(
            data_control_device,
            data_control_source
        );
#endif
    } else {
        struct wl_data_source *data_source =
            wl_data_device_manager_create_data_source(
                data_device_manager
            );
        source.proxy = (struct wl_proxy *) data_source;
        init_wl_data_source(&source);
        source_offer(&source, mime_type);

        action_on_popup_surface_getting_focus = set_data_selection;
        action_on_no_keyboard = try_setting_data_selection_directly;
        popup_tiny_invisible_surface();
    }
}

void init_primary_selection(char *mime_type) {
    ensure_has_primary_selection();

    source.pasted_callback = pasted_callback;
    source.cancelled_callback = cancelled_callback;
    source.data_to_copy = data_to_copy;
    source.temp_file_to_copy = temp_file_to_copy;

#ifdef HAVE_WLR_DATA_CONTROL
    if (data_control_supports_primary_selection) {
        create_data_control_source(mime_type);
        struct zwlr_data_control_source_v1 *data_control_source =
            (struct zwlr_data_control_source_v1 *) source.proxy;

        zwlr_data_control_device_v1_set_primary_selection(
            data_control_device,
            data_control_source
        );
        return;
    }
#endif

#ifdef HAVE_WP_PRIMARY_SELECTION
    if (primary_selection_device_manager != NULL) {
        struct zwp_primary_selection_source_v1 *primary_selection_source =
            zwp_primary_selection_device_manager_v1_create_source(
                primary_selection_device_manager
            );
        source.proxy = (struct wl_proxy *) primary_selection_source;
        init_zwp_primary_selection_source_v1(&source);
        source_offer(&source, mime_type);

        action_on_popup_surface_getting_focus = set_primary_selection;
        action_on_no_keyboard = complain_about_missing_keyboard;
        popup_tiny_invisible_surface();
        return;
    }
#endif

#ifdef HAVE_GTK_PRIMARY_SELECTION
    if (gtk_primary_selection_device_manager != NULL) {
        struct gtk_primary_selection_source *gtk_primary_selection_source =
            gtk_primary_selection_device_manager_create_source(
                gtk_primary_selection_device_manager
            );
        source.proxy = (struct wl_proxy *) gtk_primary_selection_source;
        init_gtk_primary_selection_source(&source);
        source_offer(&source, mime_type);

        action_on_popup_surface_getting_focus = set_gtk_primary_selection;
        action_on_no_keyboard = complain_about_missing_keyboard;
        popup_tiny_invisible_surface();
        return;
    }
#endif
}

void print_usage(FILE *f, const char *argv0) {
    fprintf(
        f,
        "Usage:\n"
        "\t%s [options] text to copy\n"
        "\t%s [options] < file-to-copy\n\n"
        "Copy content to the Wayland clipboard.\n\n"
        "Options:\n"
        "\t-o, --paste-once\tOnly serve one paste request and then exit.\n"
        "\t-f, --foreground\tStay in the foreground instead of forking.\n"
        "\t-c, --clear\t\tInstead of copying anything, clear the clipboard.\n"
        "\t-p, --primary\t\tUse the \"primary\" clipboard.\n"
        "\t-n, --trim-newline\tDo not copy the trailing newline character.\n"
        "\t-t, --type mime/type\t"
        "Override the inferred MIME type for the content.\n"
        "\t-s, --seat seat-name\t"
        "Pick the seat to work with.\n"
        "\t-v, --version\t\tDisplay version info.\n"
        "\t-h, --help\t\tDisplay this message.\n"
        "Mandatory arguments to long options are mandatory"
        " for short options too.\n\n"
        "See wl-clipboard(1) for more details.\n",
        argv0,
        argv0
    );
}

int main(int argc, char * const argv[]) {

    if (argc < 1) {
        bail("Empty argv");
    }

    int stay_in_foreground = 0;
    int clear = 0;
    char *mime_type = NULL;
    int primary = 0;
    int trim_newline = 0;

    static struct option long_options[] = {
        {"version", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {"primary", no_argument, 0, 'p'},
        {"trim-newline", no_argument, 0, 'n'},
        {"paste-once", no_argument, 0, 'o'},
        {"foreground", no_argument, 0, 'f'},
        {"clear", no_argument, 0, 'c'},
        {"type", required_argument, 0, 't'},
        {"seat", required_argument, 0, 's'},
        {0, 0, 0, 0}
    };
    while (1) {
        int option_index;
        const char *opts = "vhpnofct:s:";
        int c = getopt_long(argc, argv, opts, long_options, &option_index);
        if (c == -1) {
            break;
        }
        if (c == 0) {
            c = long_options[option_index].val;
        }
        switch (c) {
        case 'v':
            print_version_info();
            exit(0);
        case 'h':
            print_usage(stdout, argv[0]);
            exit(0);
        case 'p':
            primary = 1;
            break;
        case 'n':
            trim_newline = 1;
            break;
        case 'o':
            paste_once = 1;
            break;
        case 'f':
            stay_in_foreground = 1;
            break;
        case 'c':
            clear = 1;
            break;
        case 't':
            mime_type = strdup(optarg);
            break;
        case 's':
            requested_seat_name = strdup(optarg);
            break;
        default:
            // getopt has already printed an error message
            print_usage(stderr, argv[0]);
            exit(1);
        }
    }

    init_wayland_globals();

    if (primary) {
        ensure_has_primary_selection();
    }

    if (!clear) {
        if (optind < argc) {
            // copy our command-line args
            data_to_copy = (const char * const *) &argv[optind];
        } else {
            // copy stdin
            temp_file_to_copy = dump_stdin_into_a_temp_file();
            if (trim_newline) {
                trim_trailing_newline(temp_file_to_copy);
            }
            if (mime_type == NULL) {
                mime_type = infer_mime_type_from_contents(temp_file_to_copy);
            }
        }
    }

    if (!stay_in_foreground && !clear) {
        if (fork() != 0) {
            // exit in the parent, but leave the
            // child running in the background
            exit(0);
        }
    }

    if (!primary) {
        init_selection(mime_type);
    } else {
        init_primary_selection(mime_type);
    }

    if (clear) {
        wl_display_roundtrip(display);
        exit(0);
    }

    while (wl_display_dispatch(display) >= 0);

    perror("wl_display_dispatch");
    return 1;
}
