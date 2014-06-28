#include <stdio.h>
#include "sd-id128.h"
void* functions[] = {
sd_id128_to_string,
sd_id128_from_string,
sd_id128_randomize,
sd_id128_get_machine,
sd_id128_get_boot,
};
int main(void) {
unsigned i; for (i=0;i<sizeof(functions)/sizeof(void*);i++) printf("%p\n", functions[i]);
return 0; }
