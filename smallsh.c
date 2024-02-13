/*
References: (Everything I used for this assignment)

asprintf(): https://c-for-dummies.com/blog/?p=3934
            https://stackoverflow.com/questions/12746885/why-use-asprintf-instead-of-sprintf

Man Pages:
getenv(3): https://man7.org/linux/man-pages/man3/getenv.3.html
waitpid(3p): https://man7.org/linux/man-pages/man3/waitpid.3p.html
wait(3p): https://man7.org/linux/man-pages/man3/wait.3p.html
kill(2): https://man7.org/linux/man-pages/man2/kill.2.html
W* Macros: https://linux.die.net/man/2/waitpid
sigaction(2): https://man7.org/linux/man-pages/man2/sigaction.2.html
clearerr(3p): https://man7.org/linux/man-pages/man3/clearerr.3p.html
atoi(3): https://man7.org/linux/man-pages/man3/atoi.3.html
chdir(2): https://man7.org/linux/man-pages/man2/chdir.2.html
exec(3p): https://man7.org/linux/man-pages/man3/exec.3p.html
execvp(3): https://linux.die.net/man/3/execvp

CS-374 Modules:
Exploration: Process API – Creating and Terminating Processes
Exploration: Process API - Monitoring Child Processes
Exploration: Process API - Executing a New Program
Exploration: Signal Handling API
Exploration: Processes and I/O

Office Hours:
Jackson Bohrer
Ninad Anklesaria

 */


#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <err.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);
char * build_str(char const *start, char const *end);

int status = 0;
int exec_err = 0;
int childStatus;
pid_t spawnpid = -5;

int background_process = 0;
int background_flag;
int redirection_flag;
int redirection_arr_size;
int sigint_flag = 0;

char *redirection_symbols[MAX_WORDS];
char *redirection_files[MAX_WORDS];

struct sigaction sigint_sa = {0}, sigint_ignore_action = {0};
struct sigaction sigtstp_sa = {0}, sigtstp_ignore_action = {0};
struct sigaction sigint_restore;
struct sigaction sigtstp_restore;


/* Signal handler function, sets flag if used */
void handle_SIGINT(int signo){
  sigint_flag = 1;
}



