#include <cstdio>
#include <ctime>

int main()
{
    printf("Hi! I'm the process #1\n");

    printf("I'll count to 10\n");
    const timespec sleep_time{.tv_sec = 1, .tv_nsec = 0};
    for (int i = 1; i <= 10; i++)
    {
        nanosleep(&sleep_time, NULL);
        printf("%d\n", i);
    }

    return 0;
}
