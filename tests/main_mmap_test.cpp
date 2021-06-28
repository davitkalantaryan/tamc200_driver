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

#define mmap_len_bar    4096
#define def_nod_name    "/dev/tamc200s5"


int main(int a_argc, char* a_argv[])
{
        const char* nod_name = def_nod_name;
        u_int    tmp_barx;
        uint16_t regVal;
        int fd = -1;
        void* mmap_address=MAP_FAILED;
        unsigned long mmap_offset = (unsigned long)(tmp_barx<< MMAP_BAR_SHIFT);

        printf("mmap test. verson 5\n");

        if(a_argc<2){
                fprintf(stderr,"provide bar number to map\n");
                return 1;
        }
        tmp_barx = (u_int)atoi(a_argv[1]);

        if(a_argc>2){
                nod_name = a_argv[2];
        }
        printf("node_name=%s, bar=%d\n",nod_name,(int)tmp_barx);

        fd = open (nod_name, O_RDWR);
        printf("fd=%d\n",fd);
        if(fd<0){
                fprintf(stderr,"Unable to open device %s \n",nod_name);
                return 1;
        }

        mmap_address = mmap(0, mmap_len_bar, PROT_READ | PROT_WRITE, MAP_SHARED, fd, mmap_offset);
        printf("mapped address: %p\n",mmap_address);

        if(mmap_address==MAP_FAILED){
                close(fd);
                fprintf(stderr,"Unable to map device " "%s" " bar %d\n",nod_name,tmp_barx);
                return 1;
        }

        regVal = ((uint16_t*)mmap_address)[0];
        printf("regVal=%d\n",(int)regVal);
        ((uint16_t*)mmap_address)[0] = regVal;
        munmap(mmap_address, mmap_len_bar);
        close(fd);

        return 0;
}

