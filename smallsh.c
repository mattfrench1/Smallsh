/*
References:
asprintf(): https://c-for-dummies.com/blog/?p=3934
            https://stackoverflow.com/questions/12746885/why-use-asprintf-instead-of-sprintf

getenv(3): https://man7.org/linux/man-pages/man3/getenv.3.html
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

struct sigaction sh_sigs = {0}, ignore_action = {0};


void handle_SIGINT(int signo){
  sigint_flag = 1;
}


int main(int argc, char *argv[])
{
  //pid_t spawnpid = -5;
  //int childStatus;

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

    background_flag = 0;
    redirection_flag = 0;

    if (feof(input)) {
      exit(0);
    }

//prompt:;
    /* TODO: Manage background processes */

    int waitpid_res = waitpid(0, &childStatus, WNOHANG | WUNTRACED);
    //printf("WAITPID_RES: %d\n", waitpid_res);

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
     
    //pid_t spawnpid = -5;
    //spawnpid = fork();

    //if (feof(input) == 0) {   //exit with status 0 once end of file is reached
    //  exit(0);
   // }

    /* TODO: prompt */
prompt:;
    sigint_flag = 0;
    if (input == stdin) {  //Interactive
      char *ps1 = getenv("PS1");
      fprintf(stderr, "%s", ps1);

      
      sh_sigs.sa_handler = handle_SIGINT;
      sigfillset(&sh_sigs.sa_mask);
      sh_sigs.sa_flags = 0;
      
      sigaction(SIGINT, &sh_sigs, NULL);
      ignore_action.sa_handler = SIG_IGN;
      sigaction(SIGTSTP, &ignore_action, NULL);

      
      ssize_t line_len = getline(&line, &n, input);
      clearerr(input);
      errno = 0;

    //fprintf(stderr,"SIGINT_FLAG: %d\n", sigint_flag);
    
      if (sigint_flag != 0 && line_len < 0){
        fprintf(stderr, "\n");
        goto prompt;
      }

      
      //ignore_action.sa_handler = SIG_IGN;

      //sigaction(SIGTSTP, &ignore_action, NULL); //ignore
      sigaction(SIGINT, &ignore_action, NULL);
      goto linecheck;
     
    
      
      

    }

  
    //sigint_flag = 0;
    //sh_sigs.sa_handler = handle_SIGINT;
    //sh_sigs.sa_handler = handle_SIGINT;
    //sigfillset(&sh_sigs.sa_mask);
    //sh_sigs.sa_flags = 0;
    //sigaction(SIGINT, &sh_sigs, NULL);



    
    
    ssize_t line_len = getline(&line, &n, input);  //n = buffer or size
    
    //fprintf(stderr,"LINE LEN: %lu\n", line_len);

    //ignore_action.sa_handler = SIG_IGN;
    //sigaction(SIGINT, &ignore_action, NULL);
    
    //reset clearerr/erro
    //clearerr(input);
    //errno = 0;
    //fprintf(stderr,"SIGINT_FLAG: %d\n", sigint_flag);
    
    //if (sigint_flag != 0 && line_len < 0){
     // goto prompt;
   // }

    //printf("LINE_LEN: %lu\n", line_len);

    //printf("EOF INPUT: %d\n", feof(input));
    //printf("EOF STDIN: %d\n", feof(stdin));

