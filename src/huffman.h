#pragma once
#include <stdint.h>

/*
Сжатие файла inputPath в outputPath
Возврат - 0 при успехе, -1 при ошибке
*/
int huffmanCompress(const char* inputPath, const char* outputPath);

/*
Разжатие файла inputPath (созданного huffmanCompress) в outputPath
Возврат - 0 при успехе, -1 при ошибке
*/
int huffmanDecompress(const char* inputPath, const char* outputPath);
