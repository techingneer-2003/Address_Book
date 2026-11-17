/*
 * Project     : Address Book
 * Author      : Maria Antony Bescy B
 * Date        : 17 November 2026
 * Description : Address Book application developed in C.
 */
/* User interface, validation, CRUD, searching, and sorting. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "abk.h"

#define INPUT_SIZE 256

static void trim(char *s)
{
    size_t n;
    char *p = s;
    while (isspace((unsigned char)*p)) ++p;
    if (p != s) memmove(s, p, strlen(p) + 1);
    n = strlen(s);
    while (n && isspace((unsigned char)s[n-1])) s[--n] = '\0';
}

/* Read a whole line safely; return 0 on end-of-file. */
static int read_line(const char *prompt, char *out, size_t cap)
{
    int ch; size_t n;
    if (prompt) { fputs(prompt, stdout); fflush(stdout); }
    if (!fgets(out, (int)cap, stdin)) return 0;
    n = strlen(out);
    if (n && out[n-1] == '\n') out[n-1] = '\0';
    else { while ((ch = getchar()) != '\n' && ch != EOF) { } }
    trim(out);
    return 1;
}

static int valid_phone(const char *s)
{
    size_t i, digits = 0;
    if (!*s) return 1;
    for (i = 0; s[i]; ++i) {
        if (isdigit((unsigned char)s[i])) ++digits;
        else if (s[i] != '+' && s[i] != '-' && s[i] != '(' && s[i] != ')' && s[i] != ' ')
            return 0;
    }
    return digits >= 5 && digits <= 15;
}

static int valid_email(const char *s)
{
    const char *at, *dot;
    if (!*s) return 1;
    at = strchr(s, '@'); dot = strrchr(s, '.');
    return at && at != s && at[1] && dot && dot > at + 1 && dot[1];
}

static int same_text(const char *a, const char *b)
{
    while (*a && *b) { if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0; ++a; ++b; }
    return *a == *b;
}

/* Portable case-insensitive substring search (avoids non-standard strcasestr). */
static int contains_case(const char *text, const char *query)
{
    size_t i, j;
    if (!*query) return 1;
    for (i = 0; text[i]; ++i) {
        for (j = 0; query[j] && text[i+j] &&
             tolower((unsigned char)text[i+j]) == tolower((unsigned char)query[j]); ++j) { }
        if (!query[j]) return 1;
    }
    return 0;
}

static int find_name(const abk_t *b, const char *name)
{
    size_t i;
    for (i = 0; i < b->count; ++i) if (same_text(b->list[i].name, name)) return (int)i;
    return -1;
}

static void print_contact(const contact_detail_t *c, size_t index)
{
    size_t i; int any;
    printf("\n[%zu] %s\n", index + 1, c->name);
    any = 0;
    for (i = 0; i < PHONE_NUMBER_COUNT; ++i) if (c->phone_numbers[i][0]) {
        printf("    Phone %zu: %s\n", i + 1, c->phone_numbers[i]); any = 1;
    }
    if (!any) puts("    Phone: (none)");
    any = 0;
    for (i = 0; i < EMAIL_ADDRESS_COUNT; ++i) if (c->email_addresses[i][0]) {
        printf("    Email %zu: %s\n", i + 1, c->email_addresses[i]); any = 1;
    }
    if (!any) puts("    Email: (none)");
}

