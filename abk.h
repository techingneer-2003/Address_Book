#ifndef ADDRESS_BOOK_H
#define ADDRESS_BOOK_H

/* Address Book: portable, menu-driven contact manager. */
#include <stddef.h>

#define DEFAULT_FILE "telephone.csv"
#define AUDIT_FILE "ABK.LOG"
#define INITIAL_SIZE 16
#define NAME_SIZE 64
#define NUMBER_SIZE 32
#define EMAIL_SIZE 96
#define PHONE_NUMBER_COUNT 1
#define EMAIL_ADDRESS_COUNT 1
#define SAVED 0
#define NOT_SAVED 1

typedef enum {
    K_FAIL = 0, K_ADDRESS_BOOK_NULL, K_FILE_NOT_AVAILABLE,
    K_FILE_COULD_NOT_BE_CREATED, K_FILENAME_NULL,
    K_MEMORY_ALLOCATION_FAILURE, K_SUCCESS, K_WRONG_FILE_FORMAT
} return_t;

typedef struct {
    char name[NAME_SIZE];
    char phone_numbers[PHONE_NUMBER_COUNT][NUMBER_SIZE];
    char email_addresses[EMAIL_ADDRESS_COUNT][EMAIL_SIZE];
} contact_detail_t;

typedef struct {
    contact_detail_t *list;
    size_t size;
    size_t count;
    char filename[256];
    int dirty;
} abk_t;

int abk_init(abk_t *book, const char *filename);
void abk_destroy(abk_t *book);
int abk_load(abk_t *book);
int abk_save(abk_t *book);
void abk_menu(abk_t *book);
void abk_audit(const char *action, const char *details);

#endif
