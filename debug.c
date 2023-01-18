#include "debug.h"

void printf_offset(char *str, FILE *file, size_t file_buffer_size, int offset) {
    size_t file_offset = ftell(file);
    printf("[debug] %s - offset: 0x%lx\n", str, (file_offset-file_buffer_size)+offset);
}