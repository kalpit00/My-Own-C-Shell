
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <string.h>

/* 
 * Sets terminal into raw mode. 
 * This causes having the characters available
 * immediately instead of waiting for a newline. 
 * Also there is no automatic echo.
 */

struct termios saved_attributes;

void reset_input_mode(void)
{
  tcsetattr(0, TCSANOW, &saved_attributes);
}

void tty_raw_mode(void)
{
	struct termios tty_attr;

  if (!isatty(0)) {
    fprintf(stderr, "Not a terminal.\n");
    exit(EXIT_FAILURE);
  }
  tcgetattr(0, &saved_attributes);
  atexit(reset_input_mode);

	tcgetattr(0, &tty_attr);

	/* Set raw mode. */
	tty_attr.c_lflag &= (~(ICANON|ECHO));
	tty_attr.c_cc[VTIME] = 0;
	tty_attr.c_cc[VMIN] = 1;
     
	tcsetattr(0, TCSAFLUSH, &tty_attr);
}
