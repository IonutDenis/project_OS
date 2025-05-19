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
#include <sys/wait.h>

#define MAX_NAME 50
#define MAX_CLUE 100
#define USER_INPUT_LEN 256
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

static pid_t monitor_pid = 0;
static int monitor_pipe[2];

void print_help()
{
    printf("Available commands:\n");
    printf("  start_monitor         - Start background monitoring\n");
    printf("  list_hunts            - List all hunts and treasure counts\n");
    printf("  list_treasures <id>   - List treasures in a hunt\n");
    printf("  view_treasure <hunt> <treasure_id> - View specific treasure\n");
    printf("  calculate_score       - Show user scores in each hunt\n");
    printf("  stop_monitor          - Stop the background monitor\n");
    printf("  exit                  - Exit the program\n");
}

void start_monitor()
{
    if (monitor_pid != 0)
    {
        printf("Monitor is already running (PID: %d)\n", monitor_pid);
        return;
    }

    if (pipe(monitor_pipe) == -1)
    {
        perror("pipe failed");
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        perror("Failed to fork monitor");
        return;
    }

    if (pid == 0)
    {
        // Child process
        close(monitor_pipe[0]);
        dup2(monitor_pipe[1], STDOUT_FILENO);
        close(monitor_pipe[1]);
        execl("./treasure_monitor", "./treasure_monitor", NULL);
        perror("Failed to start monitor");
        exit(1);
    }
    else
    {
        // Parent
        close(monitor_pipe[1]);
        monitor_pid = pid;
        printf("Monitor started (PID: %d)\n", pid);
    }
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

    close(monitor_pipe[0]);
}

#include <fcntl.h>

void read_monitor_output()
{
    char buffer[256];
    ssize_t n;

    int flags = fcntl(monitor_pipe[0], F_GETFL, 0);
    fcntl(monitor_pipe[0], F_SETFL, flags | O_NONBLOCK);

    while ((n = read(monitor_pipe[0], buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[n] = '\0';
        printf("%s", buffer);
    }

    fcntl(monitor_pipe[0], F_SETFL, flags);
}

void list_hunts()
{
    if (monitor_pid == 0)
    {
        printf("No monitor running. Use 'start_monitor' first.\n");
        return;
    }

    read_monitor_output();

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

    read_monitor_output();

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

    read_monitor_output();

    char command[256];
    snprintf(command, sizeof(command), "./treasure_manager view %s %s", hunt_id, treasure_id);
    system(command);
}

void calculate_score()
{
    DIR *dir = opendir(HUNT_DIR);
    if (!dir)
    {
        perror("Failed to open hunts directory");
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_DIR &&
            strcmp(entry->d_name, ".") != 0 &&
            strcmp(entry->d_name, "..") != 0)
        {

            int fd[2];
            if (pipe(fd) == -1)
            {
                perror("pipe");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0)
            {
                // Child
                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
                close(fd[1]);
                execl("./score_calculator", "score_calculator", entry->d_name, NULL);
                perror("Failed to run score_calculator");
                exit(1);
            }
            else
            {
                // Parent
                close(fd[1]);
                char buffer[256];
                ssize_t n;
                printf("\nScores for hunt: %s\n", entry->d_name);
                while ((n = read(fd[0], buffer, sizeof(buffer) - 1)) > 0)
                {
                    buffer[n] = '\0';
                    printf("%s", buffer);
                }
                close(fd[0]);
                waitpid(pid, NULL, 0);
            }
        }
    }

    closedir(dir);
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
        else if (strcmp(input, "calculate_score") == 0)
        {
            calculate_score();
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
