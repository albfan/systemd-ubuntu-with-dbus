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
        const char* action, *interface, *nameservers;
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

        /* set up clean environment, only salvage defined vars */
        nameservers = getenv("IF_DNS_NAMESERVERS");
        clearenv();
        setenv("PATH", "/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin", 1);
        if (nameservers)
                setenv("IF_DNS_NAMESERVERS", nameservers, 1);

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
        /* some other env vars that scripts might expect */
        setenv("LOGICAL", interface, 1);
        setenv("METHOD", "networkd", 1);
        setenv("ADDRFAM", "inet", 1);
        setenv("VERBOSITY", "0", 1);
        if (strcmp(action, "up") == 0) {
                setenv("MODE", "start", 1);
                setenv("PHASE", "post-up", 1);
        } else {
                setenv("MODE", "stop", 1);
                setenv("PHASE", "post-down", 1);
        }
        execl("/bin/run-parts", "/bin/run-parts", "--lsbsysinit", path, NULL);
        perror("failed to execute run-parts");
        return 1;
}
