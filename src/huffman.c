#include "huffman.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============== Двоичное дерево ===============

// Структура узла
typedef struct Node {
    uint64_t freq;
    uint8_t byte;
    struct Node* left;
    struct Node* right;
    struct Node* parent;
    uint8_t isLeft;
} Node;

// Освобождение дерева
static void freeTree(Node* node)
{
    if (!node)
        return;

    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

// =============== Минимальная куча ===============

// Структура кучи
typedef struct {
    Node** arr;
    int size;
    int capacity;
} MinHeap;

// Создание кучи
static MinHeap* heapCreate(int capacity)
{
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));

    heap->arr = (Node**)malloc(sizeof(Node*) * capacity);
    heap->size = 0;
    heap->capacity = capacity;

    return heap;
}

// Освобождение кучи
static void heapFree(MinHeap* heap)
{
    if (heap) {
        free(heap->arr);
        free(heap);
    }
}

// Добавление узла в кучу
static void heapPush(MinHeap* heap, Node* node)
{
    // если массив заполнен, увеличиваем ёмкость вдвое
    if (heap->size >= heap->capacity) {
        heap->capacity *= 2;
        Node** newArr = (Node**)realloc(heap->arr, sizeof(Node*) * heap->capacity);
        if (!newArr) {
            return;
        }
        heap->arr = newArr;
    }

    int i = heap->size++;

    // поднимаем элемент вверх, пока не восстановим свойство кучи
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap->arr[parent]->freq <= node->freq)
            break;
        heap->arr[i] = heap->arr[parent];
        i = parent;
    }

    heap->arr[i] = node;
}

// Извлечение минимального узла
static Node* heapPop(MinHeap* heap)
{
    if (heap->size == 0)
        return NULL;

    Node* top = heap->arr[0];
    Node* last = heap->arr[--heap->size];
    int i = 0;

    // восстанавливаем свойство кучи
    while (true) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = i;
        if (left < heap->size && heap->arr[left]->freq < heap->arr[smallest]->freq)
            smallest = left;
        if (right < heap->size && heap->arr[right]->freq < heap->arr[smallest]->freq)
            smallest = right;
        if (smallest == i)
            break;
        heap->arr[i] = heap->arr[smallest];
        i = smallest;
    }

    heap->arr[i] = last;
    return top;
}

/* =============== Битовый вывод =============== */

// Структура для накопления битов и записи их байтами в файл
typedef struct {
    FILE* file;
    uint8_t buffer;
    int bitsCount;
} BitWriter;

// Инициализация битового писателя
static void bitWriterInit(BitWriter* bw, FILE* file)
{
    bw->file = file;
    bw->buffer = 0;
    bw->bitsCount = 0;
}

// Запись одного бита в накопитель
static void bitWriterWriteBit(BitWriter* bw, int bit)
{
    if (bit)
        bw->buffer |= (1 << (7 - bw->bitsCount));

    bw->bitsCount++;

    if (bw->bitsCount == 8) {
        fwrite(&bw->buffer, 1, 1, bw->file);

        bw->buffer = 0;
        bw->bitsCount = 0;
    }
}

// Заполнение байта до конца
static void bitWriterFlush(BitWriter* bw)
{
    if (bw->bitsCount > 0)
        fwrite(&bw->buffer, 1, 1, bw->file);
}

/* =============== Битовый ввод =============== */

// Структура для чтения битов из файла
typedef struct {
    FILE* file;
    uint8_t buffer;
    int bitsLeft;
} BitReader;

// Инициализация битового читателя
static void bitReaderInit(BitReader* br, FILE* file)
{
    br->file = file;
    br->buffer = 0;
    br->bitsLeft = 0;
}

// Чтение одного бита, возвращает бит или -1 при ошибке или конце файла
static int bitReaderReadBit(BitReader* br)
{
    // если в буфере нет непрочитанных битов, читаем новый байт
    if (br->bitsLeft == 0) {
        int c = fgetc(br->file);

        if (c == EOF)
            return -1;

        br->buffer = (uint8_t)c;
        br->bitsLeft = 8;
    }
    // извлекаем старший бит
    int bit = (br->buffer >> 7) & 1;
    // сдвигаем буфер влево, чтобы следующий бит стал старшим
    br->buffer <<= 1;
    // уменьшаем счётчик оставшихся битов
    br->bitsLeft--;

    return bit;
}

/* =============== Запись/чтение целых чисел в формате little-endian =============== */

