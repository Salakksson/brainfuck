#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef TMPFILE 
#define TMPFILE "/tmp/_bfc"
#endif

int main(int argc, char** argv)
{
	if (argc < 3)
		return printf("Syntax: %s <source> <output>\n", argv[0]);
    
    FILE* fp = fopen(argv[1], "r");
    if (!fp)
        return printf("Failed to read file '%s': %s\n", argv[1], strerror(errno));

    fseek(fp, 0, SEEK_END);
    size_t sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* input = malloc(sz + 1);
    fread(input, 1, sz, fp);
    fclose(fp);

    struct label 
    {
        size_t labelno;
        struct label* prev;
    }* labels;
    
    fp = fopen(TMPFILE".asm", "w+");
    if (!fp)
        return printf("Failed to create temporary file "TMPFILE".asm: %s\n", strerror(errno));
    
    fprintf(fp, 
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
        fprintf(fp, "inc si\n");
        break;
    case '<':
        fprintf(fp, "dec si\n");
        break;
    case '+':
        fprintf(fp, "inc byte [buffer + esi]\n");
        break;
    case '-':
        fprintf(fp, "dec byte [buffer + esi]\n");
        break;
    case ',':
        fprintf(fp, 
                 "mov eax, 3\n"
                 "mov ebx, 0\n"
                 "lea ecx, [buffer + esi]\n"
                 "mov edx, 1\n"
                 "int 0x80\n");
        break;
    case '.':
        fprintf(fp, 
                 "mov eax, 4\n"
                 "mov ebx, 1\n"
                 "lea ecx, [buffer + esi]\n"
                 "mov edx, 1\n"
                 "int 0x80\n");
        break;
    case '[':
        fprintf(fp, "_l%lu:\n", labelno);
        old = labels;
        labels = malloc(sizeof(*labels));
        labels->prev = old;
        labels->labelno = labelno++;
        break;
    case ']':
        fprintf(fp, "cmp byte [buffer + esi], 0\n" 
                     "jne _l%lu\n", labels->labelno);
        old = labels;
        labels = labels->prev;
        free(old);
        break;
    }
    fprintf(fp, 
            "mov eax, 1\n"
            "mov ebx, [buffer + esi]\n"
            "int 0x80\n");
    fclose(fp);
    char command[260];    

    int len = snprintf(command, 260, "nasm -f elf32 -o "TMPFILE".o "TMPFILE".asm ; "
                                    "ld -m elf_i386 -s -o %s "TMPFILE".o ; "
                                    "rm "TMPFILE".asm "TMPFILE".o", argv[2]);
    if (len == 260 || len < 1) 
        return printf("Likely failed to create assembler command using snprintf, exiting to be safe\n");
    system(command);
}
