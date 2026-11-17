/* Entry point for the Address Book application. */
#include <stdio.h>
#include <string.h>
#include "abk.h"

int main(int argc, char **argv)
{
    abk_t book;
    const char *filename = (argc > 1) ? argv[1] : DEFAULT_FILE;

    if (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
        puts("Address Book - contact manager\n"
             "Usage: ./abk [data-file.csv]\n\n"
             "Without an argument, the program uses telephone.csv in the current directory.\n"
             "Menu features: add, list, search, update, delete, sort, statistics, save, exit.\n"
             "Contacts are stored in CSV format; actions are recorded in ABK.LOG.");
        return 0;
    }
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [data-file.csv]\n", argv[0]);
        return 2;
    }
    if (!abk_init(&book, filename)) {
        fprintf(stderr, "Could not initialize address book (out of memory or invalid filename).\n");
        return 1;
    }
    if (!abk_load(&book)) {
        fprintf(stderr, "Could not load '%s'. Check permissions and file format.\n", filename);
        abk_destroy(&book);
        return 1;
    }
    abk_menu(&book);
    abk_destroy(&book);
    return 0;
}
