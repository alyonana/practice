#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// Conversion tables from source encodings to Unicode
#include "cp1251_to_unicode.h"
#include "koi8r_to_unicode.h"
#include "iso88595_to_unicode.h"

void convert_to_utf8(uint16_t unicode, FILE* out) {
    if (unicode < 0x80) {
        fputc(unicode, out);
    } else if (unicode < 0x800) {
        fputc(0xC0 | (unicode >> 6), out);
        fputc(0x80 | (unicode & 0x3F), out);
    } else {
        fputc(0xE0 | (unicode >> 12), out);
        fputc(0x80 | ((unicode >> 6) & 0x3F), out);
        fputc(0x80 | (unicode & 0x3F), out);
    }
}

int convert_file(const char* input_file, const char* encoding, const char* output_file) {
    FILE* in = fopen(input_file, "rb");
    if (!in) {
        fprintf(stderr, "Error opening input file\n");
        return 1;
    }
    fseek(in, 0, SEEK_END);
    long filesize = ftell(in);
    if (filesize == 0) {
        fprintf(stderr, "Input file is empty\n");
        fclose(in);
        return 1;
    }
    rewind(in);

    FILE* out = fopen(output_file, "wb");
    if (!out) {
        fprintf(stderr, "Error opening output file\n");
        fclose(in);
        return 1;
    }

    const uint16_t* conversion_table;
    if (strcmp(encoding, "CP1251") == 0) {
        conversion_table = cp1251_to_unicode;
    } else if (strcmp(encoding, "KOI8-R") == 0) {
        conversion_table = koi8r_to_unicode;
    } else if (strcmp(encoding, "ISO-8859-5") == 0) {
        conversion_table = iso88595_to_unicode;
    } else {
        fprintf(stderr, "Unsupported encoding\n");
        fclose(in);
        fclose(out);
        return 1;
    }

    int ch;
    while ((ch = fgetc(in)) != EOF) {
        uint16_t unicode;
        if (ch < 0x80) {
            unicode = ch;
        } else {
            unicode = conversion_table[ch - 0x80];
        }
        convert_to_utf8(unicode, out);
    }

    fclose(in);
    fclose(out);
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input_file> <encoding> <output_file>\n", argv[0]);
        return 1;
    }

    return convert_file(argv[1], argv[2], argv[3]);
}