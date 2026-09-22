#include "huffman.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============== Двоичное дерево ===============

// Структура узла
typedef struct Node {
    uint64_t frequency;
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
    Node** nodes;
    int size;
    int capacity;
} MinHeap;

// Создание кучи
static MinHeap* heapCreate(int capacity)
{
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    if (!heap)
        return NULL;

    heap->nodes = (Node**)malloc(sizeof(Node*) * capacity);
    if (!heap->nodes) {
        free(heap);
        return NULL;
    }

    heap->size = 0;
    heap->capacity = capacity;

    return heap;
}

// Освобождение кучи
static void heapFree(MinHeap* heap)
{
    if (heap) {
        free(heap->nodes);
        free(heap);
    }
}

// Добавление узла в кучу
static int heapPush(MinHeap* heap, Node* node)
{
    // если массив заполнен, увеличиваем ёмкость вдвое
    if (heap->size >= heap->capacity) {
        int newCapacity = heap->capacity * 2;
        Node** newArr = (Node**)realloc(heap->nodes, sizeof(Node*) * newCapacity);
        if (!newArr)
            return -1;
        heap->nodes = newArr;
        heap->capacity = newCapacity;
    }

    int i = heap->size++;

    // поднимаем элемент вверх, пока не восстановим свойство кучи
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap->nodes[parent]->frequency <= node->frequency)
            break;
        heap->nodes[i] = heap->nodes[parent];
        i = parent;
    }

    heap->nodes[i] = node;
    return 0;
}

// Извлечение минимального узла
static Node* heapPop(MinHeap* heap)
{
    if (heap->size == 0)
        return NULL;

    Node* top = heap->nodes[0];
    Node* last = heap->nodes[--heap->size];
    int i = 0;

    // восстанавливаем свойство кучи
    while (true) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left >= heap->size)
            break;

        int smallest = left;
        if (right < heap->size && heap->nodes[right]->frequency < heap->nodes[left]->frequency)
            smallest = right;
        if (last->frequency <= heap->nodes[smallest]->frequency)
            break;

        heap->nodes[i] = heap->nodes[smallest];
        i = smallest;
    }

    heap->nodes[i] = last;
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
static void bitWriterInit(BitWriter* bitWriter, FILE* file)
{
    bitWriter->file = file;
    bitWriter->buffer = 0;
    bitWriter->bitsCount = 0;
}

// Запись одного бита в накопитель
static void bitWriterWriteBit(BitWriter* bitWriter, int bit)
{
    if (bit)
        bitWriter->buffer |= (1 << (7 - bitWriter->bitsCount));

    bitWriter->bitsCount++;

    if (bitWriter->bitsCount == 8) {
        fwrite(&bitWriter->buffer, 1, 1, bitWriter->file);

        bitWriter->buffer = 0;
        bitWriter->bitsCount = 0;
    }
}

// Заполнение байта до конца
static void bitWriterFlush(BitWriter* bitWriter)
{
    if (bitWriter->bitsCount > 0)
        fwrite(&bitWriter->buffer, 1, 1, bitWriter->file);
}

/* =============== Битовый ввод =============== */

// Структура для чтения битов из файла
typedef struct {
    FILE* file;
    uint8_t buffer;
    int bitsLeft;
} BitReader;

// Инициализация битового читателя
static void bitReaderInit(BitReader* bitReader, FILE* file)
{
    bitReader->file = file;
    bitReader->buffer = 0;
    bitReader->bitsLeft = 0;
}

