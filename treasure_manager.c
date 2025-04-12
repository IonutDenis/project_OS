#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define MAX_NAME 50
#define MAX_CLUE 100
#define HUNT_DIR "hunts"

typedef struct
{
    int id;
    char user[MAX_NAME];
    double lat;
    double lon;
    char clue[MAX_CLUE];
    int value;
} Treasure;

void print_help()
{
    printf("Usage:\n");
    printf("  ./treasure_manager add <hunt_id> - Add treasure to hunt\n");
    printf("  ./treasure_manager list <hunt_id> - List treasures in hunt\n");
    printf("  ./treasure_manager view <hunt_id> <treasure_id> - View treasure\n");
    printf("  ./treasure_manager remove <hunt_id> <treasure_id> - Remove treasure\n");
}

void log_operation(const char *hunt_id, const char *message)
{
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "hunts/%s/logged_hunt", hunt_id);

    FILE *log = fopen(log_path, "a");
    if (!log)
    {
        perror("Failed to open log file");
        return;
    }

    time_t now = time(NULL);
    char *time_str = ctime(&now);
    if (time_str)
    {
        time_str[strcspn(time_str, "\n")] = 0;
    }

    fprintf(log, "[%s] %s\n", time_str ? time_str : "Unknown time", message);
    fclose(log);
}

int create_hunt_dir(const char *hunt_id)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s", HUNT_DIR, hunt_id);

    if (access(path, F_OK) == -1)
    {
      
        int fd = open(path, O_CREAT | O_WRONLY, 0644);
        if (fd == -1)
        {
            perror("Failed to create hunt directory file");
            return -1;
        }

        close(fd);
        if (unlink(path) == -1)
        {
            perror("Failed to remove temporary file to leave directory");
            return -1;
        }
    }
    return 0;
}

int get_treasure_file(const char *hunt_id, int flags)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/%s/treasures.dat", HUNT_DIR, hunt_id);

    int fd = open(path, flags, 0644);
    if (fd == -1)
    {
        perror("Failed to open treasure file");
    }
    return fd;
}

int add_treasure(const char *hunt_id)
{
    if (create_hunt_dir(hunt_id) == -1)
    {
        return -1;
    }

    Treasure t;
    printf("Enter treasure details:\n");

    printf("ID: ");
    scanf("%d", &t.id);

    printf("User: ");
    scanf("%49s", t.user);

    printf("Latitude: ");
    scanf("%lf", &t.lat);

    printf("Longitude: ");
    scanf("%lf", &t.lon);

    printf("Clue: ");
    scanf(" %99[^\n]", t.clue);

    printf("Value: ");
    scanf("%d", &t.value);

    int fd = get_treasure_file(hunt_id, O_WRONLY | O_CREAT | O_APPEND);
    if (fd == -1)
        return -1;

    if (write(fd, &t, sizeof(Treasure)) != sizeof(Treasure))
    {
        perror("Failed to write treasure");
        close(fd);
        return -1;
    }

    close(fd);

    char msg[256];
    snprintf(msg, sizeof(msg), "Added treasure ID %d by user %s", t.id, t.user);
    log_operation(hunt_id, msg);

    printf("Treasure added successfully!\n");
    return 0;
}

void list_treasures(const char *hunt_id)
{
    int fd = get_treasure_file(hunt_id, O_RDONLY);
    if (fd == -1)
        return;

    struct stat st;
    if (fstat(fd, &st) == -1)
    {
        perror("Failed to get file info");
        close(fd);
        return;
    }

    printf("\nHunt: %s\n", hunt_id);
    printf("File size: %ld bytes\n", st.st_size);
    printf("Last modified: %ld\n\n", (long)st.st_mtime);

    Treasure t;
    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        printf("ID: %d\n", t.id);
        printf("User: %s\n", t.user);
        printf("Location: %.6f, %.6f\n", t.lat, t.lon);
        printf("Clue: %s\n", t.clue);
        printf("Value: %d\n\n", t.value);
    }

    close(fd);

    log_operation(hunt_id, "Listed all treasures");
}

void view_treasure(const char *hunt_id, int treasure_id)
{
    int fd = get_treasure_file(hunt_id, O_RDONLY);
    if (fd == -1)
        return;

    Treasure t;
    int found = 0;

    while (read(fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if (t.id == treasure_id)
        {
            found = 1;
            printf("\nTreasure found:\n");
            printf("ID: %d\n", t.id);
            printf("User: %s\n", t.user);
            printf("Location: %.6f, %.6f\n", t.lat, t.lon);
            printf("Clue: %s\n", t.clue);
            printf("Value: %d\n", t.value);
            break;
        }
    }

    if (!found)
    {
        printf("Treasure with ID %d not found\n", treasure_id);
    }

    close(fd);

    char msg[256];
    if (found)
        snprintf(msg, sizeof(msg), "Viewed treasure ID %d", treasure_id);
    else
        snprintf(msg, sizeof(msg), "Failed to find treasure ID %d", treasure_id);
    log_operation(hunt_id, msg);
}

int remove_treasure(const char *hunt_id, int treasure_id)
{
    char temp_path[256];
    snprintf(temp_path, sizeof(temp_path), "%s/%s/treasures.tmp", HUNT_DIR, hunt_id);

    int src_fd = get_treasure_file(hunt_id, O_RDONLY);
    if (src_fd == -1)
        return -1;

    int dst_fd = open(temp_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (dst_fd == -1)
    {
        close(src_fd);
        return -1;
    }

    Treasure t;
    int found = 0;

    while (read(src_fd, &t, sizeof(Treasure)) == sizeof(Treasure))
    {
        if (t.id == treasure_id)
        {
            found = 1;
        }
        else
        {
            write(dst_fd, &t, sizeof(Treasure));
        }
    }

    close(src_fd);
    close(dst_fd);

    if (!found)
    {
        unlink(temp_path);
        printf("Treasure not found\n");
        return -1;
    }

    char orig_path[256];
    snprintf(orig_path, sizeof(orig_path), "%s/%s/treasures.dat", HUNT_DIR, hunt_id);

    if (rename(temp_path, orig_path) == -1)
    {
        perror("Failed to replace treasure file");
        return -1;
    }

    printf("Treasure removed successfully\n");

    if (found)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Removed treasure ID %d", treasure_id);
        log_operation(hunt_id, msg);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }

    if (create_hunt_dir(HUNT_DIR) == -1)
    {
        return 1;
    }

    if (strcmp(argv[1], "add") == 0 && argc == 3)
    {
        return add_treasure(argv[2]);
    }
    else if (strcmp(argv[1], "list") == 0 && argc == 3)
    {
        list_treasures(argv[2]);
        return 0;
    }
    else if (strcmp(argv[1], "view") == 0 && argc == 4)
    {
        view_treasure(argv[2], atoi(argv[3]));
        return 0;
    }
    else if (strcmp(argv[1], "remove") == 0 && argc == 4)
    {
        return remove_treasure(argv[2], atoi(argv[3]));
    }
    else
    {
        print_help();
        return 1;
    }
}
