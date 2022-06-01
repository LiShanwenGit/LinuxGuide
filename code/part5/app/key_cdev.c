#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    unsigned int keyval;
    int fd = 0;
    /* 打开驱动文件 */
    fd = open(argv[1], O_RDWR);
    if (fd<0) printf("can't open %s file\n",argv[1]);

    while(1)
    {
        read(fd, &keyval,sizeof(keyval));
        printf("key_value = %d\n",keyval);
    }
    return 0;
}

