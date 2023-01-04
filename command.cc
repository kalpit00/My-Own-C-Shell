/*
 * CS252: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 * DO NOT PUT THIS PROJECT IN A PUBLIC REPOSITORY LIKE GIT. IF YOU WANT 
 * TO MAKE IT PUBLICALLY AVAILABLE YOU NEED TO REMOVE ANY SKELETON CODE 
 * AND REWRITE YOUR PROJECT SO IT IMPLEMENTS FUNCTIONALITY DIFFERENT THAN
 * WHAT IS SPECIFIED IN THE HANDOUT. WE OFTEN REUSE PART OF THE PROJECTS FROM  
 * SEMESTER TO SEMESTER AND PUTTING YOUR CODE IN A PUBLIC REPOSITORY
 * MAY FACILITATE ACADEMIC DISHONESTY.
 */

#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "command.hh"
#include "shell.hh"

Command::Command() {
  // Initialize a new vector of Simple Commands
  _simpleCommands = std::vector<SimpleCommand *>();

  _outFile = NULL;
  _inFile = NULL;
  _errFile = NULL;
  _background = false;
  _append = false;
  count = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ) {
  // add the simple command to the vector
  _simpleCommands.push_back(simpleCommand);
}

void Command::clear() {
  // deallocate all the simple commands in the command vector
  for (auto simpleCommand : _simpleCommands) {
    delete simpleCommand;
  }

  // remove all references to the simple commands we've deallocated
  // (basically just sets the size to 0)
  _simpleCommands.clear();

  if (_outFile == _errFile) {
    delete _outFile;
    _outFile = NULL;
    _errFile = NULL;
  }

  if ( _outFile ) {
    delete _outFile;
  }
  _outFile = NULL;

  if ( _inFile ) {
    delete _inFile;
  }
  _inFile = NULL;

  if ( _errFile ) {
    delete _errFile;
  }
  _errFile = NULL;

  _background = false;
  _append = false;
  count = 0;
}

void Command::print() {
  printf("\n\n");
  printf("              COMMAND TABLE                \n");
  printf("\n");
  printf("  #   Simple Commands\n");
  printf("  --- ----------------------------------------------------------\n");

  int i = 0;
  // iterate over the simple commands and print them nicely
  for ( auto & simpleCommand : _simpleCommands ) {
    printf("  %-3d ", i++ );
    simpleCommand->print();
  }

  printf( "\n\n" );
  printf( "  Output       Input        Error        Background\n" );
  printf( "  ------------ ------------ ------------ ------------\n" );
  printf( "  %-12s %-12s %-12s %-12s\n",
      _outFile?_outFile->c_str():"default",
      _inFile?_inFile->c_str():"default",
      _errFile?_errFile->c_str():"default",
      _background?"YES":"NO");
  printf( "\n\n" );
}

void Command::execute() {
  // Don't do anything if there are no simple commands
  if ( _simpleCommands.size() == 0 ) {
    Shell::prompt();
    return;
  }

  if (count > 1) {
    printf("Ambiguous output redirect.\n");
    clear();
    Shell::prompt();
    return;
  }
  // Print contents of Command data structure
  // print();

  if (!strcmp(_simpleCommands[0] -> _arguments[0] -> c_str(), "exit")) {
    printf("Good Bye!!\n");
    _Exit(1);
  }

  // Add execution here
  // For every simple command fork a new process
  // Setup i/o redirection
  // and call exec

  int tmpin = dup(0);
  int tmpout = dup(1);
  int tmperr = dup(2);

  int fdin;
  int fdout;
  int fderr;

  if (_inFile) {
    fdin = open(_inFile -> c_str(), O_RDONLY);
  }
  else {
    fdin = dup(tmpin);
  }

  int ret;
  int numSimpleCommands = _simpleCommands.size();

  for (int i = 0; i < numSimpleCommands; i++) {

    dup2(fdin, 0);
    close(fdin);

    if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "setenv")) {
      if (setenv(_simpleCommands[i]->_arguments[1]->c_str(), _simpleCommands[i]->_arguments[2]->c_str(), 1)) {
        perror("setenv");
      }
      clear();
      Shell::prompt();
      return;
    }

    if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "unsetenv")) {
      if (unsetenv(_simpleCommands[i]->_arguments[1]->c_str())) {
        perror("unsetenv");
      }
      clear();
      Shell::prompt();
      return;
    }

    if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "cd")) {
      if (_simpleCommands[i]->_arguments.size() == 1) {
        if (chdir(getenv("HOME"))) perror("cd");
      }
      else {
        const char * path = _simpleCommands[i]->_arguments[1]->c_str();
        char message[1024] = "cd: can't cd to ";
        strcat(message, path);
        if (chdir(path)) {
          perror(message);
        }
      }
      clear();
      Shell::prompt();
      return;
    }

