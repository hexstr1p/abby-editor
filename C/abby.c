#include <ctype.h>
#include <stdio.h>
#include <errno.h>
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

// this kills the editor
void Yamete(const char *s);


void StartRawMode() {
  if (tcgetattr(STDIN_FILENO, &org_term) == -1) {
    Yamete("tcgetattr");
  }
  atexit(EndRawMode);

  struct termios raw_term = org_term;


  // these are bitflags
  // first some old miscellaneous flags for old terminal emulators
  // BRKINT is break checking, INPCK is parity checking, ISTRIP is 8th bit strip
  raw_term.c_iflag &= ~(BRKINT | INPCK | ISTRIP);
  // CS8 is a bitmask
  raw_term.c_cflag |= (CS8);
  // the IXON flag stops manual software flow control from the old days
  // I stops input flag, CR stops carriage return, and NL stops new line
  raw_term.c_iflag &= ~(ICRNL | IXON);
  // this one turns off output processing
  raw_term.c_oflag &= ~(OPOST);
  // the ICANON lets it read input bit by bit instead of line by line
  // the ISIG flag stops the ctrl-z and ctrl-y signals in the terminal
  // the IEXTEN flag stops ctrl-o and ctrl-v
  raw_term.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);

  // I'm going to have read() timeout if it doesn't get input after a while
  raw_term.c_cc[VMIN] = 0;
  raw_term.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_term) == -1) {
    Yamete("tcsetattr");
  }
  return;
}

void EndRawMode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &org_term) == -1) {
    Yamete("tcsetattr");
  }
  return;
}

void Yamete(const char *s) {
  perror(s);
  exit(1);
}


int main() {
  // disabling echoing
  StartRawMode();

  // read every input as it comes in
  while (1) {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) == -1 && errno != EAGAIN) {
      Yamete("read");
    }
    if (isprint(c)) printf("%d ('%c')\r\n", c, c);
    else printf("%d\r\n", c);
    if (c == 'q') break;
  }

  return 0;
}
