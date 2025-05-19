#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

int main()
{
    while (1)
    {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        if (time_str)
        {
            time_str[strcspn(time_str, "\n")] = 0;
        }

        printf("[Monitor] Everything looks good at %s\n", time_str ? time_str : "Unknown time");
        fflush(stdout);
        sleep(5); // wait 5 seconds
    }
    return 0;
}
