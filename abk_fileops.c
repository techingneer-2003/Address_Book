/*
 * Project     : Address Book
 * Author      : Maria Antony Bescy B
 * Date        : 17 November 2026
 * Description : Address Book application developed in C.
 */
/* Dynamic storage and CSV persistence for the Address Book. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "abk.h"

static int reserve(abk_t *b, size_t needed)
{
    contact_detail_t *p;
    size_t cap;
    if (needed <= b->size) return 1;
    cap = b->size ? b->size : INITIAL_SIZE;
    while (cap < needed) {
        if (cap > ((size_t)-1) / 2 / sizeof(*p)) return 0;
        cap *= 2;
    }
    p = realloc(b->list, cap * sizeof(*p));
    if (!p) return 0;
    b->list = p; b->size = cap;
    return 1;
}

int abk_init(abk_t *b, const char *filename)
{
    if (!b || !filename || !*filename || strlen(filename) >= sizeof(b->filename)) return 0;
    memset(b, 0, sizeof(*b));
    b->list = calloc(INITIAL_SIZE, sizeof(*b->list));
    if (!b->list) return 0;
    b->size = INITIAL_SIZE;
    strcpy(b->filename, filename);
    return 1;
}

void abk_destroy(abk_t *b)
{
    if (b) { free(b->list); b->list = NULL; b->count = b->size = 0; }
}

/* Parse one CSV record with quoted fields and escaped double quotes. */
static int parse_csv(char *line, char fields[][EMAIL_SIZE], size_t field_count)
{
    size_t f = 0, n = 0;
    char *p = line;
    while (*p && f < field_count) {
        n = 0;
        if (*p == '"') {
            ++p;
            while (*p) {
                if (*p == '"' && p[1] == '"') { if (n + 1 < EMAIL_SIZE) fields[f][n++] = '"'; p += 2; }
                else if (*p == '"') { ++p; break; }
                else { if (n + 1 < EMAIL_SIZE) fields[f][n++] = *p; ++p; }
            }
            while (*p && *p != ',') ++p;
        } else {
            while (*p && *p != ',' && *p != '\r' && *p != '\n') {
                if (n + 1 < EMAIL_SIZE) fields[f][n++] = *p;
                ++p;
            }
        }
        fields[f][n] = '\0'; ++f;
        if (*p == ',') ++p; else break;
    }
    return (f == field_count);
}

int abk_load(abk_t *b)
{
    FILE *fp;
    char line[2048], fields[1 + PHONE_NUMBER_COUNT + EMAIL_ADDRESS_COUNT][EMAIL_SIZE];
    size_t i;
    if (!b) return 0;
    fp = fopen(b->filename, "r");
    if (!fp) {
        /* A missing data file is a normal first run: create it on save. */
        if (fopen(b->filename, "r") == NULL) { b->count = 0; b->dirty = 0; return 1; }
        return 0;
    }
    while (fgets(line, sizeof(line), fp)) {
        contact_detail_t c;
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;
        memset(fields, 0, sizeof(fields));
        if (!parse_csv(line, fields, 1 + PHONE_NUMBER_COUNT + EMAIL_ADDRESS_COUNT)) {
            fclose(fp); return 0;
        }
        memset(&c, 0, sizeof(c));
        if (strlen(fields[0]) >= sizeof(c.name)) { fclose(fp); return 0; }
        strcpy(c.name, fields[0]);
        for (i = 0; i < PHONE_NUMBER_COUNT; ++i) {
            if (strlen(fields[1+i]) >= NUMBER_SIZE) { fclose(fp); return 0; }
            strcpy(c.phone_numbers[i], fields[1+i]);
        }
        for (i = 0; i < EMAIL_ADDRESS_COUNT; ++i) {
            if (strlen(fields[1+PHONE_NUMBER_COUNT+i]) >= EMAIL_SIZE) { fclose(fp); return 0; }
            strcpy(c.email_addresses[i], fields[1+PHONE_NUMBER_COUNT+i]);
        }
        if (!reserve(b, b->count + 1)) { fclose(fp); return 0; }
        b->list[b->count++] = c;
    }
    if (ferror(fp)) { fclose(fp); return 0; }
    fclose(fp); b->dirty = 0;
    return 1;
}

static int write_field(FILE *fp, const char *s)
{
    const char *p;
    if (fputc('"', fp) == EOF) return 0;
    for (p = s; *p; ++p) {
        if (*p == '"' && fputc('"', fp) == EOF) return 0;
        if (fputc((unsigned char)*p, fp) == EOF) return 0;
    }
    return fputc('"', fp) != EOF;
}

int abk_save(abk_t *b)
{
    FILE *fp;
    size_t i, j;
    char temp[300];
    if (!b) return 0;
    if (snprintf(temp, sizeof(temp), "%s.tmp", b->filename) >= (int)sizeof(temp)) return 0;
    fp = fopen(temp, "w");
    if (!fp) return 0;
    for (i = 0; i < b->count; ++i) {
        contact_detail_t *c = &b->list[i];
        if (!write_field(fp, c->name)) goto fail;
        for (j = 0; j < PHONE_NUMBER_COUNT; ++j) if (fputc(',', fp) == EOF || !write_field(fp, c->phone_numbers[j])) goto fail;
        for (j = 0; j < EMAIL_ADDRESS_COUNT; ++j) if (fputc(',', fp) == EOF || !write_field(fp, c->email_addresses[j])) goto fail;
        if (fputc('\n', fp) == EOF) goto fail;
    }
    if (fclose(fp) != 0) { remove(temp); return 0; }
    if (rename(temp, b->filename) != 0) { remove(temp); return 0; }
    b->dirty = 0; abk_audit("SAVE", "Contact database saved successfully");
    return 1;
fail:
    fclose(fp); remove(temp); return 0;
}
