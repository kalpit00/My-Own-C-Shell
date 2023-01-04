#include <cstdio>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <limits.h>
#include "shell.hh"
#include "y.tab.hh"

#ifndef YY_BUF_SIZE
#ifdef __ia64__
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

int yyparse(void);
void yyrestart(FILE * file);
void yypush_buffer_state(YY_BUFFER_STATE buffer);
void yypop_buffer_state();
YY_BUFFER_STATE yy_create_buffer(FILE * file, int size);

void Shell::prompt() {
  if ( isatty(0) & !_srcCmd ) {
  char* pPrompt = getenv("PROMPT");
  if (pPrompt != NULL) printf("%s", pPrompt);
    else printf("myshell>");
  fflush(stdout);
  }
}

/*extern "C" void sig_handler_ctrlC ( int sig ) {
  if (sig == SIGINT) {
    printf("\nEXITING PROGRAM\n");
    exit(EXIT_SUCCESS);
  }
}*/

extern "C" void sigIntHandler ( int sig ) {
/*
  pid_t pid = wait3(0, 0, NULL);
  while (waitpid(-1, NULL, WNOHANG) > 0) {
    printf("\n[%d] exited.", pid);
  }
  */
  if (sig == SIGINT) {
  Shell::_currentCommand.clear();
  printf("\n");
  Shell::prompt();
  }

  if (sig == SIGCHLD) {
  pid_t pid = waitpid(-1, NULL, WNOHANG);
    for (unsigned i=0; i<Shell::_lastPIDs.size(); i++) {
      if (pid == Shell::_lastPIDs[i]) {
        printf("[%d] exited\n", pid);
        Shell::_lastPIDs.erase(Shell::_lastPIDs.begin()+i);
        break;
      }
    }
  }
}

void source(void) {
  std::string s = ".shellrc";
  FILE * in = fopen(s.c_str(), "r");

  if (!in) {
    return;
  }

  yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE));
  Shell::_srcCmd = true;
  yyparse();
  yypop_buffer_state();
  fclose(in);
  Shell::_srcCmd = false;
}

int main(int argc, char** argv) {

 /* struct sigaction sig_int_action;
  sig_int_action.sa_handler = sig_handler_ctrlC;
  sigemptyset(&sig_int_action.sa_mask);
  sig_int_action.sa_flags = SA_RESTART;

  if (sigaction(SIGINT, &sig_int_action, NULL)) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }

  struct sigaction sig_int_action2;
  sig_int_action2.sa_handler = sig_handler_zombie;
  sigemptyset(&sig_int_action2.sa_mask);
  sig_int_action2.sa_flags = SA_RESTART;

  if (sigaction(SIGCHLD, &sig_int_action2, NULL)) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }*/

  struct sigaction sa;
  sa.sa_handler = sigIntHandler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;

  source();

  std::string s = std::to_string(getpid());
  setenv("$", s.c_str(), 1);
  
  char abs_path[256];
  realpath(argv[0], abs_path);
  setenv("SHELL", abs_path, 1);
/*
  FILE * fd = fopen(".shellrc", "r");
  if (fd) {
    yyrestart(fd);
    yyparse();
    yyrestart(stdin);
    fclose(fd);
  }
  else {
    if ( isatty(0) ) {
      Shell::prompt();
    }
  }*/

  Shell::_srcCmd = false;
  Shell::prompt();

  if (sigaction(SIGINT, &sa, NULL)) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  if (sigaction(SIGCHLD, &sa, NULL)) {
    perror("sigaction");
    exit(EXIT_FAILURE);
  }
  yyrestart(stdin);
  yyparse();
}

Command Shell::_currentCommand;
std::vector<int> Shell::_lastPIDs;
bool Shell::_srcCmd;
