#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>

// setting up ctrl-q to quit
// use hexidecimal because C doesn't have binary literals
// this mirrors what the ctrl key does and sets the upper 3 bits to 0
#define CTRL_KEY(k) ((k) & 0x1f)




/* Declarations */
struct editorConfig {
  int term_rows;
  int term_cols;
  struct termios org_term;
};

struct editorConfig E;


// disable console echo
// this will make no input showup, like entering a password
// I'm refering to this as "raw" mode
void StartRawMode();

// this will exit raw mode
void EndRawMode();

// this kills the editor
void Yamete(const char *s);

// get keyboard input
char ReadKey();
void ProcessKey();

// screen drawing
void ScreenRefresh();
void DrawRows();
int WindowSize(int *rows, int *cols);

// cursor stuff
int CursorPosition(int *rows, int *cols);

void InitEditor();




/* Functions */

char ReadKey() {
  int n;
  char c;
  while ((n = read(STDIN_FILENO, &c, 1)) != 1) {
    if (n == -1 && errno != EAGAIN) {
      Yamete("read");
    }
  }
  return c;
}

void ProcessKey() {
  char c = ReadKey();
  switch (c) {
    case CTRL_KEY('q'):
      // clear the screen on exit
      write(STDOUT_FILENO, "\x1b[2J", 4);
      write(STDOUT_FILENO, "\x1b[H", 3);
      exit(0);
      break;
  }
  return;
}

void StartRawMode() {
  if (tcgetattr(STDIN_FILENO, &E.org_term) == -1) {
    Yamete("tcgetattr");
  }
  atexit(EndRawMode);

  struct termios raw_term = E.org_term;

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
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.org_term) == -1) {
    Yamete("tcsetattr");
  }
  return;
}

void Yamete(const char *s) {
  // clear the screen on exit
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);

  perror(s);
  exit(1);
  return;
}

void ScreenRefresh() {
  // this is writing 4 bytes to the terminal
  // the first is \x1b, the escape character
  // then the other three bytes are [2J
  // the stuff after the [ tells the terminal to do various things
  // J clears the screen, and the 2 means the entire screen
  // I'm using VT100 escape sequences here, they are supported most places
  // later it may change to ncurses
  write(STDOUT_FILENO, "\x1b[2J", 4);
  // put the cursor at the top left
  // H is cursor position
  write(STDOUT_FILENO, "\x1b[H", 3);

  DrawRows();
  write(STDOUT_FILENO, "\x1b[H", 3);
  return;
}

void DrawRows() {
  for (int i = 0; i < E.term_rows; ++i) {
    write(STDOUT_FILENO, ">\r\n", 3);
  }
  return;
}

int WindowSize(int *rows, int *cols) {
  struct winsize ws;
  // ioctl will put the terminal rows and cols into ws, or return -1
  if (1 || ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    // but first a backup if ioctl doesn't work
    // the C command is cursor forward
    // the B command is cursor down
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) {
      return -1;
    }
    return CursorPosition(rows, cols);
  }
  else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

int CursorPosition(int *rows, int *cols) {
  // the n command is Device Status Report, 6 asks for cursor position
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;

  // setting up a timeout buffer for some systems
  // it also breaks on the 'R' character
  char buf[32];
  unsigned int i;
  for (i = 0; i < sizeof(buf) - 1; ++i) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}

void InitEditor() {
  if (WindowSize(&E.term_rows, &E.term_cols) == -1) {
    Yamete("WindowSize");
  }
}




int main() {
  // setup abby
  StartRawMode();
  InitEditor();

  // read every input as it comes in
  while (1) {
    ScreenRefresh();
    ProcessKey();
  }
  return 0;
}
