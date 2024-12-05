#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "lunix-chrdev.h"

int main() {
    int fd;
    int mode = 0;           
    char buffer[256];       
    ssize_t bytes_read;

    fd = open("/dev/lunix0-batt", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    (void)! ioctl(fd, LUNIX_IOC_RAW_DATA, &mode);
    while(1) {
        bytes_read = read(fd, buffer, sizeof(buffer) - 1);
        if (bytes_read < 0) {
            perror("read failed");
            break;
        }

        buffer[bytes_read] = '\0';  
        printf("Read from device: %s\n", buffer);

        sleep(1);
    }

    close(fd);
    return 0;
}