int main(int argc, char *argv[])
{


  FILE *input = stdin;
  char *input_fn = "(stdin)";
  if (argc == 2) {
    input_fn = argv[1];
    input = fopen(input_fn, "re");
    if (!input) err(1, "%s", input_fn);
  } else if (argc > 2) {
    errx(1, "too many arguments");
  }

  char *line = NULL;
  size_t n = 0;
  for (;;) {

    
    background_flag = 0;  //reset flags
    redirection_flag = 0;

    if (feof(input)) {
      exit(0);
    }

    /* Manage background processes*/
    int waitpid_res = waitpid(0, &childStatus, WNOHANG | WUNTRACED);  //Have to use WNOHANG/WUNTRACED
   
    if (waitpid_res > 0) {
      if (WIFSTOPPED(childStatus)) {
        fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);
        kill(spawnpid, SIGCONT);
      }

      if (WIFSIGNALED(childStatus)) {
        status = WTERMSIG(childStatus);
        fprintf(stderr, "Child process %d done. Signaled %d.\n", spawnpid, status);
      }

      if (WIFEXITED(childStatus)) {
        status = WEXITSTATUS(childStatus);
        fprintf(stderr, "Child process %d done. Exit status %d.\n", spawnpid, status);
      }
    }
     

    /* Set sigaction struct's signal handler functions */
    sigint_sa.sa_handler = handle_SIGINT;
    sigtstp_sa.sa_handler = handle_SIGINT;
    
    /* Call sigaction() to set signals & use oldact to later reset signals*/
    sigaction(SIGINT, &sigint_sa, &sigint_restore);
    sigaction(SIGTSTP, &sigtstp_sa, &sigtstp_restore);

prompt:;
    if (feof(input)) {
      exit(0);
    }

    sigint_flag = 0;
    if (input == stdin) {  //Interactive
      char *ps1 = getenv("PS1");
      fprintf(stderr, "%s", ps1);

      /* Set ignore actions */
      sigint_ignore_action.sa_handler = SIG_IGN;
      sigtstp_ignore_action.sa_handler = SIG_IGN;
      
      sigaction(SIGTSTP, &sigtstp_ignore_action, NULL); //Always ignore SIGTSTP

      ssize_t line_len = getline(&line, &n, input);
      clearerr(input);
      errno = 0;

      if (line_len < 0 && sigint_flag == 0) {
        exit(0);  //EOF
      }

      /* Jump back to prompt if SIGINT is detected in input*/
      if (sigint_flag != 0 && line_len < 0){
        fprintf(stderr, "\n");
        goto prompt;
      }

      sigaction(SIGINT, &sigint_ignore_action, NULL);  //Now, always ignore SIGINT
      goto linecheck;
    }


    ssize_t line_len = getline(&line, &n, input);  //n = buffer or size

linecheck:;
    if (line_len < 0)  {
      break;
    }
    
    size_t nwords = wordsplit(line);
    
    if (words[nwords-1] != NULL) {
      if (strcmp(words[nwords-1], "&") == 0) {
        background_flag = 1;     //Set background flag
        words[nwords-1] = NULL;
        nwords --;
      }
    }

    for (size_t i = 0; i < nwords; i++) {
      if (words[i] != NULL) {
        char *exp_word = expand(words[i]);  //replace words with expanded words
        free(words[i]);
        words[i] = exp_word;
      }
    }

    if (nwords > 0){
      if (words[0] == NULL) {
        continue;
      }

      if (strcmp(words[0], "exit") == 0) {
        if (nwords == 1) {
          exit(status);

        } else if (nwords == 2) {
            int int_val = atoi(words[1]);
            if (int_val == 0 && strcmp(words[1], "0") != 0) {
              errx(1, "given exit status is not an int value");

            } else {
              exit(int_val);  //Exit with given status
            }

        } else {
          errx(1, "too many arguments");
        }
       
      } else if (strcmp(words[0], "cd") == 0) {
        if (nwords == 1) {
          char *home_env = getenv("HOME");  //No arguments defaults to home
          chdir(home_env);

        } else if (nwords == 2) {
          char *new_env = words[1];
          int chdir_result = chdir(new_env);

          if (chdir_result != 0) {
            errx(1, "invalid directory");
          }

        }else {
          errx(1, "too many arguments");
        }

      }else{
        /* Fork if command is not cd/exit */
        spawnpid = fork();
        
        switch (spawnpid) {
          case -1:{
            errx(1, "fork failed");
            break;
                  }

          case 0: {
          /* Child process */
           
            //Reset signals
            sigaction(SIGINT, &sigint_restore, NULL);
            sigaction(SIGTSTP, &sigtstp_restore, NULL);
           
            /* Add redirection commands to one array and corresponding files to another */
            int j = 0;
            for (size_t i = 0; i < nwords; i++) {
              if (words[i] != NULL) {
                if (strcmp(words[i], "<") == 0){
                  if (i+1 < nwords) {
                    redirection_symbols[j] = words[i];
                    redirection_files[j] = words[i+1];
                    words[i] = NULL;
                    words[i+1] = NULL;
                    j ++;
                    redirection_flag = 1;
                  }

                } else if (strcmp(words[i], ">") == 0) {
                    if (i+1 < nwords) {
                      redirection_symbols[j] = words[i];
                      redirection_files[j] = words[i+1];
                      words[i] = NULL;
                      words[i+1] = NULL;
                      j ++;
                      redirection_flag = 1;
                    }

                } else if (strcmp(words[i], ">>") == 0) {
                    if (i+1 < nwords) {
                      redirection_symbols[j] = words[i];
                      redirection_files[j] = words[i+1];
                      words[i] = NULL;
                      words[i+1] = NULL;
                      j ++;
                      redirection_flag = 1;
                    }
                }
            }
          }
          
            redirection_arr_size = j;
          
            if (redirection_flag != 0) {   //use redirection commands
              for (int i = 0; i < redirection_arr_size; i++) {
                if (strcmp(redirection_symbols[i], "<") == 0){               
                  int new_fd = open(redirection_files[i], O_RDONLY);   //Only want to open for reading
                  
                  if (new_fd == -1) {
                    perror("cannot open file");
                    exit(1);
                  }
                
                  int dup_res = dup2(new_fd, 0);   //Use dup2 to change stdin stream
                  if (dup_res == -1){
                    perror("new file dupe()");
                    exit(2);
                  }
             
                close(new_fd);
          
              } else if (strcmp(redirection_symbols[i], ">") == 0) {
                  int new_fd = open(redirection_files[i], O_WRONLY | O_CREAT | O_TRUNC, 0777);  //Open to write
                
                  if (new_fd == -1) {
                    perror("cannot open file");
                    exit(1);
                  }

                  int dup_res = dup2(new_fd, 1);  //Use dup2 to change stdout
                  if (dup_res == -1) {
                    perror("new file dupe()");
                    exit(2);
                  }

                  close(new_fd);

              } else if (strcmp(redirection_symbols[i], ">>") == 0) {
                  int new_fd = open(redirection_files[i], O_WRONLY | O_CREAT | O_APPEND, 0777);

                  if (new_fd == -1) {
                    perror("cannot open file");
                    exit(1);
                  }

                  int dup_res = dup2(new_fd, 1);
                  if (dup_res == -1) {
                    perror("new file dupe()");
                    exit(2);
                  }
                  close(new_fd);
              }
            }
           
            
              execvp(words[0], words);  //Use execvp on words array that was modified due to redirection 
              perror("execvp");
              exit(2);
            }


            /* Create new array with words array values that is null terminated */
            char *newargv[nwords+1];
            for (size_t i = 0; i < nwords; i++) {
              newargv[i] = words[i];
            }
            newargv[nwords] = NULL;
  
            execvp(newargv[0], newargv);   //Success here means child exited normally
            perror("execvp");
            exit(2);

            break;
                  }


           default: {
            /* Parent process here */
            if (background_flag == 0) {
              spawnpid = waitpid(spawnpid, &childStatus, WUNTRACED);    //only wait if background process flag not set
            
              if (WIFSTOPPED(childStatus)) {
                fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);
                kill(spawnpid, SIGCONT);
              }

             if (WIFEXITED(childStatus)) {
              status = WEXITSTATUS(childStatus);  //Update status if child exited properly
             }

             if (WIFSIGNALED(childStatus)){          
              status = WTERMSIG(childStatus) + 128;    //Only use if signaled
             }

            } else {

              if (WIFSTOPPED(childStatus)) {
                fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);
                kill(spawnpid, SIGCONT);
              }

              if (WIFSIGNALED(childStatus)){          
                status = WTERMSIG(childStatus) + 128;    //Only use if signaled
              }
            }

            background_process = spawnpid;  //Update background process to latest child process created           
            break;
                    }

           /* Clean out words array */
           for (size_t i = 0; i < nwords; i++) {  
              free(words[i]);
              words[i] = NULL;
           }
        }
    }
    }
  }
   
}


