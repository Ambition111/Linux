#include <stdio.h>
#include <unistd.h>

int main()
{
    int fd[2];
    int ret = pipe(fd);
    if (ret < 0)
    {
        perror("pipe");
        return 0;
    }

    int count = 0;  // 记录到底写了多少次（每次写一个字节）
    while (1)
    {
        if (write(fd[1], "1", 1) == 0)
            break;
        printf("%d\n", count);
        count++;    // 最后一次为65536
    }
    return 0;
}