// Запись 64-битного беззнакового числа
static void writeUint64LE(uint64_t value, FILE* file)
{
    for (int i = 0; i < 8; ++i) {
        fputc((int)(value & 0xFF), file);
        value >>= 8;
    }
}

// Чтение 64-битного беззнакового числа
static uint64_t readUint64LE(FILE* file)
{
    uint64_t value = 0;

    for (int i = 0; i < 8; ++i) {
        int c = fgetc(file);

        if (c == EOF)
            return 0;

        value |= ((uint64_t)(uint8_t)c) << (i * 8);
    }

    return value;
}

// Запись 16-битного беззнакового числа
static void writeUint16LE(uint16_t value, FILE* file)
{
    fputc(value & 0xFF, file);
    fputc((value >> 8) & 0xFF, file);
}

// Чтение 16-битного беззнакового числа
static uint16_t readUint16LE(FILE* file)
{
    int lo = fgetc(file);
    int hi = fgetc(file);

    if (lo == EOF || hi == EOF)
        return 0;

    // собираем число из младшего и старшего байта
    return (uint16_t)lo | ((uint16_t)hi << 8);
}

/* =============== Преобразование листа в битовую последовательность =============== */

// Заполняет массив bits кодом Хаффмана для полученного листа
static void getCodeBits(Node* leaf, uint8_t bits[256], int* len)
{
    uint8_t stack[256];
    int top = 0;
    Node* node = leaf;

    while (node->parent) {
        stack[top++] = node->isLeft ? 0 : 1;
        node = node->parent;
    }

    *len = top;

    for (int i = 0; i < top; ++i)
        bits[i] = stack[top - 1 - i];
}

/* =============== Построение дерева Хаффмана =============== */

// Построение дерева по таблице частот, leafNodes (если не NULL) заполняется указателями на листья
static Node* buildTreeFromFreqs(const uint64_t freqs[256], Node* leafNodes[256])
{
    MinHeap* heap = heapCreate(256);
    int nonZero = 0;

    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            Node* node = (Node*)calloc(1, sizeof(Node));
            node->freq = freqs[i];
            node->byte = (uint8_t)i;
            heapPush(heap, node);
            if (leafNodes)
                leafNodes[i] = node;
            nonZero++;
        } else if (leafNodes) {
            leafNodes[i] = NULL;
        }
    }

    if (nonZero == 0) {
        heapFree(heap);
        return NULL;
    }

    if (nonZero == 1) {
        Node* root = heapPop(heap);
        heapFree(heap);
        return root;
    }

    while (heap->size > 1) {
        Node* left = heapPop(heap);
        Node* right = heapPop(heap);
        Node* parent = (Node*)calloc(1, sizeof(Node));
        parent->freq = left->freq + right->freq;
        parent->left = left;
        parent->right = right;
        left->parent = parent;
        right->parent = parent;
        left->isLeft = 1;
        right->isLeft = 0;
        heapPush(heap, parent);
    }

    Node* root = heapPop(heap);
    heapFree(heap);
    return root;
}

/* =============== Функции для пользователя =============== */

