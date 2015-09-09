/***
  suid helper to run /etc/network/if-*.d/ hooks from networkd

  (C) 2015 Canonical Ltd.
  Author: Martin Pitt <martin.pitt@ubuntu.com>

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
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
        const char* action, *interface;
        const char *c;
        char path[100];

        if (argc != 3 || (strcmp(argv[1], "up") != 0 && strcmp(argv[1], "post-down") != 0)) {
                fprintf(stderr, "Usage: %s up|post-down <interface>\n", argv[0]);
                return 1;
        }
        action = argv[1];
        interface = argv[2];

        /* disallow control chars, spaces, or quotes, as this is passed to shell scripts */
        for (c = interface; *c != '\0'; ++c)
                if (isspace(*c) || iscntrl(*c) || *c == '\'' || *c == '"') {
                        fprintf(stderr, "Invalid character '%c' in interface name\n", *c);
                        return 1;
                }

        //fprintf(stderr, "action: %s iface %s", action, interface);

        /* fully elevate to root privs, so that we will keep them through exec() */
        if (setreuid(0, 0) < 0) {
                fprintf(stderr, "Could not change real/effective user to root: %m\n");
                return 1;
        }

        snprintf(path, sizeof(path), "/etc/network/if-%s.d", action);

        /* Run the actual command in the background, to avoid blocking networkd */
        if (daemon(0, 1) < 0) {
                perror("Failed to daemonize");
                return 1;
        }
        setenv("IFACE", interface, 1);
        execl("/bin/run-parts", "/bin/run-parts", "--lsbsysinit", path, NULL);
        perror("failed to execute run-parts");
        return 1;
}
