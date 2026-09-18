/*------------------------------------------------------------------------------
Linux Shell Project
Job Control Helper Module

This file was provided by my proffesor

Operating Systems Course
Dept. of Computer Architecture - UMA
------------------------------------------------------------------------------*/

#include "job_control.h"
#ifndef __APPLE__
#include <malloc.h>
#endif
#include <string.h>

// Reads user input from standard input, tokenizing into args[]
void get_command(char inputBuffer[], int size, char *args[], int *background)
{
  int length, i, start, ct;

  ct = 0;
  *background = 0;

  length = read(STDIN_FILENO, inputBuffer, size);  

  start = -1;
  if (length == 0)
  {
    printf("\nExiting Shell\n");
    exit(0); // EOF / Ctrl+D pressed
  } 
  if (length < 0)
  {
    perror("Error reading command");
    exit(-1);
  }

  for (i = 0; i < length; i++) 
  { 
    switch (inputBuffer[i])
    {
    case ' ':
    case '\t':
      if (start != -1)
      {
        args[ct] = &inputBuffer[start];
        ct++;
      }
      inputBuffer[i] = '\0';
      start = -1;
      break;
    case '\n':
      if (start != -1)
      {
        args[ct] = &inputBuffer[start];     
        ct++;
      }
      inputBuffer[i] = '\0';
      args[ct] = NULL;
      break;
    default:
      if (inputBuffer[i] == '&') // Background process flag
      {
        *background = 1;
        if (start != -1)
        {
          args[ct] = &inputBuffer[start];     
          ct++;
        }
        inputBuffer[i] = '\0';
        args[ct] = NULL;
        i = length;
      }
      else if (start == -1) start = i;
    }
  }   
  args[ct] = NULL;
} 

// Allocates and creates a new job structure node
job *new_job(pid_t pid, const char *command, enum ground place)
{
  job *aux = (job *) malloc(sizeof(job));
  aux->pgid = pid;
  aux->command = strdup(command);
  aux->ground = place;
  aux->next = NULL;
  return aux;
}

// Inserts an item at the beginning of the job list
void add_job(job *list, job *item)
{
  job *aux = list->next;
  list->next = item;
  item->next = aux;
  list->pgid++;
}

// Deletes a specific job item from the job list
int delete_job(job *list, job *item)
{
  job *aux = list;
  while ((aux->next != NULL) && (aux->next != item)) 
    aux = aux->next;
  if (aux->next)
  {
    aux->next = item->next;
    free(item->command);
    free(item);
    list->pgid--;
    return 1;
  }
  else
    return 0;
}

// Finds a job item by Process Group ID (PID)
job *get_item_bypid(job *list, pid_t pid)
{
  job *aux = list;
  while ((aux->next != NULL) && (aux->next->pgid != pid)) 
    aux = aux->next;
  return aux->next;
}

// Finds a job item by its position index in the list
job *get_item_bypos(job *list, int n)
{
  job *aux = list;
  if ((n < 1) || (n > list->pgid))
    return NULL;
  n--;
  while ((aux->next != NULL) && n)
  { 
    aux = aux->next; 
    n--;
  }
  return aux->next;
}

// Displays single job details
void print_item(job *item)
{
  printf("PID %d. Command: %s. State: %s.\n", 
         item->pgid, item->command, ground_strings[item->ground]);
}

// Iterates through and prints all jobs in the list
void print_list(job *list, void (*print)(job *))
{
  int n = 1;
  job *aux = list;
  printf("Contents of %s:\n", list->command);
  while(aux->next != NULL) 
  {
    printf(" [%d] ", n);
    print(aux->next);
    n++;
    aux = aux->next;
  }
}

// Analyzes status returned by waitpid()
enum status analyze_status(int status, int *info)
{
  if (WIFSTOPPED(status))
  {
    *info = WSTOPSIG(status);
    return(SUSPENDED);
  }
  else
  {
    if (WIFSIGNALED(status))
    { 
      *info = WTERMSIG(status); 
      return(RESUMED);
    }
    else
    { 
      *info = WEXITSTATUS(status); 
      return(TERMINATED);
    }
  }
}

// Sets signal actions for terminal-related signals
void terminal_signals(void (*func) (int))
{
  signal(SIGINT,  func); // CTRL+C
  signal(SIGQUIT, func); // CTRL+\
  signal(SIGTSTP, func); // CTRL+Z
  signal(SIGTTIN, func); // BG process reads from terminal
  signal(SIGTTOU, func); // BG process writes to terminal
}		

// Blocks or unblocks specific signal
void block_signal(int signal, int block)
{
  sigset_t block_sigchld;
  sigemptyset(&block_sigchld);
  sigaddset(&block_sigchld, signal);
  if (block)
    sigprocmask(SIG_BLOCK, &block_sigchld, NULL);
  else
    sigprocmask(SIG_UNBLOCK, &block_sigchld, NULL);
}