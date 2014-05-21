#include <stdio.h>
#include "sd-daemon.h"
void* functions[] = {
sd_booted,
sd_is_fifo,
sd_is_mq,
sd_is_socket,
sd_is_socket_inet,
sd_is_socket_unix,
sd_is_special,
sd_listen_fds,
sd_notify,
sd_notifyf,
};
int main(void) {
unsigned i; for (i=0;i<sizeof(functions)/sizeof(void*);i++) printf("%p\n", functions[i]);
return 0; }