// Чтение одного бита, возвращает бит или -1 при ошибке или конце файла
static int bitReaderReadBit(BitReader* bitReader)
{
    // если в буфере нет непрочитанных битов, читаем новый байт
    if (bitReader->bitsLeft == 0) {
        int c = fgetc(bitReader->file);

        if (c == EOF)
            return -1;

        bitReader->buffer = (uint8_t)c;
        bitReader->bitsLeft = 8;
    }
    // извлекаем старший бит
    int bit = (bitReader->buffer >> 7) & 1;
    // сдвигаем буфер влево, чтобы следующий бит стал старшим
    bitReader->buffer <<= 1;
    // уменьшаем счётчик оставшихся битов
    bitReader->bitsLeft--;

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
    int lowByte = fgetc(file);
    int highByte = fgetc(file);

    if (lowByte == EOF || highByte == EOF)
        return 0;

    // собираем число из младшего и старшего байта
    return (uint16_t)lowByte | ((uint16_t)highByte << 8);
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
static Node* buildTreeFromFreqs(const uint64_t frequencies[256], Node* leafNodes[256])
{
    MinHeap* heap = heapCreate(256);
    if (!heap)
        return NULL;

    int nonZero = 0;

    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            Node* node = (Node*)calloc(1, sizeof(Node));
            if (!node) {
                for (int k = 0; k < heap->size; ++k)
                    freeTree(heap->nodes[k]);
                heapFree(heap);
                return NULL;
            }

            node->frequency = frequencies[i];
            node->byte = (uint8_t)i;

            if (heapPush(heap, node) != 0) {
                freeTree(node);
                for (int k = 0; k < heap->size; ++k)
                    freeTree(heap->nodes[k]);
                heapFree(heap);
                return NULL;
            }

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
        if (!parent) {
            for (int k = 0; k < heap->size; ++k)
                freeTree(heap->nodes[k]);
            freeTree(left);
            freeTree(right);
            heapFree(heap);
            return NULL;
        }

        parent->frequency = left->frequency + right->frequency;
        parent->left = left;
        parent->right = right;
        left->parent = parent;
        right->parent = parent;
        left->isLeft = 1;
        right->isLeft = 0;
        if (heapPush(heap, parent) != 0) {
            freeTree(parent);
            for (int k = 0; k < heap->size; ++k)
                freeTree(heap->nodes[k]);
            heapFree(heap);
            return NULL;
        }
    }

    Node* root = heapPop(heap);
    heapFree(heap);
    return root;
}

// =============== Функция очистки ресурсов ===============

