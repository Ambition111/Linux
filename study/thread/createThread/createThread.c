#include <stdio.h>
#include <unistd.h>
#include <pthread.h>


typedef struct thread_info
{
    int thread_num_ = 0;
}THREADINFO;

void* thread_start(void* arg)
{
    pthread_detach(pthread_self());
    THREADINFO* ti = (THREADINFO*)arg;
    //int *i = (int*)arg;
    //while (1)
    //{
        printf("I am new thread~~~:%d\n", ti->thread_num_);
        sleep(1);
    //}
    return NULL;
}

int main()
{
    pthread_t tid;

    int ret;
    //{
        //int i = 10;
        //ret = pthread_create(&tid, NULL, thread_start, (void*));
    //}
    
    for (int i = 0; i < 4; ++i)
    {
        THREADINFO* threadinfo = new THREADINFO();
        threadinfo->thread_num_ = i;
        ret = pthread_create(&tid, NULL, thread_start, (void*)threadinfo);
    
        if (ret < 0)
        {
            perror("pthread_create");
            return 0;
        }
    }

    while (1)
    {
        printf("I am main thread\n");
        sleep(1);
    }

    return 0;
}
