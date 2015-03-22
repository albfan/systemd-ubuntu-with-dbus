/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2010 Lennart Poettering

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <mntent.h>

#include "log.h"
#include "util.h"
#include "path-util.h"
#include "set.h"
#include "mount-setup.h"
#include "exit-status.h"

static bool is_mounted(const char *dev_path) {
        _cleanup_free_ char *parent_path = NULL;
        struct stat st, pst;
        int r;

        parent_path = strjoin(dev_path, "/..", NULL);

        r = stat(dev_path, &st);
        if (r < 0)
                return false;

        r = stat(parent_path, &pst);
        if (r < 0)
                return false;

        /*
         * This code to check if a given path is a mountpoint is
         * borrowed from util-linux' mountpoint tool.
         */
        if ((st.st_dev != pst.st_dev) ||
            (st.st_dev == pst.st_dev && st.st_ino == pst.st_ino))
                return true;

        return false;
}

/* Goes through /etc/fstab and remounts all API file systems, applying
 * options that are in /etc/fstab that systemd might not have
 * respected */

int main(int argc, char *argv[]) {
        int ret = EXIT_FAILURE;
        _cleanup_endmntent_ FILE *f = NULL;
        struct mntent* me;
        Hashmap *pids = NULL;

        if (argc > 1) {
                log_error("This program takes no argument.");
                return EXIT_FAILURE;
        }

        log_set_target(LOG_TARGET_AUTO);
        log_parse_environment();
        log_open();

        umask(0022);

        f = setmntent("/etc/fstab", "r");
        if (!f) {
                if (errno == ENOENT)
                        return EXIT_SUCCESS;

                log_error_errno(errno, "Failed to open /etc/fstab: %m");
                return EXIT_FAILURE;
        }

        pids = hashmap_new(NULL);
        if (!pids) {
                log_error("Failed to allocate set");
                goto finish;
        }

        ret = EXIT_SUCCESS;

        while ((me = getmntent(f))) {
                pid_t pid;
                int k;
                char *s;

                /* Remount the root fs, /usr and all API VFS */
                if (!mount_point_is_api(me->mnt_dir) &&
                    !path_equal(me->mnt_dir, "/") &&
                    !path_equal(me->mnt_dir, "/usr"))
                        continue;

                /* Skip /usr if it hasn't been mounted by the initrd */
                if (path_equal(me->mnt_dir, "/usr") &&
                    !is_mounted("/usr"))
                        continue;

                log_debug("Remounting %s", me->mnt_dir);

                pid = fork();
                if (pid < 0) {
                        log_error_errno(errno, "Failed to fork: %m");
                        ret = EXIT_FAILURE;
                        continue;
                }

                if (pid == 0) {
                        const char *arguments[5];
                        /* Child */

                        arguments[0] = "/bin/mount";
                        arguments[1] = me->mnt_dir;
                        arguments[2] = "-o";
                        arguments[3] = "remount";
                        arguments[4] = NULL;

                        execv("/bin/mount", (char **) arguments);

                        log_error_errno(errno, "Failed to execute /bin/mount: %m");
                        _exit(EXIT_FAILURE);
                }

                /* Parent */

                s = strdup(me->mnt_dir);
                if (!s) {
                        log_oom();
                        ret = EXIT_FAILURE;
                        continue;
                }


                k = hashmap_put(pids, UINT_TO_PTR(pid), s);
                if (k < 0) {
                        log_error_errno(k, "Failed to add PID to set: %m");
                        ret = EXIT_FAILURE;
                        continue;
                }
        }

        while (!hashmap_isempty(pids)) {
                siginfo_t si = {};
                char *s;

                if (waitid(P_ALL, 0, &si, WEXITED) < 0) {

                        if (errno == EINTR)
                                continue;

                        log_error_errno(errno, "waitid() failed: %m");
                        ret = EXIT_FAILURE;
                        break;
                }

                s = hashmap_remove(pids, UINT_TO_PTR(si.si_pid));
                if (s) {
                        if (!is_clean_exit(si.si_code, si.si_status, NULL)) {
                                if (si.si_code == CLD_EXITED)
                                        log_error("/bin/mount for %s exited with exit status %i.", s, si.si_status);
                                else
                                        log_error("/bin/mount for %s terminated by signal %s.", s, signal_to_string(si.si_status));

                                ret = EXIT_FAILURE;
                        }

                        free(s);
                }
        }

finish:

        if (pids)
                hashmap_free_free(pids);

        return ret;
}
