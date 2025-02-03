#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "disk.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_disk_image>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_file = "disk_defrag";

    FILE *input_disk = fopen(input_file, "rb");
    if (!input_disk) {
        perror("Error opening input disk image");
        return 1;
    }

    FILE *output_disk = fopen(output_file, "wb");
    if (!output_disk) {
        perror("Error creating output disk image");
        fclose(input_disk);
        return 1;
    }

    defragment(input_disk, output_disk);

    fclose(input_disk);
    fclose(output_disk);
    return 0;
}