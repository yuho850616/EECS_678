/**
* @file execute.c
*
* @brief Implements interface functions between Quash and the environment and
* functions that interpret an execute commands.
*
* @note As you add things to this file you may want to change the method signature
*/

#include "execute.h"

#include <stdio.h>

#include "quash.h"

#include <sys/wait.h>

#include <stdlib.h>

#include <unistd.h>

#include "deque.h"

#include <sys/types.h>

#define READ 0
#define WRITE 1
int run = true;
static int m_pipes[2][2];

IMPLEMENT_DEQUE_STRUCT (pid_deque, pid_t);
IMPLEMENT_DEQUE (pid_deque, pid_t);
pid_deque pobject;

typedef struct Job {
int job_id;
char* cmd;
pid_deque pobject;
}Job;

IMPLEMENT_DEQUE_STRUCT (job_deque, struct Job);
IMPLEMENT_DEQUE (job_deque, struct Job);
job_deque jobject;
int num = 1;

bool init = 0;
static int m_pipes[2][2];

/***************************************************************************
* Interface Functions
***************************************************************************/

// Return a string containing the current working directory.
char* get_current_directory(bool* should_free) {
  //Get CWD Mallocs space for the return value
  // Change this to true if necessary
  char* cwd = malloc(sizeof(char)*1024);
  *should_free = true;
  return getcwd(cwd, 1024);
}

// Returns the value of an environment variable env_var
const char* lookup_env(const char* env_var) {
  // Lookup environment variables. This is required for parser to be able
  // to interpret variables from the command line and display the prompt
  // correctly
  return getenv (env_var);
}

// Check the status of background jobs
  void check_jobs_bg_status() {
    // int queuelength = length_job_deque(&jobject);
    // for (int i = 0; i < queuelength; i++) {
    //   Job cur = pop_front_job_deque (&jobject);
    //   int pidlength = length_pid_deque (&cur.pobject);
	  //   //pid_t front = peek_front_pid_deque (&cur.pobject);
    //   for (int j = 0; j < pidlength; j++) {
    //     pid_t curr_pid = pop_front_pid_deque (&cur.pobject);
		//     int status;
    //     if (waitpid (curr_pid, &status, WNOHANG)==curr_pid) {
    //       print_job_bg_complete (cur.job_id, curr_pid, cur.cmd);
    //     }
    //     else if(waitpid (curr_pid, &status, WNOHANG)==0) {
    //       push_back_pid_deque (&pobject, curr_pid);
    //     }
    //     else{
    //       perror("Fail");
    //     }
    //   }
    //   if(!is_empty_pid_deque(&cur.pobject)){
    //     push_back_job_deque (&jobject, cur);
    //   }
    // }
    int queuelength = length_job_deque(&jobject);
    for(int i=0; i<queuelength;i++)
    {
      Job cur = pop_front_job_deque (&jobject);
      pid_t top = peek_front_pid_deque (&cur.pobject);
      int pidlength = length_pid_deque (&cur.pobject);
      for(int j=0; j<pidlength;j++)
      {
        pid_t curpid = pop_front_pid_deque(&cur.pobject);
        int status;
        if(waitpid(curpid,&status,WNOHANG)==0)
        {
          push_back_pid_deque(&cur.pobject, curpid);
        }
      }
    if(is_empty_pid_deque(&cur.pobject))
    {
      print_job_bg_complete(cur.job_id, top, cur.cmd);
    }
    else{
      push_back_job_deque(&jobject, cur);
    }
  }
}

// Prints the job id number, the process id of the first process belonging to
// the Job, and the command string associated with this job
void print_job(int job_id, pid_t pid, const char* cmd) {
  printf("[%d]\t%8d\t%s\n", job_id, pid, cmd);
  fflush(stdout);
}

// Prints a start up message for background processes
void print_job_bg_start(int job_id, pid_t pid, const char* cmd) {
  printf("Background job started: ");
  print_job(job_id, pid, cmd);
}

// Prints a completion message followed by the print job
void print_job_bg_complete(int job_id, pid_t pid, const char* cmd) {
  printf("Completed: \t");
  print_job(job_id, pid, cmd);
}

