/*
* Author:     Clinton Lohr
* Date  :     02/04/2023
*/

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700


#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


void get_input(char *[]);
void parse_array(char *[]);

void execute_cmd(char *[]);
char *int_to_string(pid_t);
char *int_to_string2(int);
void check_bg_proc();

char *replace(char *restrict *restrict before, char const *restrict oldsub, char const *restrict newsub);
char *expansion(char* word);
char *exit_status[8];
char *rec_bg_pid[8];
int bg_flag;
char *save_file[256];
char *out_file_name[256];
char *in_file_name[256];
int word_count;
struct sigaction SIGINT_action = {0};
struct sigaction SIGTSTP_action = {0};
struct sigaction oldact_INT = {0};
struct sigaction oldact_STP = {0};

//Custom handler for SIGINT signal
void handle_SIGINT(int signo) {
}


int main(){

  char *expan_array[512];
  bg_flag = 0;

  SIGINT_action.sa_handler = SIG_IGN;
  sigfillset(&SIGINT_action.sa_mask);
  SIGINT_action.sa_flags = 0;
  sigaction(SIGINT, &SIGINT_action, &oldact_INT);

  SIGTSTP_action.sa_handler = SIG_IGN;
  sigfillset(&SIGTSTP_action.sa_mask);
  SIGTSTP_action.sa_flags = 0;
  sigaction(SIGTSTP, &SIGTSTP_action, &oldact_STP);

  while (1) {

    word_count = 0;

    // sets the default for exit_status
    if (!exit_status[0]) {
      exit_status[0] = "0";
    }
    
    // sets the defualt for most recent background pid
    if (!rec_bg_pid[0]) {
      rec_bg_pid[0] = "";
    }
    
    // Gets input from command prompt
    get_input(expan_array);

    // Parses words from expanded array
    parse_array(expan_array);
    
    // Executes built in or non-built in commands
    execute_cmd(expan_array);

 
    // frees output file for redirection
    if (out_file_name[0]) {
      free(out_file_name[0]);
      out_file_name[0] = NULL;
    }
   
    // frees input file for redirection
    if (in_file_name[0]) {
      free(in_file_name[0]);
      in_file_name[0] = NULL;;
    }
    
    // frees array holding split input words
    for (int i = 0; i < word_count; i++) {
      free(expan_array[i]);
      expan_array[i] = NULL;
    }

  }
  return 0;
}

/*
 * This function is responsible for executing built in commands and
 * using fork and exec to create new processes.
 * Child processes are responsible for redirecting input and output
 * files.
 */

