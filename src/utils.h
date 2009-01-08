#include <glib.h>
#include <string.h>

char *replace_tokens(const char *src, const char** from, char** to, int n);
char * replace(const char *src, const char *from, const char *to);
