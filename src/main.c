#include "huffman.h"
#include <stdio.h>
#include <string.h>

static void printUsage(const char* prog)
{
    fprintf(stderr, "Использование:\n");
    fprintf(stderr, "  %s -c <входной_файл> <выходной_файл>   сжатие\n", prog);
    fprintf(stderr, "  %s -d <входной_файл> <выходной_файл>   разжатие\n", prog);
}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        printUsage(argv[0]);
        return 1;
    }

    const char* mode = argv[1];
    const char* input = argv[2];
    const char* output = argv[3];

    if (strcmp(mode, "-c") == 0) {
        if (huffmanCompress(input, output) != 0) {
            fprintf(stderr, "Сжатие не удалось.\n");
            return 1;
        }
    } else if (strcmp(mode, "-d") == 0) {
        if (huffmanDecompress(input, output) != 0) {
            fprintf(stderr, "Разжатие не удалось.\n");
            return 1;
        }
    } else {
        printUsage(argv[0]);
        return 1;
    }

    return 0;
}
