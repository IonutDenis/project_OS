#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>

#define MAX_NAME 50
#define MAX_CLUE 200
#define TREASURE_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define MAX_PATH 256

typedef struct
{
    char id[20];
    char user[MAX_NAME];
    double lat;
    double lon;
    char clue[MAX_CLUE];
    int value;
} Treasure;

void log_operation(const char *hunt_id, const char *op)
{
    char logpath[MAX_PATH];
    snprintf(logpath, MAX_PATH, "%s/%s", hunt_id, LOG_FILE);

    FILE *log = fopen(logpath, "a");
    if (log)
    {
        time_t now = time(NULL);
        char *time_str = ctime(&now);
        time_str[strlen(time_str) - 1] = '\0';
        fprintf(log, "[%s] %s\n", time_str, op);
        fclose(log);

        char symlink_name[MAX_PATH];
        snprintf(symlink_name, MAX_PATH, "logged_hunt-%s", hunt_id);
        unlink(symlink_name);
        symlink(logpath, symlink_name);
    }
}

// int ensure_hunt_directory(const char *hunt_id)
// {
//     struct stat st = {0};
//     if (stat(hunt_id, &st) == -1)
//     {
//         if (mkdir(hunt_id, 0755) == -1)
//         {
//             perror("Failed to create hunt directory");
//             return 0;
//         }
//     }
//     return 1;
// }

int add_treasure(const char *hunt_id)
{
    if (!ensure_hunt_directory(hunt_id))
    {
        return 0;
    }

    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1)
    {
        perror("Failed to open treasure file");
        return 0;
    }

    Treasure t;
    printf("Enter treasure ID: ");
    scanf("%19s", t.id);
    printf("User name: ");
    scanf("%49s", t.user);
    printf("Latitude, Longitude: ");
    scanf("%lf %lf", &t.lat, &t.lon);
    printf("Clue: ");
    scanf(" %199[^\n]", t.clue);
    printf("Value: ");
    scanf("%d", &t.value);

    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure))
    {
        perror("Failed to write treasure");
        close(fd);
        return 0;
    }

    close(fd);
    log_operation(hunt_id, "ADD");
    return 1;
}

void list_treasures(const char *hunt_id)
{
    char path[MAX_PATH];
    snprintf(path, MAX_PATH, "%s/%s", hunt_id, TREASURE_FILE);

    int fd = open(path, O_RDONLY);
    if (fd == -1)
    {
        printf("No treasures found in hunt '%s'\n", hunt_id);
        return;
    }

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("Failed to get file stats");
        close(fd);
        return;
    }

    printf("\n=== Hunt: %s ===\n", hunt_id);
    printf("File: %s\nSize: %ld bytes\nModified: %s",
           path, st.st_size, ctime(&st.st_mtime));

    Treasure t;
    int count = 0;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        printf("\n--- Treasure %d ---\n", ++count);
        printf("ID: %s\nUser: %s\nLocation: %.6f, %.6f\n",
               t.id, t.user, t.lat, t.lon);
        printf("Value: %d\nClue: %s\n", t.value, t.clue);
    }

    close(fd);
    log_operation(hunt_id, "LIST");
}

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        printf("Usage: %s <operation> <hunt_id>\n", argv[0]);
        printf("Operations:\n");
        printf("  add  - Add a new treasure\n");
        printf("  list - List all treasures\n");
        return 1;
    }

    const char *op = argv[1];
    const char *hunt_id = argv[2];

    if (strcmp(op, "add") == 0)
    {
        if (!add_treasure(hunt_id))
        {
            fprintf(stderr, "Failed to add treasure\n");
            return 1;
        }
    }
    else if (strcmp(op, "list") == 0)
    {
        list_treasures(hunt_id);
    }
    else
    {
        fprintf(stderr, "Unknown operation\n");
        return 1;
    }

    return 0;
}