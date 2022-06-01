#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>




int main(int argc, char **argv)
{
    int fd;
    char* filename=NULL;
    int val;
    filename = argv[1];
    fd = open(filename, O_RDWR);//打开dev/设备文件
    if (fd < 0)//小于0说明没有成功
    {
        printf("error, can't open %s\n", filename);
        return 0;
    }
    if(argc !=3)
    {
        printf("usage: ./led_dev.exe [device]  [on/off]\n");  //打印用法
    }
    if(!strcmp(argv[2], "on"))  //如果输入等于on，则LED亮
        val = 0;
    else if(!strcmp(argv[2], "off"))  //如果输入等于off，则LED灭
        val = 1;
    else
        goto error;
    write(fd, &val, 4);//操作LED
    close(fd);
    return 0;
error:
    printf("usage: ./led_dev.exe  [device]  [on/off]\n");  //打印用法
    close(fd);
    return -1;
}

