/**
 * @file reader.c
 * @brief Implementação do ByteReader (ver reader.h).
 */
#include "reader.h"

#include <stdio.h>
#include <stdlib.h>

void br_init(ByteReader *r, const u1 *data, size_t size)
{
    r->data = data;
    r->size = size;
    r->pos = 0;
    r->error = 0;
}

/* Garante que há n bytes disponíveis; caso contrário marca erro. */
static int br_need(ByteReader *r, size_t n)
{
    if (r->error || r->size - r->pos < n) {
        r->error = 1;
        return 0;
    }
    return 1;
}

u1 br_u1(ByteReader *r)
{
    if (!br_need(r, 1)) return 0;
    return r->data[r->pos++];
}

u2 br_u2(ByteReader *r)
{
    if (!br_need(r, 2)) return 0;
    const u1 *p = r->data + r->pos;
    r->pos += 2;
    return (u2)((p[0] << 8) | p[1]);
}

u4 br_u4(ByteReader *r)
{
    if (!br_need(r, 4)) return 0;
    const u1 *p = r->data + r->pos;
    r->pos += 4;
    return ((u4)p[0] << 24) | ((u4)p[1] << 16) | ((u4)p[2] << 8) | (u4)p[3];
}

const u1 *br_bytes(ByteReader *r, size_t n)
{
    if (!br_need(r, n)) return NULL;
    const u1 *p = r->data + r->pos;
    r->pos += n;
    return p;
}

size_t br_remaining(const ByteReader *r)
{
    return r->size - r->pos;
}

int br_ok(const ByteReader *r)
{
    return !r->error;
}

u1 *read_whole_file(const char *path, size_t *out_size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;

    u1 *buf = NULL;
    long size;
    if (fseek(f, 0, SEEK_END) != 0 || (size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0)
        goto fail;

    buf = malloc(size > 0 ? (size_t)size : 1);
    if (!buf) goto fail;
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) goto fail;

    fclose(f);
    *out_size = (size_t)size;
    return buf;

fail:
    free(buf);
    fclose(f);
    return NULL;
}
