#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>  
#include <time.h>    

#define MAP_SIZE 4096  

int main() {
    const char *device_path = "/dev/lunix0-batt"; 

    int fd = open(device_path, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return -1;
    }

    void *mapped_data = mmap(NULL, MAP_SIZE, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mapped_data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return -1;
    }

    uint32_t *sensor_data = (uint32_t *)mapped_data;

    while (1) {
        printf("Sensor Data: %u\n", sensor_data[2]); 
        sleep(1); 
    }

    close(fd);

    return 0;
}
