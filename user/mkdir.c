#include <inc/lib.h>

void
umain(int argc, char** argv)
{
  if (argc<2)
  {
    printf("use it right, stupid\n");
    return;
  }
  int fd;
  fd=open(argv[1], O_MKDIR|O_EXCL|O_CREAT|O_TRUNC);
	struct Stat st;
  stat(argv[1], &st);
  if (fd<0 || !st.st_isdir)
  {
    printf("that probably didn't work... %e\n");
    exit();
  }
  sync();
  return;
}