static int ask_contact(contact_detail_t *c, const contact_detail_t *old)
{
    char buf[INPUT_SIZE], prompt[96]; size_t i;
    memset(c, 0, sizeof(*c));
    if (old) *c = *old;
    for (;;) {
        if (!read_line(old ? "Name [Enter keeps current]: " : "Full name: ", buf, sizeof(buf))) return 0;
        if (old && !*buf) break;
        if (!*buf) { puts("Name is required."); continue; }
        if (strlen(buf) >= NAME_SIZE) { printf("Name must be shorter than %d characters.\n", NAME_SIZE); continue; }
        break;
    }
    if (*buf) strcpy(c->name, buf);
    for (i = 0; i < PHONE_NUMBER_COUNT; ++i) {
        snprintf(prompt, sizeof(prompt), "Phone %zu%s: ", i+1, i ? " (optional)" : " (optional; Enter to skip)");
        for (;;) {
            if (!read_line(prompt, buf, sizeof(buf))) return 0;
            if (!*buf || valid_phone(buf)) break;
            puts("Invalid phone. Use digits and optional +, spaces, parentheses or hyphens (5-15 digits).");
        }
        if (strlen(buf) >= NUMBER_SIZE) { puts("Phone entry too long; skipped."); buf[0] = '\0'; }
        strcpy(c->phone_numbers[i], buf);
    }
    for (i = 0; i < EMAIL_ADDRESS_COUNT; ++i) {
        snprintf(prompt, sizeof(prompt), "Email %zu (optional): ", i+1);
        for (;;) {
            if (!read_line(prompt, buf, sizeof(buf))) return 0;
            if (!*buf || valid_email(buf)) break;
            puts("Please enter an email in a basic name@example.com format.");
        }
        if (strlen(buf) >= EMAIL_SIZE) { puts("Email entry too long; skipped."); buf[0] = '\0'; }
        strcpy(c->email_addresses[i], buf);
    }
    return 1;
}

static int save_if_needed(abk_t *b)
{
    if (!b->dirty) return 1;
    if (abk_save(b)) { puts("Changes saved."); return 1; }
    puts("ERROR: Could not save data. Changes remain in memory; check folder permissions.");
    return 0;
}

static void add_contact(abk_t *b)
{
    contact_detail_t c; contact_detail_t *p; size_t cap;
    if (!ask_contact(&c, NULL)) return;
    if (find_name(b, c.name) >= 0) {
        puts("A contact with that exact name already exists. Use Update to edit it."); return;
    }
    if (b->count == b->size) {
        cap = b->size ? b->size * 2 : INITIAL_SIZE;
        p = realloc(b->list, cap * sizeof(*p));
        if (!p) { puts("Not enough memory to add contact."); return; }
        b->list = p; b->size = cap;
    }
    b->list[b->count++] = c; b->dirty = 1;
    abk_audit("ADD", c.name); puts("Contact added."); save_if_needed(b);
}

static void list_contacts(const abk_t *b)
{
    size_t i;
    if (!b->count) { puts("\nAddress book is empty."); return; }
    printf("\n========== CONTACTS (%zu) ==========\n", b->count);
    for (i = 0; i < b->count; ++i) print_contact(&b->list[i], i);
}

static void search_contacts(const abk_t *b)
{
    char q[INPUT_SIZE]; size_t i, j; int found = 0;
    if (!read_line("Search name, phone or email: ", q, sizeof(q)) || !*q) return;
    for (i = 0; i < b->count; ++i) {
        contact_detail_t *c = &b->list[i];
        int matched = contains_case(c->name, q);
        for (j = 0; j < PHONE_NUMBER_COUNT && !matched; ++j)
            if (strstr(c->phone_numbers[j], q)) matched = 1;
        for (j = 0; j < EMAIL_ADDRESS_COUNT && !matched; ++j)
            if (contains_case(c->email_addresses[j], q)) matched = 1;
        if (matched) { print_contact(c, i); found = 1; }
    }
    if (!found) puts("No matching contacts found.");
    abk_audit("SEARCH", q);
}

static void update_contact(abk_t *b)
{
    char name[INPUT_SIZE]; int idx; contact_detail_t c;
    if (!read_line("Enter the exact name to update: ", name, sizeof(name)) || !*name) return;
    idx = find_name(b, name);
    if (idx < 0) { puts("Contact not found."); return; }
    print_contact(&b->list[idx], (size_t)idx);
    puts("Enter the new details. Leave the name blank to keep the existing name.");
    if (!ask_contact(&c, &b->list[idx])) return;
    if (find_name(b, c.name) >= 0 && !same_text(c.name, b->list[idx].name)) {
        puts("Another contact already uses that name."); return;
    }
    b->list[idx] = c; b->dirty = 1; abk_audit("UPDATE", c.name);
    puts("Contact updated."); save_if_needed(b);
}

