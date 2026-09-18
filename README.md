# Custom UNIX Shell in C

## Objective

The **Custom UNIX Shell** project aimed to develop a functional command-line interpreter (shell) on Linux using the C programming language. The primary focus was to manage system process lifecycles, handle asynchronous signals safely, implement terminal control transfer, and support job control operations (foreground/background task execution). This hands-on project was designed to deepen low-level operating system concepts, system calls, process concurrency, and dynamic memory management.

### Skills Learned

- Mastered low-level POSIX system calls for process management (`fork`, `execvp`, `waitpid`, `exit`).
- Designed dynamic concurrency management using `SIGCHLD` signal handlers to eliminate zombie processes.
- Implemented process group management (`setpgid`) and terminal control multiplexing (`tcsetpgrp`).
- Built thread/signal-safe data structures using race condition prevention mechanisms (`sigprocmask`).
- Developed custom internal shell built-ins (`cd`, `jobs`, `fg`, `bg`, `logout`).

### Tools Used

- **C Language / GCC Compiler**: Core build environment for low-level system programming.
- **GNU Make & Bash**: Build automation and command-line execution environment.
- **Linux Kernel POSIX APIs**: Signal handling, process control, and terminal allocation interfaces.

---

## Getting Started & Execution Instructions

To compile, start, and run the shell on a Linux environment, follow these steps:

### 1. Compilation
Compile the source code using `gcc`:
```bash
gcc CustomShell.c job_control.c -o MyShell
````

### 2. Starting the Shell
Launch the compiled binary:
```bash
./MyShell
````

## Step-by-Step Implementation Phases

### Step 1: Basic Architecture and Execution Loop
In this initial phase, I implemented the fundamental lifecycle loop of the shell. The program reads input lines from standard input, tokenizes arguments using `get_command()`, and parses background indicators (`&`).

For each external command:
1. The shell creates a child process using `fork()`.
2. The child process replaces its memory image with the requested binary using `execvp()`.
3. For foreground processes, the parent process waits for completion using `waitpid()` and evaluates the termination cause and status code using `analyze_status()`.
4. If a background flag (`&`) is present, the parent does not block and immediately returns to prompt the user for the next command.

---

### Step 2: Built-in Commands Handling
Standard external commands run inside child processes, but commands that alter the shell environment must run directly in the parent process. I implemented two primary built-ins:
- `cd [directory]`: Updates the parent process working directory using the `chdir()` system call. If no argument is provided, it defaults to `HOME`.
- `logout` / `exit`: Terminates the shell process via `exit(0)`.

---

### Step 3: Terminal Control & Signal Masking
To prevent background tasks or suspended jobs from interfering with terminal input/output, I established process group isolation:
1. **Process Groups**: The child process creates its own process group with `new_process_group()` (`setpgid(pid, pid)`).
2. **Terminal Handover**: Before executing foreground commands, the parent or child transfers terminal control to the new process group using `set_terminal(pgid)` (`tcsetpgrp()`).
3. **Signal Isolation**: Terminal signals (`SIGINT`, `SIGTSTP`, `SIGQUIT`, `SIGTTIN`, `SIGTTOU`) are ignored in the parent shell (`ignore_terminal_signals()`) and restored inside the child prior to `execvp()` (`restore_terminal_signals()`). When a foreground process completes or stops, the parent reclaims terminal control.

---

### Step 4: Asynchronous Signal Handler & Zombie Prevention
Executing tasks in the background left processes in a `<defunct>` (zombie) state upon termination because the parent shell was not actively calling `waitpid()` on them.

To solve this, I designed an asynchronous `SIGCHLD` signal handler:
- **Signal Handler Setup**: Installed `sigchld_handler` via `signal(SIGCHLD, handler)`.
- **Non-blocking Reaper**: The handler queries process state changes in a loop using `waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)`.
- **Shared Job List**: Active background and suspended processes are automatically added to, updated in, or removed from a global linked list (`job_list`).

---

### Step 5: Complete Job Control Subsystem (`jobs`, `fg`, `bg`)
In the final phase, I integrated full job management functionality:
1. **Concurrency Control**: Because both the main loop and `sigchld_handler` access and modify `job_list`, critical sections are protected using `block_SIGCHLD()` and `unblock_SIGCHLD()` (`sigprocmask`) to prevent race conditions.
2. **Built-in `jobs`**: Displays all active background and stopped tasks along with their PIDs and states.
3. **Built-in `fg [pos]`**: Brings a background or stopped task into the foreground, transfers terminal control to it, sends `SIGCONT` if stopped, and blocks until completion or re-suspension.
4. **Built-in `bg [pos]`**: Resumes a stopped job in the background by sending `SIGCONT` via `killpg()` and setting its execution state to `BACKGROUND`.
