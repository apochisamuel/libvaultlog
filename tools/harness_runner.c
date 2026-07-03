#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size);
int main(int argc, char *argv[]) {
    if (argc < 2) { fprintf(stderr, "Usage: %s <file>\n", argv[0]); return 1; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { fprintf(stderr, "Cannot open: %s\n", argv[1]); return 1; }
    fseek(f, 0, SEEK_END); size_t sz = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc(sz);
    if (!data) { fclose(f); return 1; }
    fread(data, 1, sz, f); fclose(f);
    fprintf(stderr, "Running input: %s (%zu bytes)\n", argv[1], sz);
    LLVMFuzzerTestOneInput(data, sz);
    fprintf(stderr, "Completed without crash.\n");
    free(data);
    return 0;
}
