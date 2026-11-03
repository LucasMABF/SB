/**
 * @file attr_decoders.c
 * @brief Decodificadores dos demais atributos (JVMS §4.7).
 *
 * TODO(Pessoa C): implementar as funções abaixo. Todas seguem o contrato
 * descrito em attributes_internal.h. Use como modelo decode_source_file()
 * e decode_code() em attributes.c:
 *   - leia os campos com br_u1/br_u2/br_u4 (o leitor já está no início do info);
 *   - mostre índices do pool com print_cp_ref(cf, "Rótulo:", idx, depth);
 *   - retorne 1 quando implementado.
 *
 * Enquanto uma função retornar 0, o atributo aparece como dump hexadecimal,
 * então a saída nunca quebra. Ordem sugerida (do mais comum nos exemplos
 * para o menos comum): LineNumberTable, LocalVariableTable, ConstantValue,
 * Exceptions, InnerClasses, Signature, StackMapTable, BootstrapMethods...
 */
#include "attributes_internal.h"
#include "access_flags.h"
#include "constant_pool.h"
#include "util.h"

#include <stdio.h>

/*
 * ConstantValue (§4.7.2): u2 constantvalue_index
 * Exibir: "Constant value index:" cp #N <valor>
 */
int decode_constant_value(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/*
 * Exceptions (§4.7.5): u2 number_of_exceptions; u2 exception_index_table[n]
 * Exibir uma linha por exceção: "Exception:" cp #N <java/io/IOException>
 */
int decode_exceptions(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/*
 * LineNumberTable (§4.7.12): u2 length; length x { u2 start_pc; u2 line_number }
 * Exibir como tabela:  Nr.  Start PC  Line number
 */
int decode_line_number_table(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/*
 * LocalVariableTable (§4.7.13) e LocalVariableTypeTable (§4.7.14), mesmo formato:
 *   u2 length; length x { u2 start_pc; u2 length; u2 name_index;
 *                         u2 descriptor_index (ou signature_index); u2 index }
 * Exibir como tabela: Nr. Start PC  Length  Index  Name  Descriptor
 * (name/descriptor resolvidos com cp_utf8).
 */
int decode_local_variable_table(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/*
 * InnerClasses (§4.7.6): u2 number_of_classes; n x {
 *   u2 inner_class_info_index; u2 outer_class_info_index (0 = nenhum);
 *   u2 inner_name_index (0 = anônima); u2 inner_class_access_flags }
 * Flags: use flags_to_string(..., FLAGS_INNER_CLASS, ...).
 */
int decode_inner_classes(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/* Signature (§4.7.9): u2 signature_index */
int decode_signature(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/*
 * Deprecated (§4.7.15) e Synthetic (§4.7.8) não têm conteúdo (length == 0).
 * Basta retornar 1 (talvez imprimir nada). Já implementado.
 */
int decode_empty(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 1;
}

/*
 * StackMapTable (§4.7.4): u2 number_of_entries; entries[] de stack_map_frame.
 * É o mais trabalhoso (frames same, same_locals_1_stack_item, chop, append,
 * full_frame, com verification_type_info). Se faltar tempo, deixar o dump
 * hexadecimal é aceitável: mencionar no vídeo como "não implementado".
 */
int decode_stack_map_table(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/*
 * BootstrapMethods (§4.7.23): u2 num_bootstrap_methods; n x {
 *   u2 bootstrap_method_ref; u2 num_bootstrap_arguments; u2 args[num] }
 * Aparece em classes que usam lambdas / concatenação de strings (Java 9+).
 */
int decode_bootstrap_methods(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}

/* EnclosingMethod (§4.7.7): u2 class_index; u2 method_index (0 = nenhum) */
int decode_enclosing_method(const ClassFile *cf, ByteReader *r, int depth)
{
    (void)cf; (void)r; (void)depth;
    return 0;
}