static void freeResources(FILE** in, FILE** out, Node** root)
{
    if (in && *in) {
        fclose(*in);
        *in = NULL;
    }
    if (out && *out) {
        fclose(*out);
        *out = NULL;
    }
    if (root && *root) {
        freeTree(*root);
        *root = NULL;
    }
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

    in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Ошибка: не удалось открыть %s\n", inputPath);
        freeResources(&in, &out, &root);
        return -1;
    }

    uint64_t frequencies[256] = { 0 };
    uint64_t fileSize = 0;
    int c = 0;

    // подсчёт частот
    while ((c = fgetc(in)) != EOF) {
        frequencies[(uint8_t)c]++;
        fileSize++;
    }

    // возвращаемся в начало файла
    rewind(in);

    out = fopen(outputPath, "wb");

    if (!out) {
        fprintf(stderr, "Ошибка: не удалось создать %s\n", outputPath);
        freeResources(&in, &out, &root);
        return -1;
    }

    // подсчёт ненулевых частот
    uint16_t numNonzero = 0;
    for (int i = 0; i < 256; ++i)
        if (frequencies[i] > 0)
            numNonzero++;

    // запись заголовка (размер оригинала, количество символов)
    writeUint64LE(fileSize, out);
    writeUint16LE(numNonzero, out);

    // запись только ненулевых (байт, частота)
    for (int i = 0; i < 256; ++i) {
        if (frequencies[i] > 0) {
            fputc(i, out);
            writeUint64LE(frequencies[i], out);
        }
    }

    // если файл пуст, заголовок уже записан —> успех
    if (fileSize == 0) {
        freeResources(&in, &out, &root);
        return 0;
    }

    // строим дерево Хаффмана
    root = buildTreeFromFreqs(frequencies, leafNodes);
    if (!root) {
        fprintf(stderr, "Ошибка: не удалось построить дерево Хаффмана\n");
        freeResources(&in, &out, &root);
        return -1;
    }

    // инициализируем битовый писатель
    BitWriter bitWriter;
    bitWriterInit(&bitWriter, out);

    // случай одного уникального символа
    if (root->left == NULL && root->right == NULL) {
        for (uint64_t i = 0; i < fileSize; ++i)
            bitWriterWriteBit(&bitWriter, 0);
        bitWriterFlush(&bitWriter);
        freeResources(&in, &out, &root);
        return 0;
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
            freeResources(&in, &out, &root);
            return -1;
        }

        Node* leaf = leafNodes[byte];
        if (!leaf) {
            fprintf(stderr, "Ошибка: нет листа для байта 0x%02X\n", byte);
            freeResources(&in, &out, &root);
            return -1;
        }

        getCodeBits(leaf, bits, &bitLen);
        for (int j = 0; j < bitLen; ++j)
            bitWriterWriteBit(&bitWriter, bits[j]);
    }
    bitWriterFlush(&bitWriter);
    freeResources(&in, &out, &root);
    return 0;
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

    in = fopen(inputPath, "rb");
    if (!in) {
        fprintf(stderr, "Ошибка: не удалось открыть %s\n", inputPath);
        freeResources(&in, &out, &root);
        return -1;
    }

    // чтение заголовка
    uint64_t originalSize = readUint64LE(in);
    uint16_t numNonzero = readUint16LE(in);
    uint64_t frequencies[256] = { 0 };
    for (uint16_t i = 0; i < numNonzero; ++i) {
        int byte = fgetc(in);
        if (byte == EOF) {
            fprintf(stderr, "Ошибка: неполный заголовок\n");
            freeResources(&in, &out, &root);
            return -1;
        }
        uint64_t frequency = readUint64LE(in);
        frequencies[(uint8_t)byte] = frequency;
    }

    out = fopen(outputPath, "wb");
    if (!out) {
        fprintf(stderr, "Ошибка: не удалось создать %s\n", outputPath);
        freeResources(&in, &out, &root);
        return -1;
    }

    // если исходный файл был пуст, выходной тоже
    if (originalSize == 0) {
        freeResources(&in, &out, &root);
        return 0;
    }

    // восстанавливаем дерево Хаффмана
    root = buildTreeFromFreqs(frequencies, NULL);
    if (!root) {
        fprintf(stderr, "Ошибка: не удалось восстановить дерево Хаффмана\n");
        freeResources(&in, &out, &root);
        return -1;
    }

    // инициализируем битовый читатель
    BitReader bitReader;
    bitReaderInit(&bitReader, in);

    // случай одного уникального символа
    if (root->left == NULL && root->right == NULL) {
        uint8_t sym = root->byte;
        for (uint64_t i = 0; i < originalSize; ++i) {
            int bit = bitReaderReadBit(&bitReader);
            if (bit == -1) {
                fprintf(stderr, "Ошибка: неожиданный конец файла при разжатии\n");
                freeResources(&in, &out, &root);
                return -1;
            }
            fputc(sym, out);
        }
        freeResources(&in, &out, &root);
        return 0;
    }

    // общий случай - декодируем биты до листа
    for (uint64_t i = 0; i < originalSize; ++i) {
        Node* node = root;
        if (!node) {
            fprintf(stderr, "Ошибка: корень дерева NULL\n");
            freeResources(&in, &out, &root);
            return -1;
        }
        while (node->left || node->right) {
            int bit = bitReaderReadBit(&bitReader);
            if (bit == -1) {
                fprintf(stderr, "Ошибка: конец файла при разжатии\n");
                freeResources(&in, &out, &root);
                return -1;
            }
            Node* next = bit ? node->right : node->left;
            if (!next) {
                fprintf(stderr, "Ошибка: некорректный битовый поток\n");
                freeResources(&in, &out, &root);
                return -1;
            }
            node = next;
        }
        fputc(node->byte, out);
    }

    freeResources(&in, &out, &root);
    return 0;
}
