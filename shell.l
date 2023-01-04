
/*
 *
 * CS-252 
 * shell.l: lexical analyzer for shell
 * You have to extend it.
 *
 */

%x src

%{

#include <cstring>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "y.tab.hh"
#include "shell.hh"
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

static void yyunput (int c,char *buf_ptr  );

void myunputc(int c) {
  unput(c);
}

extern "C" char * read_line();

int mygetc(FILE * f) {
  static char *p;
  char ch;

  if (!isatty(0)) {
    return getc(f);
  }

  if (p==NULL || *p == 0) {
    char * s = read_line();
    p =s;
  }
  ch = *p;
  p++;

  return ch;
}

#undef getc
#define getc(f) mygetc(f)

%}

%option noyywrap

%%

\n {
  return NEWLINE;
}

[ \t] {
  /* Discard spaces and tabs */
}

">" {
  return GREAT;
}

"<" {
  return LESS;
}
">>" {
  return GREATGREAT;
}

"&" {
  return AMPERSAND;
}

"|" {
  return PIPE;
}

">&" {
  return GREATAMPERSAND;
}

">>&" {
  return GREATGREATAMPERSAND;
}

"2>" {
  return INTEGERGREAT;
}

source {
  BEGIN(src);
}

<src>[^ \t\n][^ \t\n]* {
  FILE * in = fopen(yytext, "r+");

  if (!in) {
    perror("fopen");
    BEGIN(INITIAL);
  } else {
    fputc('\n', in);
    yypush_buffer_state(yy_create_buffer(in, YY_BUF_SIZE));
    BEGIN(INITIAL);
    Shell::_srcCmd = true;
    yyparse();
    yypop_buffer_state();
    fclose(in);
    Shell::_srcCmd = false;
  }
}

~[^ \n\t]* {
  std::string str = std::string(yytext);
  std::string home = getenv("HOME");
  if (str.size() == 1) {
    yylval.cpp_string = new std::string(home);
  } 
  else {
    if (str[1] == '/') {
      str = home + str.substr(1, -1);
    } 
    else {
      str = std::string("/homes/") + str.substr(1,-1);
    }
    yylval.cpp_string = new std::string(str);
  }
  return WORD;
}

[^ \\\t\n]*(\\[^nt])((\\[^nt])|[^ \\\t\n])* {
  std::string str = std::string(yytext);
  std::size_t found = str.find('\\');
  while(found!=-1) {
    str = str.substr(0,found) + str.substr(found+1,std::string::npos);
    found = str.find('\\', found+1);
  }
  yylval.cpp_string = new std::string(str);
  return WORD;
}


$\{[^ \}\n]*\}([^ \n]*|$\{[^ \}\n]*\})* {
  std::string str = std::string(yytext);
  std::string result = "";
  std::size_t found = str.find('$');
  while(found!=-1) {
    std::string component;
    result += str.substr(0, found);
    if (str[found+1] == '{') {
      std::size_t end = str.find('}');
      if (end != -1) {
        component = str.substr(found+2, end-found-2);
        if (getenv(component.c_str())) result += getenv(component.c_str());
        else result += component;      
        str = str.substr(end+1,-1);
      }
    }
    found = str.find('$');
  }
  result += str;
  /*result = result + str.substr(2,str.size()-3);
  if (getenv(str.c_str()))
    yylval.cpp_string = new std::string(getenv(str.c_str()));
  else
    yylval.cpp_string = new std::string(str);*/
  yylval.cpp_string = new std::string(result);
  return WORD;
}

\$\([^\n\$]*\)|`[^\n`]*` {
  /* Generate subshell */
  int i=0;

  std::string * cmd = new std::string(yytext);
  cmd->pop_back();

  // string check
  if (strchr(cmd->c_str(), '`') != NULL) { // if `...`
    cmd = new std::string(yytext+1);
    cmd->pop_back();
  }
  else {
    cmd = new std::string(yytext+2);  // if $(...)
    cmd->pop_back();
  }

  // Setup i/op redirection in subshell
  // and call exec
  int tmpin = dup(0);
  int tmpout = dup(1);

  // Parent creates two pipes
  int fdpipein[2];
  int fdpipeout[2];

  pipe(fdpipein);
  pipe(fdpipeout);

  // write subshell command 
  write(fdpipein[1], cmd->c_str(), cmd->size());
  write(fdpipein[1], "\n", 1);
  write(fdpipein[1], "exit", 4);
  write(fdpipein[1], "\n", 1);
  close(fdpipein[1]);

  dup2(fdpipein[0], 0);
  close(fdpipein[0]);
  dup2(fdpipeout[1], 1);
  close(fdpipeout[1]);

  // In child redirect inout from pin[0] and redirect
  // output to pout[1]. Then execute shell from
  // /proc/self/exe
  int ret = fork();
  if (ret == 0) {
    execvp("/proc/self/exe", NULL);
    _exit(1);
  } else if (ret < 0) {
    perror("fork");
    exit(1);
  }

  // restore in/out defaults
  dup2(tmpin, 0);
  dup2(tmpout, 1);
  close(tmpin);
  close(tmpout);

  // read from fdpipeout
  char ch;
  size_t size = 0;
  i = 0;

  // The parent reads the ouput from pout[0] and
  // writes it into a buffer.
  char * buffer = new char[6666];
  while (read(fdpipeout[0], &ch, 1)) {
    if ( ch == '\n' ? buffer[i++] = ' ' : buffer[i++] = ch);
  }
  buffer[i] = '\0';

  // The parent puts the characters
  // in the buffer back into lex by calling myunoutc(in c)
  // in reverse order.
  i = strlen(buffer) - 1;
  for (i ; i >= 0; i--) {
    unput(buffer[i]);
  }
}


[^ \t\n|><&]*\\[^ \t\n]* {
  int i = 0;
  char * escChar;
  escChar = new char[strlen(yytext)*2];
  char * args = yytext;
  while (*args) {
    if (*args == '\\') {
      escChar[i] = *(++args);
      i++;
    }
    else {
      escChar[i] = *args;
      i++;
    }
    args++;
  }
  escChar[i] = '\0';
  yylval.cpp_string = new std::string(escChar);
  return WORD;
}

["][^\n\"]*["] {
  int i = 0;
  char * quoteChar = new char[strlen(yytext)];
  char *args = yytext;
  while (*args) {
    if (*args == '\"') {
      quoteChar[i] = *(++args);
      i++;
    }
    else {
      quoteChar[i] = *args;
      i++;
    }
    args++;
  }
  quoteChar[i] = '\0';
  yylval.cpp_string = new std::string(quoteChar);
  return WORD;
}

[^ \t\n|><][^ \t\n|><]* {
  /* Assume that file names have only alpha chars */
  yylval.cpp_string = new std::string(yytext);
  return WORD;
}
