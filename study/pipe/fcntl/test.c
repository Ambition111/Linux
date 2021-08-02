#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main()
{
    int fd[2];
    int ret = pipe(fd);
    if (fd < 0)
    {
        perror("pipe");
        return 0;
    }
    
    // 获取读端文件描述符属性
    int flagRead = fcntl(fd[0], F_GETFL);
    printf("flagRead = %d\n", flagRead);

    // 获取写端文件描述符属性
    int flagWrite = fcntl(fd[1], F_GETFL);
    printf("flagWrite = %d\n", flagWrite);

    flagWrite |= O_NONBLOCK;
    fcntl(fd[1], F_SETFL, flagWrite);
    printf("O_NONBLOCK = %d\n", flagWrite);

    return 0;
}