/***************************************************************************
* Functions to process commands
***************************************************************************/

// Run a program reachable by the path environment variable, relative path, or
// absolute path
void run_generic(GenericCommand cmd) {
  char* exec = cmd.args[0];
  char** args = cmd.args;
  execvp (exec,args);
  perror ("ERROR: Failed to execute program");
}

// Print strings
void run_echo(EchoCommand cmd) {
  char** str = cmd.args;
  for( int i = 0; NULL != str[i]; i++ ){
    printf("%s ", str[i]);
  }
  // Flush the buffer before returning
  printf("\n");
  fflush(stdout);
}

// Sets an environment variable
void run_export(ExportCommand cmd) {
  const char* env_var = cmd.env_var;
  const char* val = cmd.val;
  setenv (env_var, val, 1);
}

// Changes the current working directory
void run_cd(CDCommand cmd) {
  const char* dir = cmd.dir;
  char* old_dir;
  char* new_dir;
  // Check if the directory is valid
  if (dir == NULL) {
    perror("ERROR: Failed to resolve path");
    return;
  }
  old_dir = getcwd(NULL, 512);
  chdir (dir);
  new_dir = getcwd(NULL, 512);
  setenv("OLDPWD", old_dir, 1);
  setenv("PWD", new_dir, 1);
  free(new_dir);
  free(old_dir);
}

// Sends a signal to all processes contained in a job
void run_kill(KillCommand cmd) {
int signal = cmd.sig;
int job_id = cmd.job;
int queuelength = length_job_deque (&jobject);
struct Job temp;
// find job
for (int i = 0; i < queuelength; i++) {
  temp = pop_front_job_deque (&jobject);
  if (temp.job_id == job_id) {
		// kill all processes
		pid_deque curr_pobject = temp.pobject;
    int pidlength = length_pid_deque (&curr_pobject);
		while (pidlength!= 0) {
	    pid_t curr_pid = pop_front_pid_deque (&curr_pobject);
	    kill (curr_pid, signal);
      pidlength = length_pid_deque (&curr_pobject);
	   }
    }
  push_back_job_deque (&jobject, temp);
  }
// int signal = cmd.sig;
// int job_id = cmd.job;
//
// int queuelength = length_job_deque (&jobject);
// Job temp;
// // find job
// for (int i = 0; i < queuelength; i++) {
//   temp = pop_front_job_deque (&jobject);
//   if (temp.job_id == job_id) {
//     pid_t kill_pid;
// 		while (!is_empty_pid_deque(&temp.pobject)) {
// 	    kill_pid = pop_front_pid_deque (&temp.pobject);
// 	    kill (kill_pid, signal);
// 	   }
//         print_job_bg_complete(temp.job_id, kill_pid, temp.cmd);
//     }
//   else{
//     push_back_pid_deque(&pobject,kill_pid);
//     }
//   }
}

// Prints the current working directory to stdout
void run_pwd() {
  bool should_free;
  char* str = get_current_directory(&should_free);
  printf("%s\n", str);
  if(should_free){
    free(str);
  }
  // Flush the buffer before returning
  fflush(stdout);
}

// Prints all background jobs currently in the job list to stdout
void run_jobs() {
  int queuelength = length_job_deque (&jobject);
  for (int j = 0; j < queuelength; j++){
    struct Job cur = pop_front_job_deque (&jobject);
    pid_t process = peek_front_pid_deque(&cur.pobject);
  	print_job (cur.job_id, process, cur.cmd);
  	push_back_job_deque (&jobject, cur);
  }
  // Flush the buffer before returning
  fflush(stdout);
}

/***************************************************************************
* Functions for command resolution and process setup
***************************************************************************/

/**
* @brief A dispatch function to resolve the correct @a Command variant
* function for child processes.
*
* This version of the function is tailored to commands that should be run in
* the child process of a fork.
*
* @param cmd The Command to try to run
*
* @sa Command
*/
void child_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
    case GENERIC:
    run_generic(cmd.generic);
    break;

    case ECHO:
    run_echo(cmd.echo);
    break;

    case PWD:
    run_pwd();
    break;

    case JOBS:
    run_jobs();
    break;

    case EXPORT:
    case CD:
    case KILL:
    case EXIT:
    case EOC:
    break;

    default:
    fprintf(stderr, "Unknown command type: %d\n", type);
  }
}


