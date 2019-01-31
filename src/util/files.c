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

#include "util/files.h"
#include "util/string.h"
#include "util/misc.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h> // open
#include <sys/stat.h> // open
#include <sys/types.h> // open
#include <stdlib.h> // exit
#include <libgen.h> // basename
#include <sys/wait.h>
#include <limits.h> // PATH_MAX


void trim_trailing_newline(const char *file_path) {
    int fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("open file for trimming");
        return;
    }

    int seek_res = lseek(fd, -1, SEEK_END);
    if (seek_res < 0 && errno == EINVAL) {
        // empty file
        goto out;
    } else if (seek_res < 0) {
        perror("lseek");
        goto out;
    }
    // otherwise, seek_res is the new file size

    char last_char;
    int read_res = read(fd, &last_char, 1);
    if (read_res != 1) {
        perror("read");
        goto out;
    }
    if (last_char != '\n') {
        goto out;
    }

    ftruncate(fd, seek_res);
out:
    close(fd);
}

char *path_for_fd(int fd) {
    char fdpath[64];
    snprintf(fdpath, sizeof(fdpath), "/dev/fd/%d", fd);
    return realpath(fdpath, NULL);
}

char *infer_mime_type_from_contents(const char *file_path) {
    int pipefd[2];
    pipe(pipefd);
    if (fork() == 0) {
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_RDONLY);
        dup2(devnull, STDIN_FILENO);
        close(devnull);
        execlp("xdg-mime", "xdg-mime", "query", "filetype", file_path, NULL);
        exit(1);
    }

    close(pipefd[1]);
    int wstatus;
    wait(&wstatus);
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
        char *res = malloc(PATH_MAX + 1);
        size_t len = read(pipefd[0], res, PATH_MAX + 1);
        len--; // trim the newline
        close(pipefd[0]);
        res[len] = 0;

        if (str_has_prefix(res, "inode/")) {
            free(res);
            return NULL;
        }

        return res;
    }
    close(pipefd[0]);
    return NULL;
}

char *infer_mime_type_from_name(const char *file_path) {
    const char *actual_ext = get_file_extension(file_path);
    if (actual_ext == NULL) {
        return NULL;
    }

    FILE *f = fopen("/etc/mime.types", "r");
    if (f == NULL) {
        f = fopen("/usr/local/etc/mime.types", "r");
    }
    if (f == NULL) {
        return NULL;
    }

    for (char line[200]; fgets(line, sizeof(line), f) != NULL;) {
        // skip comments and blank lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }

        // each line consists of a mime type and a list of extensions
        char mime_type[200];
        int consumed;
        if (sscanf(line, "%s%n", mime_type, &consumed) != 1) {
            // malformed line?
            continue;
        }
        char *lineptr = line + consumed;
        for (char ext[200]; sscanf(lineptr, "%s%n", ext, &consumed) == 1;) {
            if (strcmp(ext, actual_ext) == 0) {
                fclose(f);
                return strdup(mime_type);
            }
            lineptr += consumed;
        }
    }
    fclose(f);
    return NULL;
}

char *dump_stdin_into_a_temp_file() {
    char dirpath[] = "/tmp/wl-copy-buffer-XXXXXX";
    if (mkdtemp(dirpath) != dirpath) {
        perror("mkdtemp");
        exit(1);
    }
    char *original_path = path_for_fd(STDIN_FILENO);

    char *res_path = malloc(PATH_MAX + 1);
    memcpy(res_path, dirpath, sizeof(dirpath));
    strcat(res_path, "/");

    if (original_path != NULL) {
        char *name = basename(original_path);
        strcat(res_path, name);
    } else {
        strcat(res_path, "stdin");
    }

    if (fork() == 0) {
        FILE *res = fopen(res_path, "w");
        if (res == NULL) {
            perror("fopen");
            exit(1);
        }
        dup2(fileno(res), STDOUT_FILENO);
        fclose(res);
        execlp("cat", "cat", NULL);
        perror("exec cat");
        exit(1);
    }

    int wstatus;
    wait(&wstatus);
    if (original_path != NULL) {
        free(original_path);
    }
    if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0) {
        return res_path;
    }
    bail("Failed to copy the file");
}
