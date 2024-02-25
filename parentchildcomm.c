
#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>
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

  if (pipe2(fdout, O_NONBLOCK) == -1)
    {
      perror("pipe2");
      return -1;     
    }

  if ((childpid = fork()) == -1)
    {
      perror("fork");
      exit(1);
    }
  else
    switch(childpid)
      {
      case 0:
        close(fdout[PIPE_READ_END]);
        close(STDOUT_FILENO);
        if (dup(fdout[PIPE_WRITE_END]) == -1)
          {
            perror("dup write end fd of output pipe");
            return -1;
          }
        
        fprintf(stdout, "In child process (pid = %d)\n\n", getpid());
        fprintf(stdout, "===========Command launched=========== \n");
        
        if (execve(args[0], args, envp) == -1)
          {
            perror("execve child command");
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

  bzero(outbuf, PIPE_BUF);
  outsz = read(fdout, outbuf, PIPE_BUF-1);
  do
    {
      if (outsz == -1)
        {
          perror("read stdout fils in loop");
          exit(1);          
        }
      else if (outsz > 0)
        {
          fprintf(stdout, "read %ld bytes: '%s'\n", outsz, outbuf);      
        }
      else if (outsz == 0)
        break;
    }
  while((outsz = read(fdout, outbuf, PIPE_BUF-1)) > 0);
  if (outsz == -1)
    {
      perror("read stdout fils after loop");
      exit(1);          
    }
  
  waitpid(childpid, &wstatus, 0);
}
