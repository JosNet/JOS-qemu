#include <inc/stdio.h>
#include <inc/error.h>
#include <inc/string.h>

#define BUFLEN 1024
static char buf[BUFLEN];

char *
readline(const char *prompt)
{
	int i, c, echoing;

#if JOS_KERNEL
	if (prompt != NULL)
		cprintf("%s", prompt);
#else
	if (prompt != NULL)
		fprintf(1, "%s", prompt);
#endif

	i = 0;
	echoing = iscons(0);
	while (1) {
		c = getchar();
		if (c < 0)
    {
			if (c != -E_EOF)
				cprintf("read error: %e\n", c);
			return NULL;
		}
    //backspace
    else if ((c == '\b' || c == '\x7f') && i > 0)
    {
			if (echoing)
      {
				cputchar('\b');
        buf[--i]=0;
        cprintf("\33[2K\r");
        cprintf("%s%s", prompt, buf);
      }
		}
    else if (c >= ' ' && i < BUFLEN-1)
    {
			buf[i++] = c;
      if (buf[i-2]=='[' && buf[i-1]=='A')
      {
        memmove(buf, &buf[i-2], 2);
        buf[2]=0;
        cprintf("\33[2K\r");
        return buf;
      }
			if (echoing)
      {
				cputchar(c);
      }
		}
    else if (c == '\n' || c == '\r')
    {
			if (echoing)
				cputchar('\n');
			buf[i] = 0;
			return buf;
		}
	}
}

