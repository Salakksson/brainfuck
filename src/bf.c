#include "stdio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

int fdprintf(int fd, const char* format, ...) 
{
    va_list args;
    size_t buflen = strlen(format) + 15;
    char* buffer = alloca(buflen);    

    va_start(args, format);
    int len = vsnprintf(buffer, buflen, format, args);
    va_end(args);

    if (len < 0)
        return printf("Failed to format:\n%s\n", format);

    if (write(fd, buffer, len) != len)
        return printf("Failed to write to file: %s", strerror(errno));

    return -1;
}

int main(int argc, char** argv)
{
	if (argc < 3)
		return printf("Syntax: %s <source> <output>\n", argv[0]);
    
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1)
        return printf("Failed to read file '%s': %s\n", argv[1], strerror(errno));
    struct stat st;
    if (fstat(fd, &st) < 0)
        return printf("Failed to stat file '%s': %s\n", argv[1], strerror(errno));

    char* input = malloc(st.st_size + 1);
    read(fd, input, st.st_size);
    close(fd);

    struct label 
    {
        size_t labelno;
        struct label* prev;
    }* labels;
    
    fd = creat("/tmp/_bfc.asm", S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    fdprintf(fd, 
             "section .bss\n"
             "buffer resb 0x10000\n\n"
             "section .text\n"
             "global _start\n\n"
             "_start:\n"
             "xor esi, esi\n");
    size_t labelno = 0;
    void* old;
    char ch;
    for(size_t i = 0; (ch = input[i]); i++) switch(ch)
    {
    case '>':
        fdprintf(fd, "inc si\n");
        break;
    case '<':
        fdprintf(fd, "dec si\n");
        break;
    case '+':
        fdprintf(fd, "inc byte [buffer + esi]\n");
        break;
    case '-':
        fdprintf(fd, "dec byte [buffer + esi]\n");
        break;
    case ',':
        fdprintf(fd, 
                 "mov eax, 3\n"
                 "mov ebx, 0\n"
                 "lea ecx, [buffer + esi]\n"
                 "mov edx, 1\n"
                 "int 0x80\n");
        break;
    case '.':
        fdprintf(fd, 
                 "mov eax, 4\n"
                 "mov ebx, 1\n"
                 "lea ecx, [buffer + esi]\n"
                 "mov edx, 1\n"
                 "int 0x80\n");
        break;
    case '[':
        fdprintf(fd, "_l%d:\n", labelno);
        old = labels;
        labels = malloc(sizeof(*labels));
        labels->prev = old;
        labels->labelno = labelno++;
        break;
    case ']':
        fdprintf(fd, "cmp byte [buffer + esi], 0\n" 
                     "jne _l%d\n", labels->labelno);
        old = labels;
        labels = labels->prev;
        free(old);
        break;
    }
    fdprintf(fd, 
            "mov eax, 1\n"
            "mov ebx, [buffer + esi]\n"
            "int 0x80\n");

    char command[260];    

    int len = snprintf(command, 260, "nasm -f elf32 -o /tmp/_bfc.o /tmp/_bfc.asm; "
                                    "ld -m elf_i386 -s -o %s /tmp/_bfc.o; "
                                    "rm /tmp/_bfc.asm /tmp/_bfc.o", argv[2]);
    if (len == 260 || len < 1) 
        return printf("Likely failed to create assembler command using snprintf, exiting to be safe\n");
    system(command);
}
