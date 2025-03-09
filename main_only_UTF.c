#include <stdio.h>
#include <string.h>
#include "unicode_table.h"

void write_utf8(FILE *out, uint16_t unicode) {
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

void convert_to_utf8(FILE *in, FILE *out, Encoding enc) {
    int ch;
    while ((ch = fgetc(in)) != EOF) {
        if (ch < 0x80) {
            write_utf8(out, ascii_table[ch]);
        } else {
            write_utf8(out, extended_tables[enc][ch - 0x80]);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <input_file> <encoding> <output_file>\n", argv[0]);
        return 1;
    }

    FILE *input = fopen(argv[1], "rb");
    if (!input) {
        printf("Cannot open input file\n");
        return 1;
    }

    FILE *output = fopen(argv[3], "wb");
    if (!output) {
        fclose(input);
        printf("Cannot open output file\n");
        return 1;
    }

    Encoding encoding;
    if (strcmp(argv[2], "CP1251") == 0) {
        encoding = CP1251;
    } else if (strcmp(argv[2], "KOI8-R") == 0) {
        encoding = KOI8R;
    } else if (strcmp(argv[2], "ISO-8859-5") == 0) {
        encoding = ISO8859_5;
    } else {
        printf("Supported encodings: CP1251, KOI8-R, ISO-8859-5\n");
        fclose(input);
        fclose(output);
        return 1;
    }

    convert_to_utf8(input, output, encoding);

    fclose(input);
    fclose(output);
    return 0;
}
