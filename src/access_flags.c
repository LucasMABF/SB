/**
 * @file access_flags.c
 * @brief Decodificação das access_flags (ver access_flags.h).
 */
#include "access_flags.h"

#include <string.h>

typedef struct {
    u2          mask;
    const char *name;
} FlagName;

static const FlagName CLASS_FLAGS[] = {
    {0x0001, "public"},    {0x0010, "final"},      {0x0020, "super"},
    {0x0200, "interface"}, {0x0400, "abstract"},   {0x1000, "synthetic"},
    {0x2000, "annotation"},{0x4000, "enum"},       {0x8000, "module"},
    {0, NULL}
};

static const FlagName FIELD_FLAGS[] = {
    {0x0001, "public"},    {0x0002, "private"},    {0x0004, "protected"},
    {0x0008, "static"},    {0x0010, "final"},      {0x0040, "volatile"},
    {0x0080, "transient"}, {0x1000, "synthetic"},  {0x4000, "enum"},
    {0, NULL}
};

static const FlagName METHOD_FLAGS[] = {
    {0x0001, "public"},    {0x0002, "private"},    {0x0004, "protected"},
    {0x0008, "static"},    {0x0010, "final"},      {0x0020, "synchronized"},
    {0x0040, "bridge"},    {0x0080, "varargs"},    {0x0100, "native"},
    {0x0400, "abstract"},  {0x0800, "strict"},     {0x1000, "synthetic"},
    {0, NULL}
};

static const FlagName INNER_CLASS_FLAGS[] = {
    {0x0001, "public"},    {0x0002, "private"},    {0x0004, "protected"},
    {0x0008, "static"},    {0x0010, "final"},      {0x0200, "interface"},
    {0x0400, "abstract"},  {0x1000, "synthetic"},  {0x2000, "annotation"},
    {0x4000, "enum"},
    {0, NULL}
};

void flags_to_string(u2 flags, FlagContext ctx, char *buf, size_t buf_len)
{
    const FlagName *table = CLASS_FLAGS;
    switch (ctx) {
    case FLAGS_CLASS:       table = CLASS_FLAGS; break;
    case FLAGS_FIELD:       table = FIELD_FLAGS; break;
    case FLAGS_METHOD:      table = METHOD_FLAGS; break;
    case FLAGS_INNER_CLASS: table = INNER_CLASS_FLAGS; break;
    }

    if (buf_len == 0) return;
    buf[0] = '\0';
    for (const FlagName *f = table; f->name; f++) {
        if (flags & f->mask) {
            if (buf[0] != '\0') strncat(buf, " ", buf_len - strlen(buf) - 1);
            strncat(buf, f->name, buf_len - strlen(buf) - 1);
        }
    }
}