/**
* @brief A dispatch function to resolve the correct @a Command variant
* function for the quash process.
*
* This version of the function is tailored to commands that should be run in
* the parent process (quash).
*
* @param cmd The Command to try to run
*
* @sa Command
*/
void parent_run_command(Command cmd) {
  CommandType type = get_command_type(cmd);

  switch (type) {
    case EXPORT:
     run_export(cmd.export);
     break;

    case CD:
     run_cd(cmd.cd);
     break;

    case KILL:
     run_kill(cmd.kill);
     break;

    case GENERIC:
    case ECHO:
    case PWD:
    case JOBS:
    case EXIT:
    case EOC:
     break;

    default:
     fprintf(stderr, "Unknown command type: %d\n", type);
  }
}

/**
* @brief Creates one new process centered around the @a Command in the @a
* CommandHolder setting up redirects and pipes where needed
*
* @note Processes are not the same as jobs. A single job can have multiple
* processes running under it. This function creates a process that is part of a
* larger job.
*
* @note Not all commands should be run in the child process. A few need to
* change the quash process in some way
*
* @param holder The CommandHolder to try to run
*
* @sa Command CommandHolder
*/
void create_process(CommandHolder holder, int i) {
// Read the flags field from the parser
bool p_in  = holder.flags & PIPE_IN;
bool p_out = holder.flags & PIPE_OUT;
bool r_in  = holder.flags & REDIRECT_IN;
bool r_out = holder.flags & REDIRECT_OUT;
bool r_app = holder.flags & REDIRECT_APPEND;

int write_end = i % 2;
	int read_end = (i - 1) % 2;

  if (p_out)
	{
		pipe (m_pipes[write_end]);
	}
  pid_t pid = fork();

  push_back_pid_deque(&pobject, pid);
  if (pid == 0) {

		if (p_in)
		{
			dup2 (m_pipes[read_end][READ], STDIN_FILENO);
			close (m_pipes[read_end][READ]);
		}
		if (p_out)
		{
			dup2 (m_pipes[write_end][WRITE], STDOUT_FILENO);
			close (m_pipes[write_end][WRITE]);
		}
	  if (r_in)
	  {
		  FILE* f = fopen (holder.redirect_in, "r");
		  dup2 (fileno (f), STDIN_FILENO);
	  }
	  if (r_out)
	  {
		  if (r_app)
		  {
			  FILE* f = fopen (holder.redirect_out, "a");
			  dup2 (fileno (f), STDOUT_FILENO);
		  }
		  else
		  {
			  FILE* f = fopen (holder.redirect_out, "w");
			  dup2 (fileno (f), STDOUT_FILENO);
		  }
	  }

    child_run_command (holder.cmd);
    exit(0);
  }
  else
    if (p_out) {
      close (m_pipes[write_end][WRITE]);
    }

    parent_run_command(holder.cmd);
 }

// Run a list of commands
void run_script(CommandHolder* holders) {

if (run){
	jobject = new_job_deque (1);
  run = false;
}
pobject = new_pid_deque(1);
if (holders == NULL)
 return;

check_jobs_bg_status();

if (get_command_holder_type(holders[0]) == EXIT &&
   get_command_holder_type(holders[1]) == EOC) {
 end_main_loop();
 return;
}

CommandType type;


// Run all commands in the `holder` array
for (int i = 0; (type = get_command_holder_type(holders[i]) ) != EOC; ++i){
 create_process(holders[i], i );
}


  if (!(holders[0].flags & BACKGROUND)) {

   while (!is_empty_pid_deque (&pobject)) {
     int status;
     waitpid (pop_front_pid_deque (&pobject), &status, 0);
   }
   destroy_pid_deque (&pobject);
  }


  else {// background job
     struct Job cur;
     cur.job_id = num;
     num = num +1;
     cur.pobject = pobject;
     cur.cmd = get_command_string ();
     push_back_job_deque (&jobject, cur);
     print_job_bg_start (cur.job_id, peek_back_pid_deque (&pobject), cur.cmd);
   }
}
