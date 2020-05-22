#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/wait.h>

#define BSIZE 256

#define BASH_EXEC  "/bin/bash"
#define FIND_EXEC  "/usr/bin/find"
#define XARGS_EXEC "/usr/bin/xargs"
#define GREP_EXEC  "/bin/grep"
#define SORT_EXEC  "/usr/bin/sort"
#define HEAD_EXEC  "/usr/bin/head"

int main(int argc, char *argv[])
{
  int status;
  pid_t pid_1, pid_2, pid_3, pid_4;

  if (argc != 4) {
    printf("usage: finder DIR STR NUM_FILES\n");
    exit(0);
  }

    int p1[2];
  	int p2[2];
  	int p3[2];

  	pipe(p1);
  	pipe(p2);
  	pipe(p3);


      pid_1 = fork();
      if (pid_1 == 0)
      {
      /* First Child */
          char cmdbuf[BSIZE];
          bzero(cmdbuf, BSIZE);
          sprintf(cmdbuf, "%s %s -name \'*\'.[ch]", FIND_EXEC, argv[1]);
          dup2(p1[1], STDOUT_FILENO);
          char *myArgs[] = {BASH_EXEC, "-c", cmdbuf, (char*) 0};
          if((execv(BASH_EXEC, myArgs))< 0)
          {
              fprintf(stderr, "\nError execing find. ERROR#%d\n", errno);
              return (EXIT_FAILURE);
          }
          exit(0);
      }

      pid_2 = fork();
      if (pid_2 == 0) {
      /* Second Child */
      char cmdbuf[BSIZE];
      bzero(cmdbuf, BSIZE);
      sprintf(cmdbuf, "%s %s -c %s", XARGS_EXEC, GREP_EXEC, argv[2]);
      dup2(p1[0], STDIN_FILENO);
      dup2(p2[1], STDOUT_FILENO);
      close(p1[1]);
      char *myArgs[] = {BASH_EXEC, "-c", cmdbuf, (char*) 0};
          if((execv(BASH_EXEC, myArgs))< 0)
          {
              fprintf(stderr, "\nError execing find. ERROR#%d\n", errno);
              return (EXIT_FAILURE);
          }
          exit(0);
      }

      pid_3 = fork();
      if (pid_3 == 0) {
      /* Third Child */
      char cmdbuf[BSIZE];
      bzero(cmdbuf, BSIZE);
      sprintf(cmdbuf, "%s -t : +1.0 -2.0 --numeric --reverse", SORT_EXEC);
      dup2(p2[0], STDIN_FILENO);
      dup2(p3[1], STDOUT_FILENO);
      close(p1[1]);
      close(p2[1]);
      char *myArgs[] = {BASH_EXEC, "-c", cmdbuf, (char*) 0};
      if((execv(BASH_EXEC, myArgs))< 0)
      {
          fprintf(stderr, "\nError execing find. ERROR#%d\n", errno);
          return EXIT_FAILURE;
      }

          exit(0);
      }

      pid_4 = fork();
      if (pid_4 == 0) {
      /* Fourth Child */
        char cmdbuf[BSIZE];
        bzero(cmdbuf, BSIZE);
        sprintf(cmdbuf, "%s --lines=%s", HEAD_EXEC, argv[3]);
        dup2(p3[0], STDIN_FILENO);
        close(p1[1]);
        close(p2[1]);
        close(p3[1]);
        char *myArgs[] = {BASH_EXEC, "-c", cmdbuf, (char*) 0};
        if((execv(BASH_EXEC, myArgs))< 0)
        {
            fprintf(stderr, "\nError execing find. ERROR#%d\n", errno);
            return EXIT_FAILURE;
        }
        exit(0);
      }

      close(p1[1]);
      close(p2[1]);
      close(p3[1]);

      if ((waitpid(pid_1, &status, 0)) == -1) {
          fprintf(stderr, "Process 1 encountered an error. ERROR%d", errno);
          return EXIT_FAILURE;
      }
      if ((waitpid(pid_2, &status, 0)) == -1) {
          fprintf(stderr, "Process 2 encountered an error. ERROR%d", errno);
          return EXIT_FAILURE;
      }
      if ((waitpid(pid_3, &status, 0)) == -1) {
          fprintf(stderr, "Process 3 encountered an error. ERROR%d", errno);
          return EXIT_FAILURE;
      }
      if ((waitpid(pid_4, &status, 0)) == -1) {
          fprintf(stderr, "Process 4 encountered an error. ERROR%d", errno);
          return EXIT_FAILURE;
      }

    return 0;
  }
