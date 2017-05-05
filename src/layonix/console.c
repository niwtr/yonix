// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.
#include "yotypes.h"
#include "types.h"
#include "defs.h"
#include "param.h"
#include "param_yonix.h"
#include "traps.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define KEY_UP 0xE2
#define KEY_DN 0xE3

static void consputc(int);

static int panicked = 0;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do
  {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i]);
}
//PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...)
{
  int i, c;
  uint *argp;
  char *s;

  if (fmt == 0)
    panic("null fmt");

  argp = (uint *)(void *)(&fmt + 1);
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
  {
    if (c != '%')
    {
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch (c)
    {
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if ((s = (char *)*argp++) == 0)
        s = "(null)";
      for (; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }
}

void panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cprintf("cpu panic: ");
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

//PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *)P2V(0xb8000); // CGA memory

static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);

  if (c == '\n')
    pos += 80 - pos % 80;
  else if (c == BACKSPACE)
  {
    if (pos > 0)
      --pos;
  }
  else
    crt[pos++] = (c & 0xff) | 0x0700; // black on white

  if (pos < 0 || pos > 25 * 80)
    panic("pos under/overflow");

  if ((pos / 80) >= 24)
  { // Scroll up.
    memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
    pos -= 80;
    memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
  crt[pos] = ' ' | 0x0700;
}

void consputc(int c)
{
  if (panicked)
  {
    cli();
    for (;;)
      ;
  }

  if (c == BACKSPACE)
  {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  }
  else
    uartputc(c);
  cgaputc(c);
}

#define INPUT_BUF 128
struct
{
  char buf[INPUT_BUF];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} input;

#define CMD_BUF 30
struct
{
  char cmd[CMD_BUF][INPUT_BUF];
  uint n; // number of command
  uint c; // current display index
} cmd_histy;

#define C(x) ((x) - '@') // Control-x

int histy_flag = 0;
int pausing = 1;

void clear_line()
{
  while (input.e != input.w &&
         input.buf[(input.e - 1) % INPUT_BUF] != '\n')
  {
    input.e--;
    consputc(BACKSPACE);
  }
}

void put_histy()
{
  for (int i = 0; i < strlen(cmd_histy.cmd[(cmd_histy.c - 1) % CMD_BUF]); i++)
  {
    char c = cmd_histy.cmd[(cmd_histy.c - 1) % CMD_BUF][i];
    if (c == '\n')
      break;

    input.buf[input.e++ % INPUT_BUF] = c;
    consputc(c);
  }
}

void consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0, dokill = 0, dolistproc = 0;
  int q;
  while ((c = getc()) >= 0)
  {
    switch (c)
    {
    case C('P'): // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'): // Kill line.
      clear_line();
      break;
    case C('H'):
    case '\x7f': // Backspace
      if (input.e != input.w)
      {
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case C('C'): //kill
      dokill = 1;
      break;
    case C('L'): //list procs.
        dbg_lstprocs();
        int status=0, pid=0;
        //0 init
        //1 k ok
        //2 reading
        //3 finish
        while((q=getc())!=C('L'))
          {
            if(q==-1)continue;
            switch(q){
            case 'k':
              status = 1;
              break;
            case '\n':
              if(status==2)
                status = 3;
              break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
              if(status==2){
                pid=pid*10+q-'0';
                consputc(q);
              }
              break;
            default:
              break;
            }

          if(status == 1)
          {
            cprintf("kill: ");
            status=2;
          }
          if(status == 3)
          {

              cprintf("\nkilled: %d\n", pid);
              kill(pid);

              status=0;
          }
        }

      break;
    case C('R'):
      dbg_lstrdy();
      while((q=getc())!=C('R'));
      break;
    case C('S'):
      dbg_lstslp();
      while((q=getc())!=C('S'));
      break;

    case KEY_UP: // last command
      if (cmd_histy.c > 1)
      {
        clear_line();

        cmd_histy.c--;
        put_histy();
      }
      break;

    case KEY_DN: // next command
      if (cmd_histy.c < cmd_histy.n)
      {
        clear_line();

        cmd_histy.c++;
        put_histy();
      }
      break;

    case C('Z'):
      while((q=getc())!=C('Z'));
    default:
      if (c != 0 && input.e - input.r < INPUT_BUF)
      {
        c = (c == '\r') ? '\n' : c;

        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);
        if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF)
        {
          input.w = input.e;
          if (strlen(&input.buf[input.r]) > 1 && strncmp(cmd_histy.cmd[(cmd_histy.n - 1) % CMD_BUF], &input.buf[input.r], strlen(&input.buf[input.r]) - 1))
          {
            safestrcpy(cmd_histy.cmd[cmd_histy.n++ % CMD_BUF], &input.buf[input.r], strlen(&input.buf[input.r]));
          }
          cmd_histy.c = cmd_histy.n + 1;

          wakeup(&input.r);
        }
      }
      break;
    }
  }
  if (dokill)
  {
    if (proc && proc->p_pid > 2)
    {
      int pid = proc->p_pid;
      kill(pid);
      cprintf("killed: %d\n", pid);
    }
  }

  if (dolistproc)
  {
    dbg_lstprocs();
  }

  if (doprocdump)
  {
    dbg_procdump(); // now call procdump() wo. cons.lock held
  }
}

int consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  while (n > 0)
  {
    while (input.r == input.w)
    {
      if (proc->p_killed)
      {
        ilock(ip);
        return -1;
      }
      sleep(&input.r);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D'))
    { // EOF
      if (n < target)
      {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  for (i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  ilock(ip);

  return n;
}

void consoleinit(void)
{

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;

  picenable(IRQ_KBD);
  //ioapicenable(IRQ_KBD, 0);//TODO warning
}

void prtwelcome()
{
  cprintf("  ___    ___ ________  ________   ___     ___    ___ \n");
  cprintf(" |\\  \\  /  /|\\   __  \\|\\   ___  \\|\\  \\   |\\  \\  /  /|\n");
  cprintf(" \\ \\  \\/  / | \\  \\|\\  \\ \\  \\\\ \\  \\ \\  \\  \\ \\  \\/  / /\n");
  cprintf("  \\ \\    / / \\ \\  \\\\\\  \\ \\  \\\\ \\  \\ \\  \\  \\ \\    / / \n");
  cprintf("   \\/  /  /   \\ \\  \\\\\\  \\ \\  \\\\ \\  \\ \\  \\  /     \\/  \n");
  cprintf(" __/  / /      \\ \\_______\\ \\__\\\\ \\__\\ \\__\\/  /\\   \\  \n");
  cprintf("|\\___/ /        \\|_______|\\|__| \\|__|\\|__/__/ /\\ __\\ \n");
  cprintf("\\|___|/                                  |__|/ \\|__| \n");
  cprintf("Welcome to YONIX, a simple yet complete operating system.\n");
  cprintf("Author: Tianrui Niu, Han Liu, Lingxuan Li, \n        Zhongyuan Zhou, Linghuan Zhu, Zhenjie yu\n");


}
