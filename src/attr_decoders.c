/**
 * @file attr_decoders.c
 * @brief Decodificadores dos demais atributos (JVMS §4.7).
 *
 * Todas as funções seguem o contrato descrito em attributes_internal.h:
 *   - o leitor já está no início do campo info do atributo;
 *   - índices do pool são exibidos com print_cp_ref (índice + valor resolvido);
 *   - retornam 1 (se o leitor marcar erro, o chamador avisa que o atributo
 *     está truncado).
 *
 * Atributos sem decodificador (ex.: RuntimeVisibleTypeAnnotations, Module)
 * aparecem como dump hexadecimal, então a saída nunca quebra.
 */
#include "attributes.h"
#include "attributes_internal.h"
#include "access_flags.h"
#include "constant_pool.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

/* Concatena s em buf sem estourar o tamanho n. */
static void append(char *buf, size_t n, const char *s)
{
    size_t len = strlen(buf);
    if (len + 1 < n) snprintf(buf + len, n - len, "%s", s);
}

/* Linha "Label: N" sem referência ao pool. */
static void print_count(int depth, const char *label, unsigned value)
{
    print_indent(depth);
    printf("%-*s %u\n", LABEL_WIDTH, label, value);
}

/* Como print_cp_ref, mas o índice 0 significa "nenhum" (campos opcionais). */
static void print_optional_cp_ref(const ClassFile *cf, const char *label, u2 idx,
                                  const char *none, int depth)
{
    if (idx == 0) {
        print_indent(depth);
        printf("%-*s cp #0 <%s>\n", LABEL_WIDTH, label, none);
    } else {
        print_cp_ref(cf, label, idx, depth);
    }
}

/* ---------------------------------------------------------------------- */
/* Atributos simples (um índice para o pool)                              */
/* ---------------------------------------------------------------------- */

/* ConstantValue (§4.7.2): u2 constantvalue_index */
int decode_constant_value(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 idx = br_u2(r);
    if (br_ok(r)) print_cp_ref(cf, "Constant value index:", idx, depth);
    return 1;
}

/* Signature (§4.7.9): u2 signature_index */
int decode_signature(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 idx = br_u2(r);
    if (br_ok(r)) print_cp_ref(cf, "Signature index:", idx, depth);
    return 1;
}

/* NestHost (§4.7.28): u2 host_class_index */
int decode_nest_host(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 idx = br_u2(r);
    if (br_ok(r)) print_cp_ref(cf, "Host class:", idx, depth);
    return 1;
}

/* EnclosingMethod (§4.7.7): u2 class_index; u2 method_index (0 = nenhum) */
int decode_enclosing_method(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 class_index = br_u2(r);
    u2 method_index = br_u2(r);
    if (!br_ok(r)) return 1;
    print_cp_ref(cf, "Class index:", class_index, depth);
    print_optional_cp_ref(cf, "Method index:", method_index,
                          "nenhum (inicializador de campo/classe)", depth);
    return 1;
}

/*
 * Deprecated (§4.7.15) e Synthetic (§4.7.8) não têm conteúdo (length == 0).
 * Basta retornar 1: o nome do atributo já foi exibido.
 */
int decode_empty(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Listas de classes                                                      */
/* ---------------------------------------------------------------------- */

/* Formato comum: u2 número; u2 índices[número] (todos CONSTANT_Class). */
static void decode_class_list(const ClassFile *cf, ByteReader *r, int depth,
                              const char *count_label, const char *item_label)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return;
    print_count(depth, count_label, n);
    for (u2 i = 0; i < n; i++) {
        u2 idx = br_u2(r);
        if (!br_ok(r)) return;
        char label[48];
        snprintf(label, sizeof label, "%s [%u]:", item_label, i);
        print_cp_ref(cf, label, idx, depth + 1);
    }
}

/* Exceptions (§4.7.5): u2 number_of_exceptions; u2 exception_index_table[n] */
int decode_exceptions(const ClassFile *cf, ByteReader *r, int depth)
{
    decode_class_list(cf, r, depth, "Number of exceptions:", "Exception");
    return 1;
}

/* NestMembers (§4.7.29): u2 number_of_classes; u2 classes[n] */
int decode_nest_members(const ClassFile *cf, ByteReader *r, int depth)
{
    decode_class_list(cf, r, depth, "Number of classes:", "Member");
    return 1;
}