void execute_cmd(char *expan_array[]) {

  pid_t spawnPid = -5;
  int child_status;
 
  if (expan_array[0]) {
    if (strcmp(expan_array[0], "exit") == 0) {
      if (!expan_array[1]) {
        const char *str;
        char *ptr;
        int ret_int;

        str = exit_status[0];
        ret_int = strtol(str, &ptr, 0);

        fprintf(stderr, "\nexit\n");
        kill(0, SIGINT);
        exit(ret_int);
      }

      else if (!expan_array[2]) {
        // add integer check to if below
        const char *str;
        char *ptr;
        int ret_int;

        str = expan_array[1];
        ret_int = strtol(str, &ptr, 0);

        fprintf(stderr, "\nexit\n");
        kill(0, SIGINT);
        exit(ret_int);
      } 

      else {
        fprintf(stderr, "Too many arguments for command 'exit'\n");
        goto exit;
      }
    }
  }
    
// Built in 'cd' shell command
  if (expan_array[0]) {
    if (strcmp(expan_array[0], "cd") == 0) {
      // checks for arguments for 'cd' redirects to 'HOME'
      // environment variable if True
      if (!expan_array[1]) {
        chdir(getenv("HOME"));
        goto exit;
      }

      // checks if more than one argument was entered for 'cd'
      // changes directory to specified file if False
      else if (!expan_array[2]) {
        chdir(expan_array[1]);
        goto exit;
      }

      // restarts get input process if too many agruments were entered
      else {
        fprintf(stderr, "Too many arguments for command 'cd'\n");
        goto exit;
      }
    }
  }

  // Calls the fork() function to create a child process
  spawnPid = fork();
  switch (spawnPid) {
    
    // Fork failed
    case -1:
      fprintf(stderr, "Fork function failed\n");
      exit(-1);
    
    // Child process created
    case 0:
      
      // resets signals to original dispositions
      sigaction(SIGINT, &oldact_INT, NULL);
      //SIGTSTP_action.sa_handler = SIG_DFL;
      sigaction(SIGTSTP, &oldact_STP, NULL);

      // checks if there was an input file for redirection
      if (in_file_name[0] && out_file_name[0]) {
          int source_fd;
          int target_fd;
          int result;

          // attempts to open file
          if((source_fd = open(in_file_name[0], O_RDONLY)) == -1) {
            fprintf(stderr, "Failed to open file\n");
            exit(-1); 
          } 

          // attempts to copy file descriptor
          if((result = dup2(source_fd, 0)) == -1) {
            fprintf(stderr, "Failed to duplicate file descriptor\n");
            exit(-1);
          }

          if ((target_fd = open(out_file_name[0], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
              fprintf(stderr, "Failed to open file\n");
              exit(-1);
          }

          if ((result = dup2(target_fd, 1)) == -1) {
            fprintf(stderr, "Failed to copy file descriptor\n");
            exit(-1);
          }

          //closes file
          close(source_fd);
          close(target_fd);
      }
       
      else if(in_file_name[0]) {
        int target_fd;
        int result;
        if((target_fd = open(in_file_name[0], O_RDONLY)) == -1) {
            fprintf(stderr, "Failed to open file\n");
            exit(-1);
        } 

        if((result = dup2(target_fd, 0)) == -1) {
            fprintf(stderr, "Failed to create a copy of file descripor\n");
            exit(-1);
          }

        close(target_fd);

        }

        // checks if there was an output file for redirection
        else if (out_file_name[0]) {
          int target_fd;
          int result;

          // attempts to open file for writing or 
          // creates a new file if it does not exits
          if ((target_fd = open(out_file_name[0], O_WRONLY | O_CREAT | O_TRUNC, 0777)) == -1) {
            fprintf(stderr, "Failed to open file\n");
            //exit_status[0] = "-1";
            exit(-1);
          }

          if ((result = dup2(target_fd, 1)) == -1) {
            fprintf(stderr, "Failed to copy file descriptor\n");
            //exit_status[0] = "-1";
            exit(-1);

          }
          
      }

      // calls execvp to execute a command
      if(execvp(expan_array[0], expan_array) == -1) {
        fprintf(stderr, "Failed to execute command buddy\n");
         exit(-1);
        }
       

    // Parent process runs in default
    default:

      // if background flag is not set
      if (bg_flag == 0) {

        //blocking wait
        spawnPid = waitpid(spawnPid, &child_status, WUNTRACED);

        // checks if child process exited normally
        if (WIFEXITED(child_status)){
          int ret_exit = WEXITSTATUS(child_status);
          exit_status[0] = int_to_string2(ret_exit);
        }

        // checks if child process exited via signal
        else if (WIFSIGNALED(child_status)){
          int ret_exit = WTERMSIG(child_status);
          ret_exit += 128;
          exit_status[0] = int_to_string2(ret_exit);
        }

        // checks for stopped child processes, continues them running if True
        else if (WIFSTOPPED(child_status)) {
          kill(0, SIGCONT);
          errno = 0;
          fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t)spawnPid);
          rec_bg_pid[0] = int_to_string(spawnPid);
          bg_flag = 1;
        }
        
        goto exit;

      }
      
      // Runs processes in background, blocks default wait block
      else {
        rec_bg_pid[0] = int_to_string(spawnPid);
        spawnPid = waitpid(spawnPid, &child_status, WNOHANG | WUNTRACED);
      }

      goto exit;

  }
exit:;

}