linecheck:;
    if (line_len < 0)  {
      //err(1, "%s", input_fn);
                          //hand eof here?
      break;
    }
    
    
    //printf("INPUT: %s\n", line);
    
   
    //fork();

    

    
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

    


    //for (size_t i = 0; i < nwords; i++) {
    //  printf("VAL: %s\n", words[i]);
   // }
  
  

    if (nwords > 0){
   
      
      //char *first_word = words[0];
      
      //if (first_word[0] == '_') {
      //  char replaced_word[strlen(first_word)];
      //  int j = 0;
      //  for (int i = 1; i < strlen(first_word); i++){
      //    replaced_word[j] = first_word[i];
      //    j ++;
      //  }
      //  free(words[0]);
      //  words[0] = replaced_word;
        //printf("NEW WORD: %s\n", words[0]);
     // }

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
             
  

              exit(int_val);
            }

        } else {
          errx(1, "too many arguments");
        }
       
      



      }else if (strcmp(words[0], "cd") == 0) {
        if (nwords == 1) {
          char *home_env = getenv("HOME");
          //printf("HOME: %s\n", home_env);
          //char cwd[1024];
          //getcwd(cwd, sizeof(cwd));
          //printf("WE ARE HERE: %s\n", cwd);
          chdir(home_env);

          //char nwd[1024];
          //getcwd(nwd, sizeof(nwd));
          //printf("WE ARE NOW HERE: %s\n", nwd);

        }else if (nwords == 2) {
          //system("ls");
          build_str(NULL, NULL);
          //char *new_env = getenv(words[1]);
          //printf("NEW ENV: %s\n", new_env);
          
          //following lines currenntly work
          //char cwd[1024];
          //getcwd(cwd, sizeof(cwd));
          //build_str(cwd, NULL);
          //build_str("/", NULL);  //dont need
          //char *new_env = build_str(words[1], NULL);
          
          //printf("ENV: %s\n", new_env);
        
          //printf("WE ARE HERE: %s\n", cwd);
          
          char *new_env = words[1];
          int chdir_result = chdir(new_env);

          if (chdir_result != 0) {
            errx(1, "invalid directory");
          }

          //char nwd[1024];
          //getcwd(nwd, sizeof(nwd));
          //printf("WE ARE NOW HERE: %s\n", nwd);

        }else {
          errx(1, "too many arguments");
        }
      }else{
        //printf("FORKING!\n");
        spawnpid = fork();
        
        switch (spawnpid) {
          case -1:{
            errx(1, "fork failed");
            break;
                  }

          case 0: {
            //char *first_word = words[0];
      
            

            //for (size_t i = 0; i < nwords; i++) {
            //  printf("WORDS: %s\n", words[i]);
           // }

            /*
            build_str(NULL, NULL);
            build_str("/bin/", NULL);
            char *exec_cmd = build_str(words[0], NULL);

            if (strcmp(words[0], "echo") == 0) {
            //char *exec_cmd = build_str(words[0], NULL);
            //char *newargv[] = {exec_cmd, NULL};
            char *newargv[nwords+1];
            newargv[0] = exec_cmd;
            newargv[nwords] = NULL;

            for (size_t i = 1; i <= nwords; i++) {
              newargv[i] = words[i];
            }
            exec_err = execv(newargv[0], newargv);
            //perror("execv");
            //exit(2);
            


            }else{
            char *newargv[] = {exec_cmd, NULL};
            exec_err = execv(newargv[0], newargv);
            //perror("execv");
            //exit(2);
            


            }
            
            if (exec_err != 0) {
            for (size_t i = 0; i < nwords; ++i) {
              char *exp_word = expand(words[i]);
              free(words[i]);
              words[i] = exp_word;
              fprintf(stderr, "%s\n", words[i]);
            }
            }
            */
           
            //Reset signals
            sigaction(SIGINT, &sh_sigs, NULL);
            sigaction(SIGTSTP , &sh_sigs, NULL);

            
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

              }else if (strcmp(words[i], ">") == 0) {
                if (i+1 < nwords) {
                  redirection_symbols[j] = words[i];
                  redirection_files[j] = words[i+1];
                  words[i] = NULL;
                  words[i+1] = NULL;
                  j ++;
                  redirection_flag = 1;
                }
              }else if (strcmp(words[i], ">>") == 0) {
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
            //printf("REDIRECTIOM ARR SIZE: %d\n", redirection_arr_size);
            //for (int i = 0; i < redirection_arr_size; i++) {
            //  printf("REDIRECTION VAL: %s // REDIRECTION FILE: %s\n", redirection_symbols[i], redirection_files[i]); 
            //}



            if (redirection_flag != 0) {   //use redirection commands
            for (int i = 0; i < redirection_arr_size; i++) {
              if (strcmp(redirection_symbols[i], "<") == 0){
                
                int new_fd = open(redirection_files[i], O_RDONLY);   //Only want to open for reading
                
                if (new_fd == -1) {
                  perror("cannot open file");
                  exit(1);
                }
                
                int dup_res = dup2(new_fd, 0);
                if (dup_res == -1){
                  perror("new file dupe()");
                  exit(2);
                }
             
                close(new_fd);
          
              }else if (strcmp(redirection_symbols[i], ">") == 0) {
                int new_fd = open(redirection_files[i], O_WRONLY | O_CREAT | O_TRUNC, 0777);
                
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

              }else if (strcmp(redirection_symbols[i], ">>") == 0) {
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
           
            
            execvp(words[0], words);
            perror("execvp");
            exit(2);

            }


            char *newargv[nwords+1];
            for (size_t i = 0; i < nwords; i++) {
              newargv[i] = words[i];
            }
            newargv[nwords] = NULL;

           // for (size_t i = 0; i < nwords+1; i++) {
           //   printf("ARR: %s\n", newargv[i]);
           // }

            //if (strcmp(newargv[0], "_exit") == 0){
            //  int int_val = atoi(newargv[1]);
            //  if (int_val == 0 && strcmp(newargv[1], "0") != 0) {
            //    errx(1, "given exit status is not an int value");
            //  }
              //status = int_val;

              //for (size_t i = 0; i < nwords; i++) {
              //char *exp_word = expand(words[i]);  //Update status in words
              //free(words[i]);
              //words[i] = exp_word;
              //}
              
           //   exit(status);
          //  }
  

            execvp(newargv[0], newargv);   //Success here means child exited normally
            perror("execvp");
            exit(2);

            break;
                  }
           default: {
            if (background_flag == 0) {
            spawnpid = waitpid(spawnpid, &childStatus, WUNTRACED);    //only wait if background process flag not set
            
            if (WIFSTOPPED(childStatus)) {
              fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);

              kill(spawnpid, SIGCONT);
            }


             if (WIFEXITED(childStatus)) {
              //printf("BACKGROUND PROCESS: %d\n", background_process);

              status = WEXITSTATUS(childStatus);
             }

            if (WIFSIGNALED(childStatus)){
          
              status = WTERMSIG(childStatus) + 128;    //Only use if signaled
              }



            }else{

       
            
            //background_process = spawnpid;
            if (WIFSTOPPED(childStatus)) {
              fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);

              kill(spawnpid, SIGCONT);
            }

            if (WIFSIGNALED(childStatus)){
          
              status = WTERMSIG(childStatus) + 128;    //Only use if signaled
              }


            }
            background_process = spawnpid;

            //printf("MADE IT TO HERE");

            //if (WIFSTOPPED(childStatus)) {
            //  fprintf(stderr, "Child process %d stopped. Continuing.\n", spawnpid);
           // }

            //if (WIFSIGNALED(childStatus)) {
              //printf("WIFSIGNALED");
           // }
           
           // if (WIFEXITED(childStatus)) {
              //printf("BACKGROUND PROCESS: %d\n", background_process);

             // status = WEXITSTATUS(childStatus);
              //background_process = spawnpid;
              //printf("BACKGROUND PROCESS: %d\n", background_process);
              //printf("USING EXPANSION: %s\n", expand("$!"));

             // for (size_t i = 0; i < nwords; i ++) {
              //  if (words[i] != NULL){
              //    char *exp_word = expand(words[i]);  //replace words with expanded words
              //    free(words[i]);
              //    words[i] = exp_word;
            //    }
             // }

              //for (size_t i = 0; i < nwords; i++) {
                //printf("VAL: %s\n", words[i]);
              //}
              
              //printf("MADE IT TO HERE");
            //}   //Removed an else here


            //if (WIFSIGNALED(childStatus)){
              //printf("MADE IT TO HERE");
              //status = WTERMSIG(childStatus) + 128;    //Only use if signaled
              //}
              //background_process = spawnpid;

              //for (size_t i = 0; i < nwords; i ++) {
                //if (words[i] != NULL){
                  //char *exp_word = expand(words[i]);  //replace words with expanded words
                  //free(words[i]);
                  //words[i] = exp_word;
                //}
             
              //for (size_t i = 0; i < nwords; i++) {
              //  printf("VAL: %s\n", words[i]);
              //}
  

              //}
              //printf("BACKGROUND PROCESS: %d\n", background_process);
              //printf("WTERMSIG: %d\n",WTERMSIG(childStatus));
            
            
            //printf("CHILD STATUS: %d\n", childStatus);
            //

           // for (size_t i = 0; i < nwords; i++) {   //clean out words
           //   free(words[i]);
           //   words[i] = NULL;
           // }
            
            
            break;
                    }


           for (size_t i = 0; i < nwords; i++) {   //clean out words
              free(words[i]);
              words[i] = NULL;
            }
           

        }





    
//    for (size_t i = 0; i < nwords; ++i) {
      //if (strcmp(words[i], "exit") == 0 && i == 0) {
      //  exit(0);
      //}

      //else if (strcmp(words[i], "cd") == 0 && i == 0) {
       // if (nwords == 1) {  //if no arg with cd, then cd home
        //   char *home_env = getenv("HOME");
        //   printf("HOME: %s\n", home_env);
         //  char cwd[1024];
          // getcwd(cwd, sizeof(cwd));
           //printf("WE ARE HERE: %s\n", cwd);
          // chdir(home_env);

          // char nwd[1024];
          // getcwd(nwd, sizeof(nwd));
          // printf("WE ARE NOW HERE: %s\n", nwd);
       // }else if (nwords == 2) {
         // printf("SECOND: %s\n", words[1]);
          
       // }else{
        //  errx(1, "too many arguments");        
       // }
     // } else {
      //  fork();
     // }
      //fprintf(stderr, "Word %zu: %s\n", i, words[i]);
//      char *exp_word = expand(words[i]);
//      free(words[i]);
//      words[i] = exp_word;
      //fprintf(stderr, "Expanded Word %zu: %s\n", i, words[i]);
//      fprintf(stderr, "%s\n", words[i]);
//    }

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
      }else{
      
      asprintf(&background_proc_str, "%d", background_process);
      build_str(background_proc_str, NULL);
      free(background_proc_str);
      }
     
    
    }else if (c == '$'){
      size_t pid = getpid();
      char *pid_str = NULL;
      asprintf(&pid_str, "%li", pid);
      //build_str("<PID: ", NULL); 
      build_str(pid_str, NULL);
      free(pid_str);  //need to free pointer 
      //build_str(">", NULL);

    }else if (c == '?'){
      //build_str("<STATUS: ", NULL);
      char *status_str = NULL;
      //asprintf(&status_str, "%d", status);
      asprintf(&status_str, "%d", status);
      build_str(status_str, NULL);
      free(status_str);
      //build_str(">", NULL);

    }else if (c == '{') {
      //build_str(start+2, NULL);
      char *var_name = build_str(start+2, end-1);  //Grab variable name
      build_str(NULL, NULL);   //Rest build_str
      char *env = getenv(var_name);
      
      //build_str("<Parameter: ", NULL);
      build_str(env, NULL);
      //build_str(">", NULL);
    }
    pos = end;
    c = param_scan(pos, &start, &end);
    build_str(pos, start);
  }
  return build_str(start, NULL);
}
