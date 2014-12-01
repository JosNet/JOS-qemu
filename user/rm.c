
#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	binaryname = "rm";
	if (argc < 2)
  {
    printf("do it right asshole\n");
  }
	else
  {
    remove(argv[1]);
  }
}