/*
 * This function takes the split words from the input string
 * and parses the tokens
*/
void parse_array(char *expan_array[]) {
  int i;
  int count = 0;
  char comment[] = "#";
  char b_ground[] = "&";
  char in_file_dir[] = "<";
  char out_file_dir[] = ">";
  int in_check = 0;
  int out_check = 0;
  int in_then_out = 0;
  int out_then_in = 0;
  
  // checks for comment token using strncmp
  for(i = 0; i < word_count -1; i++) {
    if(strncmp(expan_array[i], comment, 1) == 0) {
      count = i;
      while (expan_array[count]) {
        free(expan_array[count]);
        expan_array[count] = NULL;
        word_count --;
        count ++;
      }
      goto bg_check;
    }
  }
bg_check:
  
  // checks for background only token using strncmp
  if (strncmp(expan_array[word_count-2], b_ground, 1) == 0) {
    free(expan_array[word_count-2]);
    expan_array[word_count-2] = NULL;
    bg_flag = 1;
    word_count --;
  }

  // checks for the infile name
  // saves the file name to a global variable
  if(word_count > 4){
    if (strncmp(expan_array[word_count -3], in_file_dir, 1) == 0) {
      in_check = 1;
      if (word_count > 5) {
        if (strncmp(expan_array[word_count - 5], out_file_dir, 1) == 0) {
          in_then_out = 1;
          save_file[0] = expan_array[4];
        }
      }
    }
    
    // checks for the outfile name
    // saves the file name to a global variable
    else if (strncmp(expan_array[word_count - 3], out_file_dir, 1) == 0 ) {
      out_check = 1;
      if (word_count > 5) {
        if (strncmp(expan_array[word_count - 5], in_file_dir, 1) == 0) {
          out_then_in = 1;
          save_file[0] = expan_array[4];  
        }
      }
    }
  }

input:
  if (word_count > 3) {
    if(strncmp(expan_array[word_count - 3], in_file_dir, 1) == 0) {

      in_file_name[0] = strdup(expan_array[word_count - 2]);
      free(expan_array[word_count - 2]);
      expan_array[word_count - 2] = NULL;
      free(expan_array[word_count - 3]);
      expan_array[word_count - 3] = NULL;
      
      if (out_then_in) {
        goto exit;
      }

      if (!out_check) {
        goto exit;
      }

      else {
        word_count -=2;
      }
    }
  }

  // checks for the outfile descriptor
  if (word_count > 3) {
    if (strncmp(expan_array[word_count - 3], out_file_dir, 1) == 0) {

      out_file_name[0] = strdup(expan_array[word_count - 2]);
      free(expan_array[word_count - 2]);
      expan_array[word_count -2] = NULL;
      free(expan_array[word_count - 3]);
      expan_array[word_count - 3] = NULL;

      if (out_then_in){
        word_count -= 2;
        goto input;
      }

      if (in_then_out) {
        goto exit;
      }
    }
  }

exit:

  if (in_check) {
    word_count -= 2;
  }

  else if (out_check) {
    word_count -= 2;
  }


}

/*
 * This function gets the user input from stdin and
 * and splits the string into words
 * Calls expansion function to expand tokens
 */
void get_input(char *expan_array[]) {

  char *PS1;
  FILE *input = stdin;
  char *line = NULL;
  size_t len = 0;
  char *input_array[512];
  int count = 0;

  // sets command line
  if ((PS1 = getenv("PS1")) == NULL) {
    PS1 = "";
  }

input:

  SIGINT_action.sa_handler = handle_SIGINT;

  check_bg_proc();
  
  fprintf(stderr, "%s", PS1);

  for(;;) {  
    int line_len = getline(&line, &len, input);
    if (feof(input)) {
      const char *str;
      char *ptr;
      int ret_int;
      
      str = exit_status[0];
      ret_int = strtol(str, &ptr, 0);
      fprintf(stderr, "\nexit\n");
      kill(0,SIGINT);
      exit(ret_int);
    }

    if (line_len == -1) {
      fprintf(stderr, "\n");
      clearerr(input);
      errno = 0;
      goto input;
    }

    bg_flag = 0;

    SIGINT_action.sa_handler = SIG_IGN;
 
    // sets delimiters
    char *IFS; 
    if ((IFS = getenv("IFS")) == NULL) {
      IFS = "\t\n"; 
    }
    
    count = 0;
    char* token = strtok(line, IFS);
  
    // uses strtok to create tokens 
    while(token) {
      input_array[count] = strdup(token);
      count ++;
      word_count ++;
      token = strtok(NULL, IFS);
    } 

    free(line);
    input_array[count] = NULL;  
    count = 0;

    // calls expansion function to expand tokens
    // and stores them in new array
    while(input_array[count]) {
      char *ret = expansion(input_array[count]);
      expan_array[count] = strdup(ret);
      input_array[count] = NULL;
      free(input_array[count]);
      count ++; 
    }

    free(input_array[count]);
    expan_array[count] = NULL;
    word_count ++;
    count = 0;
    break;
    
  }
}