/*
Сжатие файла inputPath в outputPath
Возврат - 0 при успехе, -1 при ошибке
*/
int huffmanCompress(const char* inputPath, const char* outputPath)
{
    FILE* in = NULL;
    FILE* out = NULL;
    Node* root = NULL;
    Node* leafNodes[256] = { NULL };
    int ret = -1;

    in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Ошибка: не удалось открыть %s\n", inputPath);
        goto cleanup;
    }

    uint64_t freqs[256] = { 0 };
    uint64_t fileSize = 0;
    int c = 0;

    // подсчёт частот
    while ((c = fgetc(in)) != EOF) {
        freqs[(uint8_t)c]++;
        fileSize++;
    }

    // возвращаемся в начало файла
    rewind(in);

    out = fopen(outputPath, "wb");

    if (!out) {
        fprintf(stderr, "Ошибка: не удалось создать %s\n", outputPath);
        goto cleanup;
    }

    // подсчёт ненулевых частот
    uint16_t numNonzero = 0;
    for (int i = 0; i < 256; ++i)
        if (freqs[i] > 0)
            numNonzero++;

    // запись заголовка (размер оригинала, количество символов)
    writeUint64LE(fileSize, out);
    writeUint16LE(numNonzero, out);

    // запись только ненулевых (байт, частота)
    for (int i = 0; i < 256; ++i) {
        if (freqs[i] > 0) {
            fputc(i, out);
            writeUint64LE(freqs[i], out);
        }
    }

    // если файл пуст, заголовок уже записан —> успех
    if (fileSize == 0) {
        ret = 0;
        goto cleanup;
    }

    // строим дерево Хаффмана
    root = buildTreeFromFreqs(freqs, leafNodes);
    if (!root) {
        fprintf(stderr, "Ошибка: не удалось построить дерево Хаффмана\n");
        goto cleanup;
    }

    // инициализируем битовый писатель
    BitWriter bw;
    bitWriterInit(&bw, out);

    // случай одного уникального символа
    if (root->left == NULL && root->right == NULL) {
        for (uint64_t i = 0; i < fileSize; ++i)
            bitWriterWriteBit(&bw, 0);
        bitWriterFlush(&bw);
        ret = 0;
        goto cleanup;
    }

    // возвращаемся в начало файла
    rewind(in);

    // кодируем каждый символ
    uint8_t bits[256];
    int bitLen;
    for (uint64_t i = 0; i < fileSize; ++i) {
        int byte = fgetc(in);
        if (byte == EOF) {
            fprintf(stderr, "Ошибка: преждевременный конец входного файла\n");
            goto cleanup;
        }

        Node* leaf = leafNodes[byte];
        if (!leaf) {
            fprintf(stderr, "Ошибка: нет листа для байта 0x%02X\n", byte);
            goto cleanup;
        }

        getCodeBits(leaf, bits, &bitLen);
        for (int j = 0; j < bitLen; ++j)
            bitWriterWriteBit(&bw, bits[j]);
    }
    bitWriterFlush(&bw);
    ret = 0;

cleanup:
    if (in)
        fclose(in);
    if (out)
        fclose(out);
    if (root)
        freeTree(root);
    return ret;
}

/*
Разжатие файла inputPath (созданного huffmanCompress) в outputPath
Возврат - 0 при успехе, -1 при ошибке
*/
int huffmanDecompress(const char* inputPath, const char* outputPath)
{
    FILE* in = NULL;
    FILE* out = NULL;
    Node* root = NULL;
    int ret = -1;

    in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Ошибка: не удалось открыть %s\n", inputPath);
        goto cleanup;
    }

    // чтение заголовка
    uint64_t originalSize = readUint64LE(in);
    uint16_t numNonzero = readUint16LE(in);
    uint64_t freqs[256] = { 0 };
    for (uint16_t i = 0; i < numNonzero; ++i) {
        int byte = fgetc(in);
        if (byte == EOF) {
            fprintf(stderr, "Ошибка: неполный заголовок\n");
            goto cleanup;
        }
        uint64_t freq = readUint64LE(in);
        freqs[(uint8_t)byte] = freq;
    }

    out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Ошибка: не удалось создать %s\n", outputPath);
        goto cleanup;
    }

    // если исходный файл был пуст, выходной тоже
    if (originalSize == 0) {
        ret = 0;
        goto cleanup;
    }

    // восстанавливаем дерево Хаффмана
    root = buildTreeFromFreqs(freqs, NULL);
    if (!root) {
        fprintf(stderr, "Ошибка: не удалось восстановить дерево Хаффмана\n");
        goto cleanup;
    }

    // инициализируем битовый читатель
    BitReader br;
    bitReaderInit(&br, in);

    // случай одного уникального символа
    if (root->left == NULL && root->right == NULL) {
        uint8_t sym = root->byte;
        for (uint64_t i = 0; i < originalSize; ++i) {
            int bit = bitReaderReadBit(&br);
            if (bit == -1) {
                fprintf(stderr, "Ошибка: неожиданный конец файла при разжатии\n");
                goto cleanup;
            }
            fputc(sym, out);
        }
        ret = 0;
        goto cleanup;
    }

    // общий случай - декодируем биты до листа
    for (uint64_t i = 0; i < originalSize; ++i) {
        Node* node = root;
        if (!node) {
            fprintf(stderr, "Ошибка: корень дерева NULL\n");
            goto cleanup;
        }
        while (node->left || node->right) {
            int bit = bitReaderReadBit(&br);
            if (bit == -1) {
                fprintf(stderr, "Ошибка: конец файла при разжатии\n");
                goto cleanup;
            }
            Node* next = bit ? node->right : node->left;
            if (!next) {
                fprintf(stderr, "Ошибка: некорректный битовый поток\n");
                goto cleanup;
            }
            node = next;
        }
        fputc(node->byte, out);
    }

    ret = 0;

cleanup:
    if (in)
        fclose(in);
    if (out)
        fclose(out);
    if (root)
        freeTree(root);
    return ret;
}