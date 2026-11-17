/* Append-only audit log. Logging must never block normal application use. */
#include <stdio.h>
#include <time.h>
#include "abk.h"

void abk_audit(const char *action, const char *details)
{
    FILE *fp = fopen(AUDIT_FILE, "a");
    time_t now;
    struct tm *tm_info;
    char stamp[32] = "unknown-time";
    if (!fp) return;
    now = time(NULL); tm_info = localtime(&now);
    if (tm_info) strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(fp, "[%s] %-10s %s\n", stamp, action ? action : "EVENT", details ? details : "");
    fclose(fp);
}