/* PermittedSubclasses (§4.7.31): mesmo formato do NestMembers */
int decode_permitted_subclasses(const ClassFile *cf, ByteReader *r, int depth)
{
    decode_class_list(cf, r, depth, "Number of classes:", "Subclass");
    return 1;
}

/*
 * InnerClasses (§4.7.6): u2 number_of_classes; n x {
 *   u2 inner_class_info_index; u2 outer_class_info_index (0 = nenhum);
 *   u2 inner_name_index (0 = anônima); u2 inner_class_access_flags }
 */
int decode_inner_classes(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Number of classes:", n);

    for (u2 i = 0; i < n; i++) {
        u2 inner = br_u2(r), outer = br_u2(r), name = br_u2(r), flags = br_u2(r);
        if (!br_ok(r)) return 1;

        char buf[256];
        flags_to_string(flags, FLAGS_INNER_CLASS, buf, sizeof buf);
        print_indent(depth);
        printf("[%u]\n", i);
        print_cp_ref(cf, "Inner class:", inner, depth + 1);
        print_optional_cp_ref(cf, "Outer class:", outer, "nenhuma (local ou anônima)", depth + 1);
        print_optional_cp_ref(cf, "Inner name:", name, "anônima", depth + 1);
        print_indent(depth + 1);
        printf("%-*s 0x%04X [%s]\n", LABEL_WIDTH, "Access flags:", flags, buf);
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Tabelas de depuração (aninhadas no Code)                               */
/* ---------------------------------------------------------------------- */

/*
 * LineNumberTable (§4.7.12): u2 length; length x { u2 start_pc; u2 line_number }
 *   Nr.  Start PC  Line number
 */
int decode_line_number_table(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf;
    u2 n = br_u2(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Line number table length:", n);
    if (n > 0) {
        print_indent(depth + 1);
        printf("%-4s %-9s %s\n", "Nr.", "Start PC", "Line number");
    }
    for (u2 i = 0; i < n; i++) {
        u2 start_pc = br_u2(r), line = br_u2(r);
        if (!br_ok(r)) return 1;
        print_indent(depth + 1);
        printf("%-4u %-9u %u\n", i, start_pc, line);
    }
    return 1;
}

/*
 * LocalVariableTable (§4.7.13) e LocalVariableTypeTable (§4.7.14), mesmo formato:
 *   u2 length; length x { u2 start_pc; u2 length; u2 name_index;
 *                         u2 descriptor_index (ou signature_index); u2 index }
 * Só muda o significado da 4ª coluna: descritor ou assinatura genérica.
 */
static void decode_local_vars(const ClassFile *cf, ByteReader *r, int depth, const char *type_column)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return;
    print_count(depth, "Local variable table length:", n);
    if (n > 0) {
        print_indent(depth + 1);
        printf("%-4s %-9s %-7s %-6s %-26s %s\n", "Nr.", "Start PC", "Length", "Index", "Name", type_column);
    }
    for (u2 i = 0; i < n; i++) {
        u2 start_pc = br_u2(r), length = br_u2(r);
        u2 name = br_u2(r), type = br_u2(r), index = br_u2(r);
        if (!br_ok(r)) return;

        char name_col[64];
        snprintf(name_col, sizeof name_col, "cp #%u <%s>", name, cp_utf8(cf, name));
        print_indent(depth + 1);
        printf("%-4u %-9u %-7u %-6u %-26s cp #%u <%s>\n", i, start_pc, length, index,
               name_col, type, cp_utf8(cf, type));
    }
}

int decode_local_variable_table(const ClassFile *cf, ByteReader *r, int depth)
{
    decode_local_vars(cf, r, depth, "Descriptor");
    return 1;
}

int decode_local_variable_type_table(const ClassFile *cf, ByteReader *r, int depth)
{
    decode_local_vars(cf, r, depth, "Signature");
    return 1;
}

/* ---------------------------------------------------------------------- */
/* StackMapTable (§4.7.4)                                                 */
/* ---------------------------------------------------------------------- */

/*
 * verification_type_info: u1 tag e, para Object/Uninitialized, um u2.
 * Escreve em buf a forma legível (ex.: "int", "cp #5 <java/lang/String>").
 */
static void read_verification_type(const ClassFile *cf, ByteReader *r, char *buf, size_t n)
{
    static const char *names[] = {
        "top", "int", "float", "double", "long", "null", "uninitializedThis"
    };
    u1 tag = br_u1(r);
    if (!br_ok(r)) {
        snprintf(buf, n, "<truncado>");
    } else if (tag <= 6) {
        snprintf(buf, n, "%s", names[tag]);
    } else if (tag == 7) {
        u2 idx = br_u2(r);
        snprintf(buf, n, "cp #%u <%s>", idx, cp_class_name(cf, idx));
    } else if (tag == 8) {
        snprintf(buf, n, "uninitialized(%u)", br_u2(r));
    } else {
        snprintf(buf, n, "<tag %u inválida>", tag);
    }
}

/* Imprime "label: [t1, t2, ...]" lendo `count` verification_type_info. */
static void print_verification_types(const ClassFile *cf, ByteReader *r, int depth,
                                     const char *label, unsigned count)
{
    print_indent(depth);
    printf("%s [", label);
    for (unsigned i = 0; i < count && br_ok(r); i++) {
        char buf[300];
        read_verification_type(cf, r, buf, sizeof buf);
        printf("%s%s", i > 0 ? ", " : "", buf);
    }
    printf("]\n");
}

/*
 * Cada frame guarda offset_delta; o pc real é offset_delta para o primeiro
 * frame e pc_anterior + offset_delta + 1 para os seguintes (§4.7.4).
 */
int decode_stack_map_table(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Number of entries:", n);

    long pc = -1;
    for (u2 i = 0; i < n; i++) {
        u1 type = br_u1(r);
        if (!br_ok(r)) return 1;

        const char *kind;
        u2 delta;
        if (type <= 63) {
            kind = "same";
            delta = type;
        } else if (type <= 127) {
            kind = "same_locals_1_stack_item";
            delta = (u2)(type - 64);
        } else if (type <= 246) {
            print_indent(depth);
            printf("[%u] frame_type %u reservado -- tabela interrompida\n", i, type);
            r->error = 1;
            return 1;
        } else if (type == 247) {
            kind = "same_locals_1_stack_item_extended";
            delta = br_u2(r);
        } else if (type <= 250) {
            kind = "chop";
            delta = br_u2(r);
        } else if (type == 251) {
            kind = "same_extended";
            delta = br_u2(r);
        } else if (type <= 254) {
            kind = "append";
            delta = br_u2(r);
        } else {
            kind = "full";
            delta = br_u2(r);
        }
        if (!br_ok(r)) return 1;

        pc = (pc < 0) ? delta : pc + delta + 1;
        print_indent(depth);
        printf("[%u] %s_frame (frame_type %u), offset_delta %u -> pc %ld\n", i, kind, type, delta, pc);

        if ((type >= 64 && type <= 127) || type == 247) {
            print_verification_types(cf, r, depth + 2, "Stack:", 1);
        } else if (type >= 248 && type <= 250) {
            print_indent(depth + 2);
            printf("Chopped locals: %u\n", 251u - type);
        } else if (type >= 252 && type <= 254) {
            print_verification_types(cf, r, depth + 2, "Appended locals:", type - 251u);
        } else if (type == 255) {
            u2 nlocals = br_u2(r);
            print_verification_types(cf, r, depth + 2, "Locals:", nlocals);
            u2 nstack = br_u2(r);
            print_verification_types(cf, r, depth + 2, "Stack:", nstack);
        }
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* BootstrapMethods (§4.7.23)                                             */
/* ---------------------------------------------------------------------- */

/*
 * u2 num_bootstrap_methods; n x {
 *   u2 bootstrap_method_ref; u2 num_bootstrap_arguments; u2 args[num] }
 * Aparece em classes que usam lambdas / concatenação de strings (Java 9+).
 */
int decode_bootstrap_methods(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Number of methods:", n);

    for (u2 i = 0; i < n; i++) {
        u2 ref = br_u2(r), nargs = br_u2(r);
        if (!br_ok(r)) return 1;
        print_indent(depth);
        printf("[%u]\n", i);
        print_cp_ref(cf, "Bootstrap method:", ref, depth + 1);
        print_count(depth + 1, "Number of arguments:", nargs);
        for (u2 k = 0; k < nargs; k++) {
            u2 arg = br_u2(r);
            if (!br_ok(r)) return 1;
            char label[32];
            snprintf(label, sizeof label, "Argument [%u]:", k);
            print_cp_ref(cf, label, arg, depth + 2);
        }
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* MethodParameters (§4.7.24)                                             */
/* ---------------------------------------------------------------------- */

/* u1 parameters_count; n x { u2 name_index (0 = sem nome); u2 access_flags } */
int decode_method_parameters(const ClassFile *cf, ByteReader *r, int depth)
{
    u1 n = br_u1(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Parameters count:", n);
    for (u1 i = 0; i < n; i++) {
        u2 name = br_u2(r), flags = br_u2(r);
        if (!br_ok(r)) return 1;
        char label[32];
        snprintf(label, sizeof label, "[%u] Name:", i);
        print_optional_cp_ref(cf, label, name, "sem nome", depth);
        /* Flags válidas aqui: final 0x0010, synthetic 0x1000, mandated 0x8000. */
        char names[48] = "";
        if (flags & 0x0010) append(names, sizeof names, "final");
        if (flags & 0x1000) append(names, sizeof names, names[0] ? " synthetic" : "synthetic");
        if (flags & 0x8000) append(names, sizeof names, names[0] ? " mandated" : "mandated");
        print_indent(depth + 2);
        printf("%-*s 0x%04X [%s]\n", LABEL_WIDTH - 4, "Access flags:", flags, names);
    }
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Anotações (§4.7.16 - §4.7.22)                                          */
/* ---------------------------------------------------------------------- */

/* Limite de aninhamento: protege contra arquivos malformados/maliciosos. */
#define MAX_ANNOTATION_DEPTH 16

static void format_annotation(const ClassFile *cf, ByteReader *r, char *buf, size_t n, int level);

/*
 * element_value (§4.7.16.1), escrito numa linha no estilo do Java:
 *   42, "texto", Lpkg/Cor;.VERDE, class Ljava/lang/String;, @Lpkg/A;(...), {1, 2}
 */
static void format_element_value(const ClassFile *cf, ByteReader *r, char *buf, size_t n, int level)
{
    char tmp[1024];
    u1 tag = br_u1(r);
    if (!br_ok(r) || level > MAX_ANNOTATION_DEPTH) {
        append(buf, n, "<inválido>");
        r->error = 1;
        return;
    }

    switch (tag) {
    case 'B': case 'C': case 'D': case 'F': case 'I':
    case 'J': case 'S': case 'Z': case 's': {
        u2 idx = br_u2(r);
        cp_describe(cf, idx, tmp, sizeof tmp);
        if (tag == 's') {
            append(buf, n, "\"");
            append(buf, n, tmp);
            append(buf, n, "\"");
        } else if (tag == 'Z') {
            const cp_info *e = cp_get(cf, idx, CONSTANT_Integer);
            append(buf, n, e ? (cp_int_value(e) ? "true" : "false") : tmp);
        } else if (tag == 'C') {
            const cp_info *e = cp_get(cf, idx, CONSTANT_Integer);
            int32_t c = e ? cp_int_value(e) : 0;
            if (e && c >= 0x20 && c < 0x7F) snprintf(tmp, sizeof tmp, "'%c'", (char)c);
            append(buf, n, tmp);
        } else {
            append(buf, n, tmp);
        }
        break;
    }
    case 'e': {
        u2 type = br_u2(r), name = br_u2(r);
        snprintf(tmp, sizeof tmp, "%s.%s", cp_utf8(cf, type), cp_utf8(cf, name));
        append(buf, n, tmp);
        break;
    }
    case 'c': {
        u2 idx = br_u2(r);
        snprintf(tmp, sizeof tmp, "class %s", cp_utf8(cf, idx));
        append(buf, n, tmp);
        break;
    }
    case '@':
        format_annotation(cf, r, buf, n, level + 1);
        break;
    case '[': {
        u2 count = br_u2(r);
        append(buf, n, "{");
        for (u2 i = 0; i < count && br_ok(r); i++) {
            if (i > 0) append(buf, n, ", ");
            format_element_value(cf, r, buf, n, level + 1);
        }
        append(buf, n, "}");
        break;
    }
    default:
        snprintf(tmp, sizeof tmp, "<tag '%c' inválida>", tag);
        append(buf, n, tmp);
        r->error = 1;
    }
}

/* annotation: u2 type_index; u2 num_pairs; pairs x { u2 name_index; element_value } */
static void format_annotation(const ClassFile *cf, ByteReader *r, char *buf, size_t n, int level)
{
    u2 type = br_u2(r), pairs = br_u2(r);
    if (!br_ok(r)) return;
    append(buf, n, "@");
    append(buf, n, cp_utf8(cf, type));
    if (pairs == 0) return;
    append(buf, n, "(");
    for (u2 i = 0; i < pairs && br_ok(r); i++) {
        if (i > 0) append(buf, n, ", ");
        append(buf, n, cp_utf8(cf, br_u2(r)));
        append(buf, n, "=");
        format_element_value(cf, r, buf, n, level + 1);
    }
    append(buf, n, ")");
}

/* Lê `u2 num_annotations; annotation[]` e imprime uma anotação por linha. */
static void print_annotation_list(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return;
    print_count(depth, "Number of annotations:", n);
    for (u2 i = 0; i < n && br_ok(r); i++) {
        char buf[2048] = "";
        format_annotation(cf, r, buf, sizeof buf, 0);
        print_indent(depth + 1);
        printf("[%u] %s\n", i, buf);
    }
}

/* RuntimeVisibleAnnotations (§4.7.16) e RuntimeInvisibleAnnotations (§4.7.17) */
int decode_annotations(const ClassFile *cf, ByteReader *r, int depth)
{
    print_annotation_list(cf, r, depth);
    return 1;
}

/* Runtime[In]VisibleParameterAnnotations (§4.7.18, §4.7.19):
 * u1 num_parameters; uma lista de anotações por parâmetro. */
int decode_parameter_annotations(const ClassFile *cf, ByteReader *r, int depth)
{
    u1 n = br_u1(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Number of parameters:", n);
    for (u1 i = 0; i < n && br_ok(r); i++) {
        print_indent(depth);
        printf("Parameter [%u]\n", i);
        print_annotation_list(cf, r, depth + 1);
    }
    return 1;
}

/* AnnotationDefault (§4.7.22): um element_value */
int decode_annotation_default(const ClassFile *cf, ByteReader *r, int depth)
{
    char buf[2048] = "";
    format_element_value(cf, r, buf, sizeof buf, 0);
    print_indent(depth);
    printf("%-*s %s\n", LABEL_WIDTH, "Default value:", buf);
    return 1;
}

/* ---------------------------------------------------------------------- */
/* Outros                                                                 */
/* ---------------------------------------------------------------------- */

/* SourceDebugExtension (§4.7.11): o atributo inteiro é uma string Modified UTF-8. */
int decode_source_debug_extension(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf;
    size_t len = br_remaining(r);
    const u1 *bytes = br_bytes(r, len);
    char buf[2048];
    mutf8_format(bytes, len, buf, sizeof buf);
    print_indent(depth);
    printf("%-*s %s\n", LABEL_WIDTH, "Debug extension:", buf);
    return 1;
}

/*
 * Record (§4.7.30): u2 components_count; n x {
 *   u2 name_index; u2 descriptor_index; u2 attributes_count; attributes[] }
 * Os atributos de cada componente (ex.: Signature) são exibidos recursivamente.
 */
int decode_record(const ClassFile *cf, ByteReader *r, int depth)
{
    u2 n = br_u2(r);
    if (!br_ok(r)) return 1;
    print_count(depth, "Components count:", n);
    for (u2 i = 0; i < n; i++) {
        u2 name = br_u2(r), desc = br_u2(r), nattrs = br_u2(r);
        if (!br_ok(r)) return 1;
        print_indent(depth);
        printf("[%u] %s\n", i, cp_utf8(cf, name));
        print_cp_ref(cf, "Name:", name, depth + 1);
        print_cp_ref(cf, "Descriptor:", desc, depth + 1);
        print_count(depth + 1, "Attributes count:", nattrs);
        for (u2 k = 0; k < nattrs; k++) {
            u2 attr_name = br_u2(r);
            u4 len = br_u4(r);
            const u1 *info = br_bytes(r, len);
            if (!br_ok(r)) return 1;
            print_attribute_raw(cf, k, attr_name, info, len, depth + 1);
        }
    }
    return 1;
}