/*    if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "source")) {
      // Read source from FILE

      std::string cmd;
      std::ifstream fd;

      fd.open(_simpleCommands[i]->_arguments[1]->c_str());

      std::getline(fd, cmd);
      fd.close();

      // save in/out
      int tmpin=dup(0);
      int tmpout=dup(1);

      // input command
      int fdpipein[2];
      pipe(fdpipein);
      write(fdpipein[1], cmd.c_str(), strlen(cmd.c_str()));
      write(fdpipein[1], "\n", 1);

      close(fdpipein[1]);

      int fdpipeout[2];
      pipe(fdpipeout);

      dup2(fdpipein[0], 0);
      close(fdpipein[0]);
      dup2(fdpipeout[1], 1);
      close(fdpipeout[1]);

      int ret_source = fork();
      if (ret_source == 0) {
        execvp("/proc/self/exe", NULL);
        _exit(1);
      } else if (ret < 0) {
        perror("fork");
        exit(1);
      }

      dup2(tmpin, 0);
      dup2(tmpout, 1);
      close(tmpin);
      close(tmpout);	

      char ch;
      char * buffer = new char[i];
      int r = 0;

      // read output 
      while (read(fdpipeout[0], &ch, 1)) {
        if (ch != '\n')  buffer[r++] = ch;
      }

      buffer[r] = '\0';
      printf("%s\n",buffer);

      fflush(stdout);

      clear();
      Shell::prompt();
      return;
    }

*/

    if (i == numSimpleCommands - 1) { // last command
      if (_outFile) {
        if (_append) {
          fdout = open(_outFile -> c_str(), O_WRONLY | O_APPEND | O_CREAT, 0664);
        }
        else {
          fdout = open(_outFile -> c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
        }
      }
      else {
        fdout = dup(tmpout);
      }
    }
    else {
      int fdpipe[2];
      pipe(fdpipe);
      fdout = fdpipe[1];
      fdin = fdpipe[0];
    }

    dup2(fdout, 1);
    close(fdout);

    if (_errFile) {
      if (_append) {
        fderr = open(_errFile -> c_str(), O_WRONLY | O_APPEND | O_CREAT, 0664);
      }
      else {
        fderr = open(_errFile -> c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0664);
      }
    }
    else {
      fderr = dup(tmperr);
    }
    dup2(fderr, 2);
    close(fderr);

    int n = _simpleCommands[i]->_arguments.size();
    char *c = strdup(_simpleCommands[i]->_arguments[n-1]->c_str());
    setenv("_", c, 1);

    ret = fork();

    size_t numArgs = _simpleCommands[i]->_arguments.size();
    char ** _args = new char*[numArgs+1];
    size_t j = 0;
    for (j = 0; j < numArgs; j++) {
      _args[j] = strdup(_simpleCommands[i]->_arguments[j]->c_str());
    }
    _args[j] = NULL;

    if (ret == 0) { // child

      if (!strcmp(_simpleCommands[i]->_arguments[0]->c_str(), "printenv")) {
        char ** p = environ;

        while(*p != NULL){
          printf("%s\n", *p);
          p++;
        }
        fflush(stdout);
        _exit(1);
      }

      execvp(_simpleCommands[i]->_arguments[0]->c_str(), _args);
      perror("execvp");
      _exit(1);
    }
    else if (ret < 0) {
      perror("fork");
      return;
    }
  }

  dup2(tmpin, 0);
  dup2(tmpout, 1);
  dup2(tmperr, 2);
  close(tmpin);
  close(tmpout);
  close(tmperr);

  if (!_background) {
    //waitpid(ret, NULL, 0);

    int status;
        waitpid(ret, &status, 0);
        std::string s = std::to_string(WEXITSTATUS(status));
        setenv("?", s.c_str(), 1);
        char * pError = getenv("ON_ERROR");
        if (pError != NULL && WEXITSTATUS(status) != 0) printf("%s\n", pError);
    } else {
        std::string s = std::to_string(ret);
        setenv("!", s.c_str(), 1);
        Shell::_lastPIDs.push_back(ret);
  }
  // Clear to prepare for next command
  clear();

  // Print new prompt
    Shell::prompt();
}

SimpleCommand * Command::_currentSimpleCommand;
