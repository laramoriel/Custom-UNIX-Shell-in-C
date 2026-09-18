/*------------------------------------------------------------------------------
Linux Shell Project
Function prototypes, macros, and data structures for job control operations.

This file was provided by my proffesor

Operating Systems Course
Dept. of Computer Architecture - UMA
------------------------------------------------------------------------------*/

#ifndef _JOB_CONTROL_H
#define _JOB_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

// -----------------------   ENUMERATED DATA TYPES   -----------------------
enum status { SUSPENDED, RESUMED, TERMINATED };
enum ground { FOREGROUND, BACKGROUND, STOPPED };
static char* status_strings[] = { "Suspended", "Resumed", "Terminated" };
static char* ground_strings[] = { "Foreground", "Background", "Stopped" };

// -----------------   JOB STRUCTURE FOR JOBS LIST   ----------------
typedef struct job_
{
  pid_t pgid;          // Process Group ID (matches the leader process)
  char *command;       // Program name/command string
  enum ground ground;  // Execution state (FG, BG, STOPPED)
  struct job_ *next;   // Pointer to next job in list
} job;

// --------------------------   PUBLIC FUNCTIONS   ---------------------------
void get_command(char inputBuffer[], int size, char *args[], int *background);
job *new_job(pid_t pid, const char *command, enum ground place);
void add_job(job *list, job *item);
int delete_job(job *list, job *item);
job *get_item_bypid(job *list, pid_t pid);
job *get_item_bypos(job *list, int n);
enum status analyze_status(int status, int *info);

// -----   INTERNAL FUNCTIONS (PREFERRED VIA MACROS BELOW)   ----
void print_item(job *item);
void print_list(job *list, void (*print)(job *));
void terminal_signals(void (*func)(int));
void block_signal(int signal, int block);

// ----------------------------   PUBLIC MACROS   ----------------------------

#define list_size(list) list->pgid // Number of jobs in the list
#define empty_list(list) !(list->pgid) // Returns 1 if list is empty 
#define new_list(name) new_job(0, name, FOREGROUND) 
#define print_job_list(list) print_list(list, print_item)

#define restore_terminal_signals() terminal_signals(SIG_DFL)
#define ignore_terminal_signals()  terminal_signals(SIG_IGN)

#define set_terminal(pid)          tcsetpgrp(STDIN_FILENO, pid)
#define new_process_group(pid)     setpgid(pid, pid)

#define block_SIGCHLD()   	   block_signal(SIGCHLD, 1)
#define unblock_SIGCHLD() 	   block_signal(SIGCHLD, 0)

// --------------------   DEBUGGING MACRO   --------------------
#define debug(x,fmt) fprintf(stderr,"\"%s\":%u:%s(): --> %s= " #fmt " (%s)\n", __FILE__, __LINE__, __FUNCTION__, #x, x, #fmt)

#endif