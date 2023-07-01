#include <sys/ioctl.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    char str[] = "     Hello         milk-v duo!";
    int fd = open("/dev/oled_128_64_1", O_RDWR);
    write(fd,str,strlen(str));
    close(fd);
    return 0;
}