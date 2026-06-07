#include "../src/huffman.h"
#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEMP_COMPRESSED_PATH "compressed.bin"
#define TEMP_RESTORED_PATH "tests/output/restored.bin"

static bool filesEqual(const char* path1, const char* path2)
{
    FILE* f1 = fopen(path1, "rb");
    if (!f1)
        return false;
    FILE* f2 = fopen(path2, "rb");
    if (!f2) {
        fclose(f1);
        return false;
    }

    bool result = true;
    int c1;
    int c2;
    do {
        c1 = fgetc(f1);
        c2 = fgetc(f2);

        if (c1 != c2) {
            result = false;
            break;
        }
    } while (c1 != EOF && c2 != EOF);

    fclose(f1);
    fclose(f2);

    return result;
}

static void testOneFile(const char* inputPath)
{
    int ret = huffmanCompress(inputPath, TEMP_COMPRESSED_PATH);
    assert(ret == 0);

    ret = huffmanDecompress(TEMP_COMPRESSED_PATH, TEMP_RESTORED_PATH);
    assert(ret == 0);

    assert(filesEqual(inputPath, TEMP_RESTORED_PATH));

    remove(TEMP_COMPRESSED_PATH);
    remove(TEMP_RESTORED_PATH);

    printf("PASS: %s\n", inputPath);
}

int main()
{
    const char* testFiles[] = {
        "tests/input/input0.txt",
        "tests/input/input1.txt",
        "tests/input/input2.txt",
        "tests/input/input3.txt",
        "tests/input/input4.txt",
        "tests/input/input5.txt",
    };
    const int NUM_TESTS = sizeof(testFiles) / sizeof(testFiles[0]);

    for (int i = 0; i < NUM_TESTS; ++i)
        testOneFile(testFiles[i]);

    printf("All tests passed.\n");
    return 0;
}
