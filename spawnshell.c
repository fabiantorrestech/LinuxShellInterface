/* $begin shellmain */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>

#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#define MAXARGS 128
#define MAXLINE 8192 /* Max text line length */

extern char **environ; /* Defined by libc */

int pipe_idx = -1;
int wait_idx = -1;
/* Function prototypes */
void eval(char *cmdline);
int parseline(char *buf, char **argv, posix_spawn_file_actions_t* actions, posix_spawn_file_actions_t* actions_2, int* pipe_fds);
int builtin_command(char **argv);

void unix_error(char *msg) /* Unix-style error */
{
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

// -- my added signal handler for ctrl+c and ctrl+z
static void signal_handler(int a_signal)
{
  if(a_signal == SIGINT)
  {
    write(STDOUT_FILENO, "\ncaught sigint\nCS361 >", 22);
  }
  else if(a_signal == SIGTSTP)
  {
    write(STDOUT_FILENO, "\ncaught sigstop\nCS361 >", 23);
  }
}

// ----- main function -----
int main() {
  char cmdline[MAXLINE]; /* Command line */
  signal(SIGINT,signal_handler);
  signal(SIGTSTP, signal_handler);
  while (1) {
    char *result;
    /* Read */
    printf("CS361 > ");
    result = fgets(cmdline, MAXLINE, stdin);
    if (result == NULL && ferror(stdin)) {
      fprintf(stderr, "fatal fgets error\n");
      exit(EXIT_FAILURE);
    }

    if (feof(stdin)) exit(0);

    /* Evaluate */
    eval(cmdline);
  }
}
// ---- end main function ----

// ---- eval ----
/* eval - Evaluate a command line */
void eval(char *cmdline) {
  char *argv[MAXARGS]; /* Argument list execve() */
  char buf[MAXLINE];   /* Holds modified command line */
  int bg;              /* Should the job run in bg or fg? */
  pid_t pid;           // Process id / result
  pid_t pid2;          // process id / result 2

  // --- fab added variables ---
  pipe_idx = -1;
  wait_idx = -1;
  posix_spawn_file_actions_t actions;
  posix_spawn_file_actions_init(&actions);
  posix_spawn_file_actions_t actions_2;
  posix_spawn_file_actions_init(&actions_2);
  int pipe_fds[2];                          // pipe[0] = read end
                                            // pipe[1] = write end
  pipe(pipe_fds);


  strcpy(buf, cmdline);
  bg = parseline(buf, argv, &actions, &actions_2, &pipe_fds);
  if (argv[0] == NULL)
    return; /* Ignore empty lines */

  if (!builtin_command(argv)) {

    // run the command 'argv' using posix_spawnp.
    if (0 != posix_spawnp(&pid, argv[0], &actions, NULL, argv, environ)) {
      perror("spawn failed");
      exit(1);
    }
    
    // for piping case to do the 2nd command.
    if(pipe_idx != -1)
    {
      if (0 != posix_spawnp(&pid2, argv[pipe_idx], &actions_2, NULL, argv+pipe_idx, environ)) {
        perror("spawn failed");
        exit(1);
      }
    }
    
    // Close the read and write ends in the parent process.
    if(pipe_idx != -1)
    {
      close(pipe_fds[0]);
      close(pipe_fds[1]);
    }

    /* Parent waits for foreground job to terminate */
    if (!bg) {
      int status;
      int status_two;
      if (waitpid(pid, &status, 0) < 0)
       unix_error("waitfg: waitpid error");
      
      
       // WAIT case
      if(wait_idx > 0)
      {
        if (0 != posix_spawnp(&pid2, argv[wait_idx], &actions_2, NULL, argv+wait_idx, environ)) {
          perror("spawn failed");
          exit(1);
        }

        if (waitpid(pid2, &status_two, 0) < 0)
          unix_error("waitfg: waitpid error");

      }
      
    } 

    // bg != 0
    else
    {
      //int status_three;
      printf("%d %s", pid, cmdline);
      
      /*
      int olderrno = errno;
      while (waitpid(-1, NULL, 0) > 0) 
      {
        Sio_puts("Handler reaped child\n");
      }
      if (errno != ECHILD)
        Sio_error("waitpid error");

      Sleep(1);
      errno = olderrno;
      */

    }
  }

  return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) {
  if (!strcmp(argv[0], "exit")) /* exit command */
    exit(0);
  if (!strcmp(argv[0], "&")) /* Ignore singleton & */
    return 1;

  // TODO: implement "?" commands!

  return 0; /* Not a builtin command */
}
// ---- end eval ----

/* $begin parseline */
/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv, posix_spawn_file_actions_t* actions, posix_spawn_file_actions_t* actions_2, int* pipe_fds) {
  char *delim; /* Points to first space delimiter */
  int argc;    /* Number of args */
  int bg;      /* Background job? */

  buf[strlen(buf) - 1] = ' ';   /* Replace trailing '\n' with space */
  while (*buf && (*buf == ' ')) /* Ignore leading spaces */
    buf++;

  argc = 0;
  while ((delim = strchr(buf, ' '))) {
    argv[argc++] = buf;
    *delim = '\0';
    buf = delim + 1;
    while (*buf && (*buf == ' ')) /* Ignore spaces */
      buf++;
  }
  argv[argc] = NULL;

  if (argc == 0) /* Ignore blank line */
    return 1;

  /* Should the job run in the background? */
  if ((bg = (*argv[argc - 1] == '&')) != 0)
   argv[--argc] = NULL;
  //printf("bg = %d\n\n", bg);
  //printf("argc: %d\n", argc);


  // -- fab added code to print and set flags. --
  // <
  // >
  // < >
  // !
  // |
  // 0 = normal/unchanged
  // 1 = output redirection
  // 2 = input redirection
  // 3 = both redirection
  // 4 = pipe
  // 5  = wait

  int i = 0;
  while(argv[i] != NULL)
  {
    //printf("argv[%d]: %s\n", i, argv[i]);
    if(strcmp(argv[i], ">") == 0)
    {
      argv[i] = NULL;
      //printf("> detected...\n");
      posix_spawn_file_actions_addopen(actions, STDOUT_FILENO, argv[i+1],
                                  O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);

      //printf("2 > detected...\n");
    }
    else if(strcmp(argv[i], "<") == 0)
    {
      argv[i] = NULL;
      posix_spawn_file_actions_addopen(actions, STDIN_FILENO, argv[i+1],
                                  O_RDONLY, S_IRUSR | S_IRGRP);
      //printf("< detected...\n");
    }
    else if(strcmp(argv[i], "|") == 0)
    {
      argv[i] = NULL;
      pipe_idx = i+1;
      posix_spawn_file_actions_adddup2(actions, pipe_fds[1], STDOUT_FILENO);
      posix_spawn_file_actions_addclose(actions, pipe_fds[0]);
      posix_spawn_file_actions_adddup2(actions_2, pipe_fds[0], STDIN_FILENO);
      posix_spawn_file_actions_addclose(actions_2, pipe_fds[1]);
      //printf("| detected...\n");
    }
    else if(strcmp(argv[i], ";") == 0)
    {
      argv[i] = NULL;
      wait_idx = i+1;
      //printf("; detected...\n");
    }
    i++;
  }

  return bg;
}
/* $end parseline */
