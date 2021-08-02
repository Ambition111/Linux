#include <stdio.h>
#include <unistd.h>

int main()
{
    int fd[2];
    int ret = pipe(fd); // 创建管道成功后，内核返回给用户两个文件描述符fd[0]和fd[1],分别操作管道的读端和写端
    if (ret < 0)
    {
        perror("pipe");
        return 0;
    }

    printf("fd[0] = %d\n", fd[0]);
    printf("fd[1] = %d\n", fd[1]);

    while (1)
    {
        sleep(1);
    }

    return 0;
}