/*
 * Function Written for CS 344 course to use in smallsh program to expand and resize tokens
 */
char *replace(char *restrict *restrict before, char const *restrict oldsub, char const *restrict newsub) {
  char *str = *before;
  size_t before_len = strlen(str);
  size_t const oldsub_len = strlen(oldsub);
  size_t const newsub_len = strlen(newsub);

  for (; (str = strstr(str, oldsub));) {
    ptrdiff_t off = str - *before;
    if (newsub_len > oldsub_len) {
      str = realloc(*before, sizeof **before * (before_len + newsub_len - oldsub_len + 1));
      if (!str) goto exit;
      *before = str;
      str = *before + off;
    }
   
    memmove(str + newsub_len, str + oldsub_len, before_len + 1 - off - oldsub_len);
    memcpy(str, newsub, newsub_len);
    before_len = before_len + newsub_len - oldsub_len;
    str += newsub_len;
  }
  str = *before;
  if (newsub_len < oldsub_len) {
    str = realloc(*before, sizeof **before * (before_len + 1));
    if (!str) goto exit;
    *before = str;
  }
exit:
  return str;
}

/*
 * uses sprintf to call replace function
 */
char *expansion(char* word) {

  // ~/ Home Expansion 
  char *HOME = getenv("HOME");
  char *ret = replace(&word, "~", HOME);

  // $$ PID Expansion 
  char tmp_buffer[8];
  pid_t my_pid = getpid();
  sprintf(tmp_buffer, "%jd", (intmax_t)my_pid);
  ret = replace(&ret, "$$", &tmp_buffer[0]);

  // $? Exit Status Expansion
  ret = replace(&ret, "$?", exit_status[0]);

  // $! Recent Background Process PID
  ret = replace(&ret, "$!", rec_bg_pid[0]);

  return ret;
}

// converts a pid_t input to a string
char *int_to_string(pid_t pid) {

  char tmp_buffer[8];
  sprintf(tmp_buffer, "%jd", (intmax_t)pid);

  char *ret_str[3];
  ret_str[0] = strdup(&tmp_buffer[0]);

  return ret_str[0];
}

// converts an integer input to a string
char *int_to_string2(int int_in) {

  char tmp_buffer[8];
  sprintf(tmp_buffer, "%d", int_in);

  char *ret_str[3];
  ret_str[0] = strdup(&tmp_buffer[0]);

  return ret_str[0];  
}


 /*
  * This function is responsible for checking if any unwaited for
  * background processes exist.
 */
void check_bg_proc() {

  int child_status;
  pid_t spawnPid = 1;
  
  while (spawnPid > 0) {

    spawnPid = waitpid(0, &child_status, WNOHANG | WUNTRACED);
    if (spawnPid < 1) {
      continue;
    }

    // Exit
    if (WIFEXITED(child_status)){
      fprintf(stderr, "Child process %jd done. Exit status %d.\n", (intmax_t)spawnPid, WEXITSTATUS(child_status));
    }

    // Signaled
    else if (WIFSIGNALED(child_status)){
      fprintf(stderr, "Child process %jd done. Signaled %d.\n", (intmax_t) spawnPid, WTERMSIG(child_status));
      errno = 0;
    }

    // Stopped
    else if (WIFSTOPPED(child_status)) {
      kill(spawnPid, SIGCONT);
      errno = 0;
      fprintf(stderr, "Child process %jd stopped. Continuing.\n", (intmax_t)spawnPid);
    }
  }
}
