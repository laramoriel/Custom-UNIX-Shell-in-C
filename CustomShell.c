/*------------------------------------------------------------------------------
UNIX Shell Implementation in C

To compile:
    gcc CustomShell.c job_control.c -o MyShell

To execute:
    ./MyShell

To exit:
    Press Ctrl+D or type "logout"
------------------------------------------------------------------------------*/

#include "job_control.h"
#include <string.h>

#define MAX_LINE 256

job *job_list;

// Signal handler for background process updates (SIGCHLD)
void sigchld_handler(int sig)
{
  int status, info;
  pid_t pid;
  job *item;

  while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0)
  {
    item = get_item_bypid(job_list, pid);
    if (item != NULL)
    {
      enum status st = analyze_status(status, &info);
      if (st == TERMINATED)
      {
        printf("\n[Background process PID %d (%s) finished]\n", pid, item->command);
        delete_job(job_list, item);
      }
      else if (st == SUSPENDED)
      {
        printf("\n[Background process PID %d (%s) suspended]\n", pid, item->command);
        item->ground = STOPPED;
      }
    }
  }
}

int main(void)
{
  char inputBuffer[MAX_LINE];
  int background;
  char *args[MAX_LINE / 2];
  int pid_fork, pid_wait;
  int status, info;
  enum status status_res;

  // Ignore terminal signals in the main shell prompt
  ignore_terminal_signals();

  // Install SIGCHLD signal handler
  signal(SIGCHLD, sigchld_handler);

  // Initialize background job list
  job_list = new_list("Shell Job List");

  while (1)
  {
    printf("COMMAND-> ");
    fflush(stdout);

    get_command(inputBuffer, MAX_LINE, args, &background);

    if (args[0] == NULL) continue; // Skip empty input

    // Built-in command: cd
    if (strcmp(args[0], "cd") == 0)
    {
      if (args[1] == NULL)
      {
        chdir(getenv("HOME"));
      }
      else if (chdir(args[1]) != 0)
      {
        perror("cd failed");
      }
      continue;
    }

    // Built-in command: logout / exit
    if (strcmp(args[0], "logout") == 0 || strcmp(args[0], "exit") == 0)
    {
      exit(0);
    }

    // Built-in command: jobs
    if (strcmp(args[0], "jobs") == 0)
    {
      block_SIGCHLD();
      if (empty_list(job_list))
      {
        printf("No active background jobs.\n");
      }
      else
      {
        print_job_list(job_list);
      }
      unblock_SIGCHLD();
      continue;
    }

    // Built-in command: fg
    if (strcmp(args[0], "fg") == 0)
    {
      int pos = (args[1] != NULL) ? atoi(args[1]) : 1;
      block_SIGCHLD();
      job *item = get_item_bypos(job_list, pos);
      if (item != NULL)
      {
        set_terminal(item->pgid);
        item->ground = FOREGROUND;
        if (item->ground == STOPPED)
        {
          killpg(item->pgid, SIGCONT);
        }
        pid_wait = waitpid(item->pgid, &status, WUNTRACED);
        set_terminal(getpid());
        status_res = analyze_status(status, &info);
        if (status_res == TERMINATED)
        {
          delete_job(job_list, item);
        }
        else if (status_res == SUSPENDED)
        {
          item->ground = STOPPED;
        }
      }
      else
      {
        printf("fg: job not found\n");
      }
      unblock_SIGCHLD();
      continue;
    }

    // Built-in command: bg
    if (strcmp(args[0], "bg") == 0)
    {
      int pos = (args[1] != NULL) ? atoi(args[1]) : 1;
      block_SIGCHLD();
      job *item = get_item_bypos(job_list, pos);
      if (item != NULL && item->ground == STOPPED)
      {
        item->ground = BACKGROUND;
        killpg(item->pgid, SIGCONT);
      }
      else
      {
        printf("bg: job not found or already running\n");
      }
      unblock_SIGCHLD();
      continue;
    }

    // External commands execution
    pid_fork = fork();

    if (pid_fork < 0)
    {
      perror("Fork failed");
      exit(-1);
    }

    if (pid_fork == 0) // Child process
    {
      new_process_group(getpid());

      if (!background)
      {
        set_terminal(getpid());
      }

      restore_terminal_signals();

      execvp(args[0], args);
      printf("Error: Command '%s' not found.\n", args[0]);
      exit(-1);
    }
    else // Parent process
    {
      new_process_group(pid_fork);

      if (!background)
      {
        set_terminal(pid_fork);
        pid_wait = waitpid(pid_fork, &status, WUNTRACED);
        set_terminal(getpid());

        status_res = analyze_status(status, &info);
        if (status_res == SUSPENDED)
        {
          printf("Foreground command %s (PID %d) suspended.\n", args[0], pid_fork);
          block_SIGCHLD();
          add_job(job_list, new_job(pid_fork, args[0], STOPPED));
          unblock_SIGCHLD();
        }
        else
        {
          printf("Foreground command %s executed with PID %d. Exit status: %s. Info: %d.\n",
                 args[0], pid_fork, status_strings[status_res], info);
        }
      }
      else
      {
        printf("Background command %s running with PID %d.\n", args[0], pid_fork);
        block_SIGCHLD();
        add_job(job_list, new_job(pid_fork, args[0], BACKGROUND));
        unblock_SIGCHLD();
      }
    }
  }
  return 0;
}