// score_calculator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USER 100
#define MAX_NAME 50

typedef struct
{
    char user[MAX_NAME];
    int total;
} UserScore;

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    char path[256];
    snprintf(path, sizeof(path), "hunts/%s/treasures.dat", argv[1]);
    FILE *file = fopen(path, "rb");
    if (!file)
    {
        perror("Failed to open treasures file");
        return 1;
    }

    typedef struct
    {
        int id;
        char user[MAX_NAME];
        double lat;
        double lon;
        char clue[100];
        int value;
    } Treasure;

    UserScore scores[MAX_USER];
    int user_count = 0;

    Treasure t;
    while (fread(&t, sizeof(Treasure), 1, file))
    {
        int found = 0;
        for (int i = 0; i < user_count; i++)
        {
            if (strcmp(scores[i].user, t.user) == 0)
            {
                scores[i].total += t.value;
                found = 1;
                break;
            }
        }
        if (!found && user_count < MAX_USER)
        {
            strncpy(scores[user_count].user, t.user, MAX_NAME);
            scores[user_count].total = t.value;
            user_count++;
        }
    }

    fclose(file);

    for (int i = 0; i < user_count; i++)
    {
        printf("%s: %d\n", scores[i].user, scores[i].total);
    }

    return 0;
}
