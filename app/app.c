#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int main()
{
    int fd1 = -1,fd2 = -1;
    char data[1] = {0};
    int readb;
    
    fd1 = open("/dev/matrixkey", O_RDWR);
    fd2 = open("/dev/led_device_0", O_RDWR);

    if(fd2 < 0)
    {
        printf("mo file led_device that bai\n");
        return 0;
    }



    if(fd1 > -1)
    {

        printf("mo file thanh cong\n");
        while(1){
            readb = read(fd1,data,1);
            if(data[0] != 'n')       
            {
                printf("%c , %d bytes\n",data[0],readb);
                write(fd2,data,1);
            }
            usleep(30000);
        }
        close(fd1);
        close(fd2);
    }
    else 
    {
        printf("mo file that bai\n");
        return 0;
    }


    return 0;
}
