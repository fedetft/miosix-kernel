#include <cstdio>
#include <ctime>

int main()
{
    printf("Hi! I'm the process #2\n");

    printf("I'll write 10 letters\n");
    const timespec sleep_time{.tv_sec = 1, .tv_nsec = 0};
    for (int i = 1; i <= 10; i++)
    {
        nanosleep(&sleep_time, NULL);
        printf("%c\n", 'Z' - i);
    }

    return 0;
}
