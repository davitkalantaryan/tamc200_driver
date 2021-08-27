#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <time.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pciedev_io.h>
#include <unistd.h>
#include <sys/ioctl.h>

#define def_nod_name    "/dev/tamc200s5"


int main(int a_argc, char* a_argv[])
{
        const char* nod_name = def_nod_name;
        int fd = -1;
	int ioctl_result;
	int nIteration = 0;


        if(a_argc>1){
                nod_name = a_argv[1];
        }

        fd = open (nod_name, O_RDWR);
        printf("fd=%d\n",fd);
        if(fd<0){
                fprintf(stderr,"Unable to open device %s \n",nod_name);
                return 1;
        }
	
	while(1){
		ioctl_result = ioctl(fd,PCIEDEV_GET_SWAP);
		printf("iteration:%.5d, ioctl_result=%d\n",++nIteration,ioctl_result);
		sleep(1);
	}


        close(fd);

        return 0;
}

