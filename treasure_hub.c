#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#ifndef TREASURE_COMMON_H
#define TREASURE_COMMON_H

#define MAX_NAME 50
#define MAX_CLUE 100

typedef struct
{
    int id;
    char user[MAX_NAME];
    double lat;
    double lon;
    char clue[MAX_CLUE];
    int value;
} Treasure;

#endif

#define USER_INPUT_LEN 256
#define HUNT_DIR "hunts"

static pid_t monitor_pid = 0;

void print_help()
{
    printf("Available commands:\n");
    printf("  start_monitor    - Start background monitoring\n");
    printf("  list_hunts       - List all hunts and treasure counts\n");
    printf("  list_treasures <hunt_id> - List treasures in a hunt\n");
    printf("  view_treasure <hunt_id> <treasure_id> - View specific treasure\n");
    printf("  stop_monitor     - Stop the background monitor\n");
    printf("  exit             - Exit the program\n");
}

void start_monitor()
{
    if (monitor_pid != 0)
    {
        printf("Monitor is already running (PID: %d)\n", monitor_pid);
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Failed to start monitor");
        return;
    }

    if (pid == 0)
    {
        execl("./treasure_monitor", "./treasure_monitor", NULL);
        perror("Failed to start monitor process");
        exit(1);
    }
    else
    {
        monitor_pid = pid;
        printf("Monitor started (PID: %d)\n", pid);
    }
}

void list_hunts()
{
    if (monitor_pid == 0)
    {
        printf("No monitor running. Use 'start_monitor' first.\n");
        return;
    }

    printf("Listing all hunts:\n");

    DIR *dir = opendir(HUNT_DIR);
    if (!dir)
    {
        perror("Failed to open hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[512];
        snprintf(full_path, sizeof(full_path), "%s/%s", HUNT_DIR, entry->d_name);

        struct stat sb;
        if (stat(full_path, &sb) == 0 &&
            S_ISDIR(sb.st_mode) &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0)
        {
            char path[512];
            snprintf(path, sizeof(path), "%s/%s/treasures.dat", HUNT_DIR, entry->d_name);

            struct stat st;
            if (stat(path, &st) == 0)
            {
                int count = st.st_size / sizeof(Treasure);
                printf("- %s (%d treasures)\n", entry->d_name, count);
            }
        }
    }
    closedir(dir);
}

void list_treasures(const char *hunt_id)
{
    if (monitor_pid == 0)
    {
        printf("No monitor running. Use 'start_monitor' first.\n");
        return;
    }

    char command[256];
    snprintf(command, sizeof(command), "./treasure_manager list %s", hunt_id);
    system(command);
}

void view_treasure(const char *hunt_id, const char *treasure_id)
{
    if (monitor_pid == 0)
    {
        printf("No monitor running. Use 'start_monitor' first.\n");
        return;
    }

    char command[256];
    snprintf(command, sizeof(command), "./treasure_manager view %s %s", hunt_id, treasure_id);
    system(command);
}

void stop_monitor()
{
    if (monitor_pid == 0)
    {
        printf("No monitor running\n");
        return;
    }

    printf("Stopping monitor (PID: %d)...\n", monitor_pid);
    kill(monitor_pid, SIGTERM);

    int status;
    waitpid(monitor_pid, &status, 0);
    monitor_pid = 0;

    if (WIFEXITED(status))
    {
        printf("Monitor exited with status %d\n", WEXITSTATUS(status));
    }
    else
    {
        printf("Monitor terminated abnormally\n");
    }
}

int main()
{
    printf("Treasure Hub - Interactive Management System\n");
    print_help();

    char input[USER_INPUT_LEN];
    while (1)
    {
        printf("\ntreasure_hub> ");
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "start_monitor") == 0)
        {
            start_monitor();
        }
        else if (strcmp(input, "list_hunts") == 0)
        {
            list_hunts();
        }
        else if (strncmp(input, "list_treasures ", 15) == 0)
        {
            list_treasures(input + 15);
        }
        else if (strncmp(input, "view_treasure ", 14) == 0)
        {
            char *space = strchr(input + 14, ' ');
            if (space)
            {
                *space = '\0';
                view_treasure(input + 14, space + 1);
            }
            else
            {
                printf("Usage: view_treasure <hunt_id> <treasure_id>\n");
            }
        }
        else if (strcmp(input, "stop_monitor") == 0)
        {
            stop_monitor();
        }
        else if (strcmp(input, "exit") == 0)
        {
            if (monitor_pid != 0)
            {
                printf("Error: Monitor is still running. Use 'stop_monitor' first.\n");
            }
            else
            {
                break;
            }
        }
        else if (strcmp(input, "help") == 0)
        {
            print_help();
        }
        else if (strlen(input) > 0)
        {
            printf("Unknown command. Type 'help' for available commands.\n");
        }
    }

    printf("Exiting Treasure Hub\n");
    return 0;
}
