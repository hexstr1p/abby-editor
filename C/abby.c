#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

struct termios org_term;


// disable console echo
// this will make no input showup, like entering a password
// I'm refering to this as "raw" mode
void StartRawMode();

// this will exit raw mode
void EndRawMode();


void StartRawMode() {
  tcgetattr(STDIN_FILENO, &org_term);
  atexit(EndRawMode);

  struct termios raw_term = org_term;


  // this is a bitflag
  // the ICANON lets it read input bit by bit instead of line by line
  raw_term.c_lflag &= ~(ECHO | ICANON);

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_term);
  return;
}

void EndRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_term);
  return;
}



int main() {
  // disabling echoing
  StartRawMode();

  // read every input as it comes in
  char c;
  while (read(STDIN_FILENO, &c, 1) == 1 && c != 'q') {
    // debugging purposes
    if (isprint(c)) printf("%d ('%c')\n", c, c);
    else printf("%d\n", c);
  }

  return 0;
}
