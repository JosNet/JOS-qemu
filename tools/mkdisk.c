
#include <stdio.h>
#include <stdlib.h>

const char* inf  = "obj/fs/fs.img";
const char* outf = "obj/fs/inmem_fs.S";
const char* header =
    "# Assembly copy of the filesystem image so that\n"
    "# we can make a ramfs with nice pointers\n"
    "\n"
    ".section .inmemfs\n"
    ".globl _inmem_fs_begin, _inmem_fs_end\n"
    "_inmem_fs_begin:\n"
    "\n"
    "\n.byte ";

const char* footer =
    "0x0\n"
    "_inmem_fs_end:\n"
    ".long 0x0\n"
    "# We're done!\n"
    "\n";

int main(int argc, char ** argv) {
    FILE * fin = fopen(inf, "r");
    FILE * fout = fopen(outf, "w+");
    int n;

    fprintf(fout, "%s", header);
    while(1)
    {
        unsigned int c = fgetc(fin);
        if(feof(fin))
            break;

        if(++n % 8 == 0)
            fprintf(fout, "0x%02x \n.byte ", c);
        else
            fprintf(fout, "0x%02x,", c);
    }
    fprintf(fout, "%s", footer);

    fclose(fin);
    fclose(fout);

    return 0;
}