static void delete_contact(abk_t *b)
{
    char name[INPUT_SIZE], confirm[16]; int idx;
    if (!read_line("Enter the exact name to delete: ", name, sizeof(name)) || !*name) return;
    idx = find_name(b, name);
    if (idx < 0) { puts("Contact not found."); return; }
    print_contact(&b->list[idx], (size_t)idx);
    if (!read_line("Delete this contact? (y/N): ", confirm, sizeof(confirm)) || (tolower((unsigned char)confirm[0]) != 'y')) {
        puts("Delete cancelled."); return;
    }
    if ((size_t)idx + 1 < b->count) memmove(&b->list[idx], &b->list[idx+1], (b->count-(size_t)idx-1)*sizeof(*b->list));
    --b->count; b->dirty = 1; abk_audit("DELETE", name); puts("Contact deleted."); save_if_needed(b);
}

static int compare_contacts(const void *a, const void *b)
{
    const contact_detail_t *x = a, *y = b;
    const unsigned char *p = (const unsigned char *)x->name;
    const unsigned char *q = (const unsigned char *)y->name;
    while (*p && *q) {
        int d = tolower(*p) - tolower(*q);
        if (d) return d;
        ++p; ++q;
    }
    return (int)*p - (int)*q;
}

static void sort_contacts(abk_t *b)
{
    qsort(b->list, b->count, sizeof(*b->list), compare_contacts);
    b->dirty = 1; abk_audit("SORT", "Contacts sorted alphabetically");
    puts("Contacts sorted alphabetically."); save_if_needed(b);
}

static void show_stats(const abk_t *b)
{
    size_t i, j, phones = 0, emails = 0;
    for (i = 0; i < b->count; ++i) {
        for (j = 0; j < PHONE_NUMBER_COUNT; ++j) if (b->list[i].phone_numbers[j][0]) ++phones;
        for (j = 0; j < EMAIL_ADDRESS_COUNT; ++j) if (b->list[i].email_addresses[j][0]) ++emails;
    }
    printf("\nDatabase statistics\n-------------------\nContacts: %zu\nPhone numbers: %zu\nEmail addresses: %zu\nStorage file: %s\n", b->count, phones, emails, b->filename);
}

void abk_menu(abk_t *b)
{
    char choice[32]; int running = 1;
    while (running) {
        puts("\n====================================");
        puts("          ADDRESS BOOK");
        puts("====================================");
        puts("1. Add contact");
        puts("2. View all contacts");
        puts("3. Search contacts");
        puts("4. Update contact");
        puts("5. Delete contact");
        puts("6. Sort contacts by name");
        puts("7. Database statistics");
        puts("8. Save changes");
        puts("0. Save and exit");
        if (!read_line("Choose an option: ", choice, sizeof(choice))) choice[0] = '0', choice[1] = '\0';
        if (!strcmp(choice,"1")) add_contact(b);
        else if (!strcmp(choice,"2")) list_contacts(b);
        else if (!strcmp(choice,"3")) search_contacts(b);
        else if (!strcmp(choice,"4")) update_contact(b);
        else if (!strcmp(choice,"5")) delete_contact(b);
        else if (!strcmp(choice,"6")) sort_contacts(b);
        else if (!strcmp(choice,"7")) show_stats(b);
        else if (!strcmp(choice,"8")) { if (!abk_save(b)) puts("Save failed; check permissions."); else puts("Saved successfully."); }
        else if (!strcmp(choice,"0")) { if (b->dirty && !save_if_needed(b)) {
                char answer[16];
                if (read_line("Exit anyway without saving? (y/N): ", answer, sizeof(answer)) && tolower((unsigned char)answer[0]) == 'y') running = 0;
            } else running = 0;
        } else puts("Invalid option. Please choose one of the displayed numbers.");
    }
    puts("Goodbye.");
}
