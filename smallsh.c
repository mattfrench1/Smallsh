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

#ifndef MAX_WORDS
#define MAX_WORDS 512
#endif

char *words[MAX_WORDS];
size_t wordsplit(char const *line);
char * expand(char const *word);
char * build_str(char const *start, char const *end);

int status = 0;

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
//prompt:;
    /* TODO: Manage background processes */
     
    //pid_t spawnpid = -5;
    //spawnpid = fork();

    /* TODO: prompt */
    if (input == stdin) {  //Interactive
      printf("$");
    }
    ssize_t line_len = getline(&line, &n, input);  //n = buffer or size
    if (line_len < 0) err(1, "%s", input_fn);
    
    //printf("INPUT: %s\n", line);
    
   
    //fork(); 

    size_t nwords = wordsplit(line);
  
    if (nwords > 0){
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
          build_str(NULL, NULL);
          //char *new_env = getenv(words[1]);
          //printf("NEW ENV: %s\n", new_env);
          //char cwd[1024];
          //getcwd(cwd, sizeof(cwd));
         
          //build_str(cwd, NULL);

          //build_str("/", NULL);
         
          char *new_env = build_str(words[1], NULL);
        
          //printf("WE ARE HERE: %s\n", cwd);

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
        fork();

    
    for (size_t i = 0; i < nwords; ++i) {
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
      char *exp_word = expand(words[i]);
      free(words[i]);
      words[i] = exp_word;
      //fprintf(stderr, "Expanded Word %zu: %s\n", i, words[i]);
      fprintf(stderr, "%s\n", words[i]);
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
      build_str("<BGPID>", NULL);
    
    }else if (c == '$'){
      size_t pid = getpid();
      char *pid_str = NULL;
      asprintf(&pid_str, "%li", pid);
      //build_str("<PID: ", NULL); 
      build_str(pid_str, NULL);
      free(pid_str);  //need to free pointer 
      //build_str(">", NULL);

    }else if (c == '?'){
      build_str("<STATUS: ", NULL);
      char *status_str = NULL;
      asprintf(&status_str, "%d", status);
      build_str(status_str, NULL);
      free(status_str);
      build_str(">", NULL);

    }else if (c == '{') {
      char var_name[20];
      strcpy(var_name, start+2);
      var_name[strlen(var_name)-1] = '\0';  //Remove ending '}'
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
