
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

const char * const * sys_errlist;
int sys_nerr;
int errno;

#define PIPE_READ_END	STDIN_FILENO
#define PIPE_WRITE_END	STDOUT_FILENO

pid_t
popen_dup(int* fdout_ptr, char** envp, int nargs, ...)
{
  va_list ap;
  pid_t childpid;
  char **args;
  int n = nargs;

  int fdout[2];

  args = (char **)malloc(nargs * sizeof(char *));
  va_start(ap, nargs);
  do
    args[nargs -n] = va_arg(ap, char *);
  while (--n);

  if ((childpid = fork()) == -1)
    {
      perror("fork");
      exit(1);
    }

  if (pipe2(fdout, 0) == -1)
    {
      perror("pipe2");
      return -1;     
    }

  switch(childpid)
    {
    case 0:
      fprintf(stdout, "In child process (pid = %d)\n", getpid());

      close(fdout[PIPE_READ_END]);
      close(STDOUT_FILENO);
      if (dup(fdout[PIPE_WRITE_END]) == -1)
        {
          perror("dup write end fd of output pipe");
          return -1;
        }

      if (execve(args[0], args, envp) == -1)
        {
          perror("execv child command");
          return -1;
        }

      /* Never reached ... */
      while (1);
      break;

    default:
      fprintf(stdout, "In parent process (pid = %d): PID of child process = %d\n", getpid(), childpid);
      close(fdout[PIPE_WRITE_END]);
      if (fdout_ptr)
        *fdout_ptr = fdout[PIPE_READ_END];
      break;
    }

  return (childpid);
}

int
main(int argc, char** argv, char** envp)
{
  int wstatus;
  int fdout;
  ssize_t outsz;
  char outbuf[PIPE_BUF];
  
  pid_t childpid = popen_dup(&fdout, envp, 2, "/bin/ls", "-l");

  FILE* fpout = (FILE*)(fdout == -1 ? NULL : fdopen(fdout, "r")); \

  if ((outsz = read(fileno(fpout), outbuf, PIPE_BUF-1)) == -1)
    {
      perror("read stdout fils");
      exit(1);
    }
  else
    fprintf(stdout, "read %ld bytes: '%s'\n", outsz, outbuf);
  
  waitpid(childpid, &wstatus, 0);
}
