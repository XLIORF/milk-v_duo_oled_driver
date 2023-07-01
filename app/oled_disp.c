#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>

struct reg_access_transimission
{
    char * buffer;
    unsigned int len;
}reg_access_transimission;

int main(int argc, char* argv[])
{

    reg_access_transimission.len = 1;
    reg_access_transimission.buffer = malloc(reg_access_transimission.len);
    int fd = open("/dev/oled_128_64_1", O_RDWR);
    ioctl(fd,_IOW('o', 1, struct reg_access_transimission),&reg_access_transimission);
    free(reg_access_transimission.buffer);
    return 0;
}