char *words[MAX_WORDS] = {0};


/* Splits a string into words delimited by whitespace. Recognizes
 * comments as '#' at the beginning of a word, and backslash escapes.
 *
 * Returns number of words parsed, and updates the words[] array
 * with pointers to the words, each as an allocated string.
 */
size_t wordsplit(char const *line) {
  size_t wlen = 0;
  size_t wind = 0;

  char const *c = line;
  for (;*c && isspace(*c); ++c); /* discard leading space */

  for (; *c;) {
    if (wind == MAX_WORDS) break;
    /* read a word */
    if (*c == '#') break;
    for (;*c && !isspace(*c); ++c) {
      if (*c == '\\') ++c;
      void *tmp = realloc(words[wind], sizeof **words * (wlen + 2));
      if (!tmp) err(1, "realloc");
      words[wind] = tmp;
      words[wind][wlen++] = *c; 
      words[wind][wlen] = '\0';
    }
    ++wind;
    wlen = 0;
    for (;*c && isspace(*c); ++c);
  }
  return wind;
}


/* Find next instance of a parameter within a word. Sets
 * start and end pointers to the start and end of the parameter
 * token.
 */
char
param_scan(char const *word, char const **start, char const **end)
{
  static char const *prev;
  if (!word) word = prev;
  
  char ret = 0;
  *start = 0;
  *end = 0;
  for (char const *s = word; *s && !ret; ++s) {
    s = strchr(s, '$');
    if (!s) break;
    switch (s[1]) {
    case '$':
    case '!':
    case '?':
      ret = s[1];
      *start = s;
      *end = s + 2;
      break;
    case '{':;
      char *e = strchr(s + 2, '}');
      if (e) {
        ret = s[1];
        *start = s;
        *end = e + 1;
      }
      break;
    }
  }
  prev = *end;
  return ret;
}

/* Simple string-builder function. Builds up a base
 * string by appending supplied strings/character ranges
 * to it.
 */
char *
build_str(char const *start, char const *end)
{
  static size_t base_len = 0;
  static char *base = 0;

  if (!start) {
    /* Reset; new base string, return old one */
    char *ret = base;
    base = NULL;
    base_len = 0;
    return ret;
  }
  /* Append [start, end) to base string 
   * If end is NULL, append whole start string to base string.
   * Returns a newly allocated string that the caller must free.
   */
  size_t n = end ? end - start : strlen(start);
  size_t newsize = sizeof *base *(base_len + n + 1);
  void *tmp = realloc(base, newsize);
  if (!tmp) err(1, "realloc");
  base = tmp;
  memcpy(base + base_len, start, n);
  base_len += n;
  base[base_len] = '\0';

  return base;
}

/* Expands all instances of $! $$ $? and ${param} in a string 
 * Returns a newly allocated string that the caller must free
 */
char *
expand(char const *word)
{
  char const *pos = word;
  char const *start, *end;
  char c = param_scan(pos, &start, &end);
  build_str(NULL, NULL);
  build_str(pos, start);

  while (c) {
    if (c == '!'){    
      char *background_proc_str = NULL;
      if (background_process == 0) {  //No background = empty string
        build_str("", NULL);

      } else {    
        asprintf(&background_proc_str, "%d", background_process);  //Use asprintf to convert int to str
        build_str(background_proc_str, NULL);
        free(background_proc_str);
      }
     
    } else if (c == '$') {
      size_t pid = getpid();
      char *pid_str = NULL;
      asprintf(&pid_str, "%li", pid);
      build_str(pid_str, NULL);
      free(pid_str);  //need to free pointer 

    } else if (c == '?') {
      char *status_str = NULL;
      asprintf(&status_str, "%d", status);
      build_str(status_str, NULL);
      free(status_str);

    } else if (c == '{') {
      char *old_val = build_str(NULL, NULL);  //Need to first save current value in build_str
      char *var_name = build_str(start+2, end-1);  //Grab variable name
      build_str(NULL, NULL); //Reset build_str
      build_str(old_val, NULL);
      char *env = getenv(var_name);
      if (env != NULL) {
        build_str(env, NULL);
      }
    
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}
