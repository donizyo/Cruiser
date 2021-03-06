#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "java.h"
#include "log.h"
#include "opcode.h"
#include "memory.h"

static int
loadAttribute(struct BufferIO *input, attr_info *info)
{
    if (ru4(&(info->attribute_length), input) < 0)
        goto error;
    return 0;
error:
    logError("Vital error: fail to initialize attribute!\r\n");
    return -1;
}

static int
skipAttribute(struct BufferIO *input,
        int length,
        char *name,
        attr_info *info)
{
    logError("Skip attribute{name_index: %.*s, length: %i}!\r\n",
            length, name,
            info->attribute_length);
    return skp(input, info->attribute_length);
}

#if VER_CMP(45, 3)
static int
loadAttribute_ConstantValue(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_ConstantValue_info *data;

    info->tag = TAG_ATTR_CONSTANTVALUE;
    data = (attr_ConstantValue_info *)
            malloc(sizeof (attr_ConstantValue_info));
    if (!data)
        return -1;
    if (ru2(&(data->constantvalue_index), input) < 0)
        return -1;

    info->data = data;
    return 0;
}

static int
freeAttribute_ConstantValue(attr_info *info)
{
    if (info->tag != TAG_ATTR_CONSTANTVALUE)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_Code(ClassFile *cf,
        struct BufferIO *input,
        method_info *method,
        attr_info *info)
{
    attr_Code_info *data;
    u2 i;

    info->tag = TAG_ATTR_CODE;
    data = (attr_Code_info *)
            malloc(sizeof (attr_Code_info));
    if (!data)
        return -1;
    if (ru2(&(data->max_stack), input) < 0)
        return -1;
    if (ru2(&(data->max_locals), input) < 0)
        return -1;
    if (ru4(&(data->code_length), input) < 0)
        return -1;
    if (data->code_length <= 0)
    {
        logError("Assertion error: data->code_length <= 0!\r\n");
        return -1;
    }
    if (data->code_length >= 65536)
    {
        logError("Assertion error: data->code_length >= 65536!\r\n");
        return -1;
    }
    data->code = (u1 *) allocMemory(data->code_length, sizeof (u1));
    if (!data->code) return -1;
    if (rbs(data->code, input, data->code_length) < 0)
        return -1;
    
    //if (disassembleCode(data->code_length, data->code) < 0) return -1;
    
    if (ru2(&(data->exception_table_length), input) < 0)
        return -1;
    if (data->exception_table_length > 0)
    {
        data->exception_table = (struct exception_table_entry *)
            allocMemory(data->exception_table_length,
                sizeof (struct exception_table_entry));
        if (!data->exception_table) return -1;
        for (i = 0u; i < data->exception_table_length; i++)
        {
            if (ru2(&(data->exception_table[i].start_pc), input) < 0)
                return -1;
            if (ru2(&(data->exception_table[i].end_pc), input) < 0)
                return -1;
            if (ru2(&(data->exception_table[i].handler_pc), input) < 0)
                return -1;
            if (ru2(&(data->exception_table[i].catch_type), input) < 0)
                return -1;
            /*
             * If the value of the catch_type item is zero,
             * this exception handler is called for all exceptions.
             * This is used to implement finally (§3.13).
             */
        }
    }
    else if (data->exception_table_length == 0)
    {
        data->exception_table = (struct exception_table_entry *) 0;
    }
    else
    {
        logError("Assertion error: Exception table length is negative!\r\n");
        return -1;
    }
    loadAttributes_code(cf, input, data,
            &(data->attributes_count),
            &(data->attributes));

    info->data = data;
    return 0;
}

static int
freeAttribute_Code(ClassFile * cf, attr_info *info)
{
    attr_Code_info *data;

    if (info->tag != TAG_ATTR_CODE)
        return -1;
    data = (attr_Code_info *) info->data;
    free(data->code);
    data->code = (u1 *) 0;
    free(data->exception_table);
    data->exception_table = 0;
    freeAttributes_code(cf, data->attributes_count, data->attributes);
    free(data->attributes);
    data->attributes = (attr_info *) 0;
    free(data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_Exceptions(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_Exceptions_info *data;
    const_Class_data *cc;
    const_Utf8_data *utf8;
    u2 i;

    info->tag = TAG_ATTR_EXCEPTIONS;
    data = (attr_Exceptions_info *)
            malloc(sizeof (attr_Exceptions_info));
    if (!data)
    {
        logError("Fail to allocate memory!\r\n");
        return -1;
    }
    bzero(data, sizeof (attr_Exceptions_info));
    if (ru2(&(data->number_of_exceptions), input) < 0)
        return -1;
    // Validate Exception attribute
    if ((data->number_of_exceptions + 1) * sizeof (u2)
            != info->attribute_length)
    {
        logError("Exception attribute is not valid!\r\n");
    }
    data->exception_index_table = (u2 *)
        malloc(sizeof (u2) * data->number_of_exceptions);
    if (!data->exception_index_table)
    {
        logError("Fail to allocate memory!\r\n");
        return -1;
    }
    for (i = 0u; i < data->number_of_exceptions; i++)
        if (ru2(&(data->exception_index_table[i]), input) < 0)
            return -1;

    info->data = data;
    return 0;
}

static int
freeAttribute_Exceptions(attr_info *info)
{
    attr_Exceptions_info *data;

    if (info->tag != TAG_ATTR_EXCEPTIONS)
        return -1;
    data = (attr_Exceptions_info *) info->data;
    free(data->exception_index_table);
    free(data);
    info->data = (void *) 0;

    return 0;
}

// If the constant pool of a class or interface C
// contains a const_Class_data entry which
// represents a class or interface that
// is not a member of a package, then C 's ClassFile structure
// must have exactly one InnerClasses attribute in its attributes table.
static int
loadAttribute_InnerClasses(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_InnerClasses_info *data;
    const_Class_data *cc;
    const_Utf8_data *cu;
    u2 i;

    info->tag = TAG_ATTR_INNERCLASSES;
    data = (attr_InnerClasses_info *)
            malloc(sizeof (attr_InnerClasses_info));
    if (!data)
        return -1;
    if (ru2(&(data->number_of_classes), input) < 0)
        return -1;
    data->classes = (struct classes_entry *)
            allocMemory(data->number_of_classes,
                sizeof (struct classes_entry));
    if (!data->classes) return -1;
    for (i = 0u; i < data->number_of_classes; i++)
    {
        if (ru2(&(data->classes[i].inner_class_info_index), input) < 0)
            return -1;
        cc = getConstant_Class(cf, data->classes[i].inner_class_info_index);
        if (!cc)
        {
            logError("Assertion error: constant_pool[%i] is not const_Class_data instance!\r\n", data->classes[i].inner_class_info_index);
            return -1;
        }
        if (ru2(&(data->classes[i].outer_class_info_index), input) < 0)
            return -1;
        /*
         * If C is not a member of a class or an interface
         * (that is, if C is a top-level class or interface (JLS §7.6)
         * or a local class (JLS §14.3) or an anonymous class (JLS §15.9.5)),
         * the value of the outer_class_info_index item must be zero.
         * Otherwise, the value of the outer_class_info_index item must be a
         * valid index into the constant_pool table, and the entry
         * at that index must be a const_Class_data (§4.4.1)
         * structure representing the class or interface of which C is a member.
         */
        if (data->classes[i].outer_class_info_index != 0)
        {
            cc = getConstant_Class(cf, data->classes[i].outer_class_info_index);
            if (!cc)
            {
                logError("Assertion error: constant_pool[%i] is not const_Class_data instance!\r\n", data->classes[i].outer_class_info_index);
                return -1;
            }
        }
        if (ru2(&(data->classes[i].inner_name_index), input) < 0)
            return -1;
        /*
         * If C is anonymous (JLS §15.9.5), the value of
         * the inner_name_index item must be zero.
         */
        if (data->classes[i].inner_name_index != 0) // not anonymous
        {
            cu = getConstant_Utf8(cf, data->classes[i].inner_name_index);
            if (!cu)
            {
                logError("Assertion error: constant_pool[%i] is not const_Class_data instance!\r\n", data->classes[i].inner_name_index);
                return -1;
            }
        }
        if (ru2(&(data->classes[i].inner_class_access_flags), input) < 0)
            return -1;
        if (data->classes[i].inner_class_access_flags & ~ACC_NESTED_CLASS)
        {
            logError("Assertion error: data->classes[%i] has unknown inner_class_access_flags: 0x%X!\r\n",
                    i, data->classes[i].inner_class_access_flags & ~ACC_NESTED_CLASS);
            /*
             * All bits of the inner_class_access_flags item not assigned in Table 4.8
             * are reserved for future use. They should be set to zero in generated class
             * files and should be ignored by Java Virtual Machine implementations.
             */
            //return -1;
        }
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_InnerClasses(attr_info *info)
{
    attr_InnerClasses_info *data;

    if (info->tag != TAG_ATTR_INNERCLASSES)
        return -1;
    data = (attr_InnerClasses_info *) info->data;
    free(data->classes);
    data->classes = 0;
    free(data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_Synthetic(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    if (info->attribute_length != 0)
    {
        logError("Assertion error: info->attribute_length != 0!\r\n");
        return -1;
    }
    info->tag = TAG_ATTR_SYNTHETIC;
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_SourceFile(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_SourceFile_info *data;
    const_Utf8_data *cu;

    info->tag = TAG_ATTR_SOURCEFILE;
    data = (attr_SourceFile_info *)
            malloc(sizeof (attr_SourceFile_info));
    if (!data)
        return -1;
    if (ru2(&(data->sourcefile_index), input) < 0)
        return -1;
    cu = getConstant_Utf8(cf, data->sourcefile_index);
    if (!cu)
    {
        logError("Assertion error: constant_pool[%i] is not const_Utf8_data instance!\r\n", data->sourcefile_index);
        return -1;
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_SourceFile(attr_info *info)
{
    if (info->tag != TAG_ATTR_SOURCEFILE)
        return -1;
    free(info->data);
    info->data = 0;

    return 0;
}

static int
loadAttribute_SourceDebugExtension(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    int cap;

    info->tag = TAG_ATTR_SOURCEDEBUGEXTENSION;
    cap = sizeof (u1) * info->attribute_length;
    info->data = (u1 *) malloc(info->attribute_length);
    if (!info->data)
        return -1;
    if (rbs((u1 *) info->data, input, cap) < 0)
        return -1;

    return 0;
}

static int
freeAttribute_SourceDebugExtension(attr_info *info)
{
    if (info->tag != TAG_ATTR_SOURCEDEBUGEXTENSION)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_LineNumberTable(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_LineNumberTable_info *data;
    u2 i, lntl;
    int cap;

    info->tag = TAG_ATTR_LINENUMBERTABLE;
    if (ru2(&lntl, input) < 0)
        return -1;
    cap = sizeof (attr_LineNumberTable_info)
        + sizeof (struct line_number_table_entry) * lntl;
    data = (attr_LineNumberTable_info *) malloc(cap);
    if (!data)
        return -1;
    data->line_number_table_length = lntl;
    for (i = 0; i < lntl; i++)
    {
        if (ru2(&(data->line_number_table[i].start_pc), input) < 0)
            return -1;
        if (ru2(&(data->line_number_table[i].line_number), input) < 0)
            return -1;
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_LineNumberTable(attr_info *info)
{
    if (info->tag != TAG_ATTR_LINENUMBERTABLE)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_LocalVariableTable(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_LocalVariableTable_info *data;
    u2 i, lvtl;
    int cap;
    const_Utf8_data *cu;

    info->tag = TAG_ATTR_LOCALVARIABLETABLE;
    if (ru2(&lvtl, input) < 0)
        return -1;
    cap = sizeof (attr_LocalVariableTable_info)
        + sizeof (struct local_variable_table_entry) * lvtl;
    data = (attr_LocalVariableTable_info *) malloc(cap);
    if (!data)
        return -1;
    data->local_variable_table_length = lvtl;
    for (i = 0u; i < lvtl; i++)
    {
        if (ru2(&(data->local_variable_table[i].start_pc), input) < 0)
            return -1;
        if (ru2(&(data->local_variable_table[i].length), input) < 0)
            return -1;
        if (ru2(&(data->local_variable_table[i].name_index), input) < 0)
            return -1;
        cu = getConstant_Utf8(cf, data->local_variable_table[i].name_index);
        if (!cu)
        {
            logError("Assertion error: constant_pool[%i] is not const_Utf8_data instance!\r\n");
            return -1;
        }
        if (ru2(&(data->local_variable_table[i].descriptor_index), input) < 0)
            return -1;
        cu = getConstant_Utf8(cf, data->local_variable_table[i].descriptor_index);
        if (!cu)
        {
            logError("Assertion error: constant_pool[%i] is not const_Utf8_data instance!\r\n");
            return -1;
        }
        if (ru2(&(data->local_variable_table[i].index), input) < 0)
            return -1;
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_LocalVariableTable(attr_info *info)
{
    if (info->tag != TAG_ATTR_LOCALVARIABLETABLE)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_Deprecated(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    if (info->attribute_length != 0)
    {
        logError("Assertion error: info->attribute_length != 0!\r\n");
        return -1;
    }
    info->tag = TAG_ATTR_DEPRECATED;
    info->data = (void *) 0;

    return 0;
}
#endif /* VERSION 45.3 */
#if VER_CMP(49, 0)
static int
loadAttribute_EnclosingMethod(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_EnclosingMethod_info *data;
    const_Class_data *cc;
    const_NameAndType_data *cn;

    info->tag = TAG_ATTR_ENCLOSINGMETHOD;
    data = (attr_EnclosingMethod_info *)
            malloc(sizeof (attr_EnclosingMethod_info));
    if (!data)
        return -1;
    if (ru2(&(data->class_index), input) < 0)
        return -1;
    cc = getConstant_Class(cf, data->class_index);
    if (!cc)
    {
        logError("Assertion error: constant_pool[%i] is not const_Class_data instance!\r\n", data->class_index);
        return -1;
    }
    if (ru2(&(data->method_index), input) < 0)
        return -1;
    if (data->method_index != 0)
    {
        cn = getConstant_NameAndType(cf, data->method_index);
        if (!cn)
        {
            logError("Assertion error: constant_pool[%i] is not const_NameAndType_data!\r\n", data->method_index);
            return -1;
        }
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_EnclosingMethod(attr_info *info)
{
    if (info->tag != TAG_ATTR_ENCLOSINGMETHOD)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_Signature(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_Signature_info *data;
    const_Utf8_data *cu;

    info->tag = TAG_ATTR_SIGNATURE;
    data = (attr_Signature_info *)
            malloc(sizeof (attr_Signature_info));
    if (!data)
        return -1;
    if (ru2(&(data->signature_index), input) < 0)
        return -1;
    cu = getConstant_Utf8(cf, data->signature_index);
    if (!cu)
    {
        logError("Assertion error: constant_pool[%i] is not const_Utf8_data instance!\r\n", data->signature_index);
        return -1;
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_Signature(attr_info *info)
{
    if (info->tag != TAG_ATTR_SIGNATURE)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_LocalVariableTypeTable(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_LocalVariableTypeTable_info *data;
    u2 i, lvttl;
    int cap;
    const_Utf8_data *cu;

    info->tag = TAG_ATTR_LOCALVARIABLETYPETABLE;
    if (ru2(&lvttl, input) < 0)
        return -1;
    cap = sizeof (attr_LocalVariableTypeTable_info)
        + sizeof (struct local_variable_type_table_entry) * lvttl;
    data = (attr_LocalVariableTypeTable_info *) malloc(cap);
    data->local_variable_type_table_length = lvttl;
    if (!data)
        return -1;
    for (i = 0u; i < lvttl; i++)
    {
        if (ru2(&(data->local_variable_type_table[i].start_pc), input) < 0)
            return -1;
        if (ru2(&(data->local_variable_type_table[i].length), input) < 0)
            return -1;
        if (ru2(&(data->local_variable_type_table[i].name_index), input) < 0)
            return -1;
        cu = getConstant_Utf8(cf, data->local_variable_type_table[i].name_index);
        if (!cu)
        {
            logError("Assertion error: constant_pool[%i] is not const_Utf8_data instance!\r\n");
            return -1;
        }
        if (ru2(&(data->local_variable_type_table[i].signature_index), input) < 0)
            return -1;
        cu = getConstant_Utf8(cf, data->local_variable_type_table[i].signature_index);
        if (!cu)
        {
            logError("Assertion error: constant_pool[%i] is not const_Utf8_data instance!\r\n");
            return -1;
        }
        if (ru2(&(data->local_variable_type_table[i].index), input) < 0)
            return -1;
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_LocalVariableTypeTable(attr_info *info)
{
    if (info->tag != TAG_ATTR_LOCALVARIABLETYPETABLE)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadElementValue(ClassFile *, struct BufferIO *, struct element_value *);
static int
freeElementValue(ClassFile *, struct element_value *);
static int
loadElementValuePair(ClassFile *, struct BufferIO *, struct element_value_pair *);
static int
freeElementValuePair(ClassFile *, struct element_value_pair *);

static int
loadAnnotation(ClassFile *cf, struct BufferIO *input,
        struct annotation *anno)
{
    const_Utf8_data *utf8;
    struct element_value_pair *pair;
    u2 i;
    
    if (ru2(&(anno->type_index), input) < 0)
        return -1;
    utf8 = getConstant_Utf8(cf, anno->type_index);
    if (!utf8)
        return -1;
    if (!isFieldDescriptor(utf8->length, utf8->bytes))
        return -1;
    if (ru2(&(anno->num_element_value_pairs), input) < 0)
        return -1;
    
    if (anno->num_element_value_pairs < 0)
        return -1;
    if (anno->num_element_value_pairs == 0)
    {
        anno->element_value_pairs = (struct element_value_pair *) 0;
        return 0;
    }
    anno->element_value_pairs = (struct element_value_pair *)
            allocMemory(anno->num_element_value_pairs,
            sizeof (struct element_value_pair));
    for (i = 0; i < anno->num_element_value_pairs; i++)
    {
        pair = &(anno->element_value_pairs[i]);
        if (loadElementValuePair(cf, input, pair) < 0)
            return -1;
    }

    return 0;
}

static int
freeAnnotation(ClassFile *cf, struct annotation *anno)
{
    u2 i;
    struct element_value_pair *pair;
    
    for (i = 0; i < anno->num_element_value_pairs; i++)
    {
        pair = &(anno->element_value_pairs[i]);
        freeElementValuePair(cf, pair);
    }
    freeMemory(anno->element_value_pairs);
    anno->element_value_pairs = (struct element_value_pair *) 0;

    return 0;
}

static int
loadElementValue(ClassFile *cf, struct BufferIO *input,
        struct element_value *value)
{
    const_Fieldref_data *cfi;
    const_NameAndType_data *cni;
    const_Utf8_data *cui;
    u2 i;
    
    if (ru1(&(value->tag), input) < 0)
        return -1;
    switch (value->tag)
    {
        // const_value_index
        case 'B':
        case 'C':
        case 'D':
        case 'F':
        case 'I':
        case 'J':
        case 'S':
        case 'Z':
        case 's':
            if (ru2(&(value->const_value_index), input) < 0)
                return -1;
            cfi = getConstant_Fieldref(cf, value->const_value_index);
            if (!cfi) return -1;
            cni = getConstant_NameAndType(cf, cfi->name_and_type_index);
            if (!cni) return -1;
            cui = getConstant_Utf8(cf, cni->descriptor_index);
            if (!cui) return -1;
            if (value->tag == 's')
            {
                if (strncmp((char *) cui->bytes,
                        "Ljava/lang/String;",
                        cui->length))
                    return -1;
            }
            else
            {
                if (cui->length != 1
                        || cui->bytes[0] != value->tag)
                    return -1;
            }
            break;
        // enum
        case 'e':
            if (ru2(&(value->enum_const_value.type_name_index), input) < 0)
                return -1;
            cui = getConstant_Utf8(cf, value->enum_const_value.type_name_index);
            if (!cui) return -1;
            // representing a valid field descriptor (§4.3.2)
            // that denotes the internal form of the binary
            // name (§4.2.1) of the type of the enum constant represented by this
            // element_value structure.
            if (!isFieldDescriptor(cui->length, cui->bytes)) return -1;
            if (ru2(&(value->enum_const_value.const_name_index), input) < 0)
                return -1;
            cui = getConstant_Utf8(cf, value->enum_const_value.const_name_index);
            if (!cui) return -1;
            // representing the simple name of the enum constant
            // represented by this element_value structure
            break;
        case 'c':
            if (ru2(&(value->class_info_index), input) < 0)
                return -1;
            cui = getConstant_Utf8(cf, value->class_info_index);
            if (!cui) return -1;
            // representing the return descriptor (§4.3.3)
            // of the type that is reified by the class
            // represented by this element_value structure
        case '@':
            if (loadAnnotation(cf, input, &(value->annotation_value)) < 0)
                return -1;
            // The element_value structure represents a "nested" annotation
            break;
        case '[':
            if (ru2(&(value->array_value.num_values), input) < 0)
                return -1;
            if (value->array_value.num_values == 0)
            {
                value->array_value.values = (struct element_value *) 0;
            }
            else
            {
                value->array_value.values = (struct element_value *)
                        allocMemory(value->array_value.num_values,
                            sizeof (struct element_value));
                if (!value->array_value.values)
                    return -1;
                for (i = 0; i < value->array_value.num_values; i++)
                    loadElementValue(cf, input, &(value->array_value.values[i]));
            }
            break;
        default:
            logError("Assertion error: Invalid element_value tag!\r\n");
            return -1;
    }
    return 0;
}

static int
freeElementValue(ClassFile *cf, struct element_value *value)
{
    if (value->tag == '[' && value->array_value.values)
    {
        free(value->array_value.values);
        value->array_value.values = (struct element_value *) 0;
    }
    return 0;
}

static int
loadElementValuePair(ClassFile *cf, struct BufferIO *input,
        struct element_value_pair *pair)
{
    u2 index;
    const_Utf8_data *utf8;

    if (ru2(&index, input) < 0)
        return -1;
    utf8 = getConstant_Utf8(cf, index);
    if (!utf8 || !isFieldDescriptor(utf8->length, utf8->bytes))
        return -1;
    pair->element_name_index = index;
    pair->value = (struct element_value *)
        allocMemory(1, sizeof (struct element_value));
    return loadElementValue(cf, input, pair->value);
}

static inline int
freeElementValuePair(ClassFile *cf, struct element_value_pair *pair)
{
    freeElementValue(cf, pair->value);
    freeMemory(pair->value);
    pair->value = (struct element_value *) 0;

    return 0;
}

static int
loadAttribute_RuntimeVisibleAnnotations(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_RuntimeVisibleAnnotations_info *data;
    u2 num_annotations, i;
    
    info->tag = TAG_ATTR_RUNTIMEVISIBLEANNOTATIONS;
    if (ru2(&num_annotations, input) < 0)
        return -1;
    data = (attr_RuntimeVisibleAnnotations_info *)
            allocMemory(1, sizeof (attr_RuntimeVisibleAnnotations_info)
                + sizeof (struct annotation) * num_annotations);
    if (!data)
        return -1;
    data->num_annotations = num_annotations;
    for (i = 0; i < num_annotations; i++)
        if (loadAnnotation(cf, input, &(data->annotations[i])) < 0)
            return -1;
    
    info->data = data;
    return 0;
}

static int
freeAttribute_RuntimeVisibleAnnotations(ClassFile *cf, attr_info *info)
{
    attr_RuntimeVisibleAnnotations_info *data;
    u2 i;
    
    if (info->tag != TAG_ATTR_RUNTIMEVISIBLEANNOTATIONS)
        return -1;
    data = (attr_RuntimeVisibleAnnotations_info *) info->data;
    for (i = 0; i < data->num_annotations; i++)
        freeAnnotation(cf, &(data->annotations[i]));
    
    free(info->data);
    info->data = (attr_RuntimeVisibleAnnotations_info *) 0;
    return 0;
}

static int
loadAttribute_RuntimeInvisibleAnnotations(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_RuntimeInvisibleAnnotations_info *data;
    u2 num_annotations, i;
    struct annotation *anno;
    
    info->tag = TAG_ATTR_RUNTIMEINVISIBLEANNOTATIONS;
    if (ru2(&num_annotations, input) < 0)
        return -1;
    data = (attr_RuntimeInvisibleAnnotations_info *)
            allocMemory(1, sizeof (attr_RuntimeInvisibleAnnotations_info)
                + sizeof (struct annotation) * num_annotations);
    if (!data)
        return -1;
    data->num_annotations = num_annotations;
    for (i = 0; i < num_annotations; i++)
        if (loadAnnotation(cf, input, &(data->annotations[i])) < 0)
            return -1;
    
    info->data = data;
    return 0;
}

static int
freeAttribute_RuntimeInvisibleAnnotations(ClassFile *cf, attr_info *info)
{
    attr_RuntimeInvisibleAnnotations_info *data;
    u2 i;
    
    if (info->tag != TAG_ATTR_RUNTIMEINVISIBLEANNOTATIONS)
        return -1;
    data = (attr_RuntimeInvisibleAnnotations_info *) info->data;
    for (i = 0; i < data->num_annotations; i++)
        freeAnnotation(cf, &(data->annotations[i]));
    
    free(info->data);
    info->data = (attr_RuntimeInvisibleAnnotations_info *) 0;
    return 0;
}

static int
loadAttribute_RuntimeVisibleParameterAnnotations(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_RuntimeVisibleParameterAnnotations_info *data;
    u1 num_parameters, i;
    u2 j;
    
    info->tag = TAG_ATTR_RUNTIMEVISIBLEPARAMETERANNOTATIONS;
    if (ru1(&num_parameters, input) < 0)
        return -1;
    data = (attr_RuntimeVisibleParameterAnnotations_info *)
            allocMemory(1, sizeof (attr_RuntimeVisibleParameterAnnotations_info)
            + num_parameters * sizeof (struct parameter_annotation));
    if (!data)
        return -1;
    data->num_parameters = num_parameters;
    for (i = 0; i < num_parameters; i++)
    {
        if (ru2(&(data->parameter_annotations[i].num_annotations), input) < 0)
            return -1;
        data->parameter_annotations[i].annotations = (struct annotation *)
                allocMemory(data->parameter_annotations[i].num_annotations,
                    sizeof (struct annotation));
        if (!data->parameter_annotations[i].annotations)
            return -1;
        for (j = 0; j < data->parameter_annotations[i].num_annotations; j++)
            if (loadAnnotation(cf, input,
                    &(data->parameter_annotations[i].annotations[j])) < 0)
                return -1;
    }
    
    info->data = data;
    return 0;
}

static int
freeAttribute_RuntimeVisibleParameterAnnotations(ClassFile *cf, attr_info *info)
{
    attr_RuntimeVisibleParameterAnnotations_info *data;
    u1 i;
    u2 j;
    
    if (info->tag != TAG_ATTR_RUNTIMEVISIBLEPARAMETERANNOTATIONS)
        return -1;
    data = (attr_RuntimeVisibleParameterAnnotations_info *) info->data;
    for (i = 0; i < data->num_parameters; i++)
    {
        for (j = 0; j < data->parameter_annotations[i].num_annotations; j++)
            freeAnnotation(cf, &(data->parameter_annotations[i].annotations[j]));
        free(data->parameter_annotations[i].annotations);
    }
    free(info->data);
    info->data = (attr_RuntimeVisibleParameterAnnotations_info *) 0;
    
    return 0;
}

static int
loadAttribute_RuntimeInvisibleParameterAnnotations(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_RuntimeInvisibleParameterAnnotations_info *data;
    u1 num_parameters, i;
    u2 j;
    
    info->tag = TAG_ATTR_RUNTIMEINVISIBLEPARAMETERANNOTATIONS;
    if (ru1(&num_parameters, input) < 0)
        return -1;
    data = (attr_RuntimeInvisibleParameterAnnotations_info *)
            allocMemory(1, sizeof (attr_RuntimeInvisibleParameterAnnotations_info)
            + num_parameters * sizeof (struct parameter_annotation));
    if (!data)
        return -1;
    data->num_parameters = num_parameters;
    for (i = 0; i < num_parameters; i++)
    {
        if (ru2(&(data->parameter_annotations[i].num_annotations), input) < 0)
            return -1;
        data->parameter_annotations[i].annotations = (struct annotation *)
                allocMemory(data->parameter_annotations[i].num_annotations,
                    sizeof (struct annotation));
        if (!data->parameter_annotations[i].annotations)
            return -1;
        for (j = 0; j < data->parameter_annotations[i].num_annotations; j++)
            if (loadAnnotation(cf, input,
                    &(data->parameter_annotations[i].annotations[j])) < 0)
                return -1;
    }
    
    info->data = data;
    return 0;
}

static int
freeAttribute_RuntimeInvisibleParameterAnnotations(ClassFile *cf, attr_info *info)
{
    attr_RuntimeInvisibleParameterAnnotations_info *data;
    u1 i;
    u2 j;
    
    if (info->tag != TAG_ATTR_RUNTIMEINVISIBLEPARAMETERANNOTATIONS)
        return -1;
    data = (attr_RuntimeInvisibleParameterAnnotations_info *) info->data;
    for (i = 0; i < data->num_parameters; i++)
    {
        for (j = 0; j < data->parameter_annotations[i].num_annotations; j++)
            freeAnnotation(cf, &(data->parameter_annotations[i].annotations[j]));
        free(data->parameter_annotations[i].annotations);
    }
    free(info->data);
    info->data = (attr_RuntimeInvisibleParameterAnnotations_info *) 0;
    
    return 0;
}

static int
loadAttribute_AnnotationDefault(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_AnnotationDefault_info *data;
    
    info->tag = TAG_ATTR_ANNOTATIONDEFAULT;
    data = (attr_AnnotationDefault_info *)
            allocMemory(1, sizeof (attr_AnnotationDefault_info));
    if (!data)
        return -1;
    if (loadElementValue(cf, input, &(data->default_value)) < 0)
        return -1;
    info->data = data;
    
    return 0;
}

static int
freeAttribute_AnnotationDefault(ClassFile *cf, attr_info *info)
{
    attr_AnnotationDefault_info *data;
    
    if (info->tag != TAG_ATTR_ANNOTATIONDEFAULT)
        return -1;
    data = (attr_AnnotationDefault_info *) info->data;
    freeElementValue(cf, &(data->default_value));
    free(info->data);
    info->data = (attr_AnnotationDefault_info *) 0;
    
    return 0;
}

#endif /* VERSION 49.0 */
#if VER_CMP(50, 0)
static int
loadVerificationTypeInfo(ClassFile *cf, struct BufferIO *input,
        union verification_type_info *stack)
{
    u1 tag;
    
    if (ru1(&tag, input) < 0)
        return -1;
    switch (tag)
    {
        case ITEM_Top:
            stack->Top_variable_info.tag = tag;
            break;
        case ITEM_Integer:
            stack->Integer_variable_info.tag = tag;
            break;
        case ITEM_Float:
            stack->Float_variable_info.tag = tag;
            break;
        case ITEM_Long:
            stack->Long_variable_info.tag = tag;
            break;
        case ITEM_Double:
            stack->Double_variable_info.tag = tag;
            break;
        case ITEM_Null:
            stack->Null_variable_info.tag = tag;
            break;
        case ITEM_UninitializedThis:
            stack->UninitializedThis_variable_info.tag = tag;
            break;
        case ITEM_Object:
            stack->Object_variable_info.tag = tag;
            if (ru2(&(stack->Object_variable_info.cpool_index), input) < 0)
                return -1;
            break;
        case ITEM_Uninitialized:
            stack->Uninitialized_variable_info.tag = tag;
            if (ru2(&(stack->Uninitialized_variable_info.offset), input) < 0)
                return -1;
            break;
        default:
            logError("Assertion error: Unknown tag [%i]!\r\n", tag);
            return -1;
    }
    return 0;
}

static int
freeVerificationTypeInfo(ClassFile *cf,
        union verification_type_info *stack)
{
    return 0;
}

static int
loadAttribute_StackMapTable(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    attr_StackMapTable_info *data;
    union stack_map_frame *entry;
    u2 i, number_of_entries;
    u1 frame_type, cap, j;

    info->tag = TAG_ATTR_STACKMAPTABLE;
    if (ru2(&number_of_entries, input) < 0)
        return -1;
    data = (attr_StackMapTable_info *)
            allocMemory(1, sizeof (attr_StackMapTable_info)
                + number_of_entries * sizeof (union stack_map_frame));
    if (!data)
        return -1;
    data->number_of_entries = number_of_entries;
    for (i = 0u; i < number_of_entries; i++)
    {
        if (ru1(&frame_type, input) < 0)
            return -1;
        entry = &(data->entries[i]);
        if (frame_type >= SMF_SAME_MIN
                && frame_type <= SMF_SAME_MAX)
        {
            entry->same_frame.frame_type = frame_type;
        }
        else if (frame_type >= SMF_SL1SI_MIN
                && frame_type <= SMF_SL1SI_MAX)
        {
            entry->same_locals_1_stack_item_frame.frame_type = frame_type;
            if (loadVerificationTypeInfo(cf, input,
                    &(entry->same_locals_1_stack_item_frame.stack)) < 0)
                return -1;
        }
        else if (frame_type == SMF_SL1SIE)
        {
            entry->same_locals_1_stack_item_frame_extended.frame_type = frame_type;
            if (ru2(&(entry->same_locals_1_stack_item_frame_extended.offset_delta), input) < 0)
                return -1;
            if (loadVerificationTypeInfo(cf, input,
                    &(entry->same_locals_1_stack_item_frame_extended.stack)) < 0)
                return -1;
        }
        else if (frame_type >= SMF_CHOP_MIN
                && frame_type <= SMF_CHOP_MAX)
        {
            entry->chop_frame.frame_type = frame_type;
            if (ru2(&(entry->chop_frame.offset_delta), input) < 0)
                return -1;
        }
        else if (frame_type == SMF_SAMEE)
        {
            entry->same_frame_extended.frame_type = frame_type;
            if (ru2(&(entry->same_frame_extended.offset_delta), input) < 0)
                return -1;
        }
        else if (frame_type >= SMF_APPEND_MIN
                && frame_type <= SMF_APPEND_MAX)
        {
            entry->append_frame.frame_type = frame_type;
            if (ru2(&(entry->append_frame.offset_delta), input) < 0)
                return -1;
            cap = frame_type - 251;
            entry->append_frame.stack = (union verification_type_info *)
                    allocMemory(cap, sizeof (union verification_type_info));
            if (!entry->append_frame.stack)
                return -1;
            for (j = 0; j < cap; j++)
                if (loadVerificationTypeInfo(cf, input,
                        &(entry->append_frame.stack[j])) < 0)
                    return -1;
        }
        else if (frame_type == SMF_FULL)
        {
            entry->full_frame.frame_type = frame_type;
            if (ru2(&(entry->full_frame.offset_delta), input) < 0)
                return -1;
            if (ru2(&(entry->full_frame.number_of_locals), input) < 0)
                return -1;
            if (entry->full_frame.number_of_locals > 0)
            {
                entry->full_frame.locals = (union verification_type_info *)
                        allocMemory(entry->full_frame.number_of_locals,
                            sizeof (union verification_type_info));
                if (!entry->full_frame.locals)
                    return -1;
            }
            else if (entry->full_frame.number_of_locals == 0)
            {
                entry->full_frame.locals = (union verification_type_info *) 0;
            }
            else
            {
                logError("Assertion error: number_of_locals is negative!\r\n");
                return -1;
            }
            for (j = 0; j < entry->full_frame.number_of_locals; j++)
                if (loadVerificationTypeInfo(cf, input,
                        &(entry->full_frame.locals[j])) < 0)
                    return -1;
            if (ru2(&(entry->full_frame.number_of_stack_items), input) < 0)
                return -1;
            if (entry->full_frame.number_of_stack_items > 0)
            {
                entry->full_frame.stack = (union verification_type_info *)
                        allocMemory(entry->full_frame.number_of_stack_items,
                            sizeof (union verification_type_info));
                if (!entry->full_frame.stack)
                    return -1;
            }
            else if (entry->full_frame.number_of_stack_items == 0)
            {
                entry->full_frame.stack = (union verification_type_info *) 0;
            }
            else if (entry->full_frame.number_of_stack_items < 0)
            {
                logError("Assertion error: number_of_stack_items is negative!\r\n");
                return -1;
            }
            for (j = 0; j < entry->full_frame.number_of_stack_items; j++)
                if (loadVerificationTypeInfo(cf, input,
                        &(entry->full_frame.stack[j])) < 0)
                    return -1;
        }
        else
        {
            logError("Unknown frame type: %i\r\n", frame_type);
        }
    }

    info->data = data;
    return 0;
}

static int
freeAttribute_StackMapTable(ClassFile *cf, attr_info *info)
{
    attr_StackMapTable_info *data;
    union stack_map_frame *entry;
    u2 i;
    u1 frame_type, j;
    
    if (info->tag != TAG_ATTR_STACKMAPTABLE)
        return -1;
    data = (attr_StackMapTable_info *) info->data;
    for (i = 0; i < data->number_of_entries; i++)
    {
        entry = &(data->entries[i]);
        frame_type = entry->same_frame.frame_type;
        if (frame_type >= SMF_SL1SI_MIN
                && frame_type <= SMF_SL1SI_MAX)
        {
            if (freeVerificationTypeInfo(cf, &(entry->same_locals_1_stack_item_frame.stack)) < 0)
                return -1;
        }
        else if (frame_type == SMF_SL1SIE)
        {
            if (freeVerificationTypeInfo(cf, &(entry->same_locals_1_stack_item_frame_extended.stack)) < 0)
                return -1;
        }
        else if (frame_type >= SMF_APPEND_MIN
                && frame_type <= SMF_APPEND_MAX)
        {
            if (entry->append_frame.stack)
            {
                for (j = 0; j < frame_type - 251; j++)
                {
                    if (freeVerificationTypeInfo(cf, &(entry->append_frame.stack[j])) < 0)
                        return -1;
                }
                free(entry->append_frame.stack);
            }
        }
        else if (frame_type == SMF_FULL)
        {
            if (entry->full_frame.locals)
            {
                for (j = 0; j < entry->full_frame.number_of_locals; j++)
                {
                    if (freeVerificationTypeInfo(cf, &(entry->full_frame.locals[j])) < 0)
                        return -1;
                }
                free(entry->full_frame.locals);
            }
            if (entry->full_frame.stack)
            {
                for (j = 0; j < entry->full_frame.number_of_stack_items; j++)
                {
                    if (freeVerificationTypeInfo(cf, &(entry->full_frame.stack[j])) < 0)
                        return -1;
                }
                free(entry->full_frame.stack);
            }
        }
    }
    free(info->data);
    info->data = (void *) 0;
    
    return 0;
}
#endif /* VERSION 50.0 */
#if VER_CMP(51, 0)
static int
loadAttribute_BootstrapMethods(ClassFile *cf, struct BufferIO *input,
        attr_info *info)
{
    attr_BootstrapMethods_info *data;
    u2 num_bootstrap_methods, i, j;
    struct bootstrap_method *m;
    
    info->tag = TAG_ATTR_BOOTSTRAPMETHODS;
    if (ru2(&num_bootstrap_methods, input) < 0)
        return -1;
    data = (attr_BootstrapMethods_info *) allocMemory(1,
            sizeof (attr_BootstrapMethods_info)
            + sizeof (struct bootstrap_method) * num_bootstrap_methods);
    if (!data)
        return -1;
    data->num_bootstrap_methods = num_bootstrap_methods;
    for (i = 0; i < num_bootstrap_methods; i++)
    {
        m = &(data->bootstrap_methods[i]);
        if (ru2(&(m->bootstrap_method_ref), input) < 0)
            return -1;
        if (ru2(&(m->num_bootstrap_arguments), input) < 0)
            return -1;
        if (m->num_bootstrap_arguments == 0)
        {
            m->bootstrap_arguments = (u2 *) 0;
        }
        else if (m->num_bootstrap_arguments > 0)
        {
            m->bootstrap_arguments = (u2 *) allocMemory(m->num_bootstrap_arguments, sizeof (u2));
            if (!m->bootstrap_arguments)
                return -1;
            for (j = 0; j < m->num_bootstrap_arguments; j++)
                if (ru2(&(m->bootstrap_arguments[j]), input) < 0)
                    return -1;
        }
        else
        {
            logError("Assertion error: num_bootstrap_arguments is negative!\r\n");
            return -1;
        }
    }
    info->data = data;
    
    return 0;
}

static int
freeAttribute_BootstrapMethods(ClassFile *cf, attr_info *info)
{
    attr_BootstrapMethods_info *data;
    u2 i;
    
    if (info->tag != TAG_ATTR_BOOTSTRAPMETHODS)
        return -1;
    data = (attr_BootstrapMethods_info *) info->data;
    for (i = 0; i < data->num_bootstrap_methods; i++)
    {
        free(data->bootstrap_methods[i].bootstrap_arguments);
        data->bootstrap_methods[i].bootstrap_arguments = (u2 *) 0;
    }
    free(info->data);
    info->data = (void *) 0;
    
    return 0;
}
#endif /* VERSION 51.0 */
#if VER_CMP(52, 0)
static int
loadAttribute_MethodParameters(ClassFile *cf, struct BufferIO *input,
        attr_info *info)
{
    attr_MethodParameters_info *data;
    u1 parameters_count;
    u1 i;
    struct parameter_entry *parameter;

    info->tag = TAG_ATTR_METHODPARAMETERS;
    if (ru1(&parameters_count, input) < 0)
        return -1;
    data = (attr_MethodParameters_info *)
        allocMemory(1, sizeof (u1)
                + sizeof (struct parameter_entry)
                * parameters_count);
    data->parameters_count = parameters_count;
    for (i = 0; i < parameters_count; i++)
    {
        parameter = &(data->parameters[i]);
        if (ru2(&(parameter->name_index), input) < 0)
            return -1;
        if (ru2(&(parameter->access_flags), input) < 0)
            return -1;
    }
    info->data = data;

    return 0;
}

static int
freeAttribute_MethodParameters(ClassFile *cf, attr_info *info)
{
    if (info->tag != TAG_ATTR_METHODPARAMETERS)
        return -1;
    free(info->data);
    info->data = (void *) 0;

    return 0;
}

static int
loadAttribute_RuntimeTypeAnnotations(ClassFile *cf,
        struct BufferIO *input, attr_info *info)
{
    u2 num_annotations;
    attr_RuntimeVisibleTypeAnnotations_info *data;
    u2 i, j;
    struct type_annotation *annotation;
    u1 target_type;
    u2 table_length;
    u2 nevp;
    u1 path_length;
    struct localvar_table_entry * entry;
    struct element_value_pair * pair;

    if (ru2(&num_annotations, input) < 0)
        return -1;
    data = (attr_RuntimeVisibleTypeAnnotations_info *)
        allocMemory(1, sizeof (u2)
                + sizeof (struct type_annotation) * num_annotations);
    data->num_annotations = num_annotations;
    for (i = 0; i < num_annotations; i++)
    {
        annotation = &(data->annotations[i]);
        if (ru1(&target_type, input) < 0)
            return -1;
        annotation->target_type = target_type;

        // retrieve target_info
        switch (target_type)
        {
            case 0x00:case 0x01:
                // type_parameter_target
                if (ru1(&(annotation->target_info.type_parameter_index),
                            input) < 0)
                    return -1;
                break;
            case 0x10:
                // supertype_target
                if (ru2(&(annotation->target_info.supertype_index),
                            input) < 0)
                    return -1;
                break;
            case 0x11:case 0x12:
                // type_parameter_bound_target
                if (ru1(&(annotation->target_info.type_parameter_index),
                            input) < 0)
                    return -1;
                if (ru1(&(annotation->target_info.bound_index),
                            input) < 0)
                    return -1;
                break;
            case 0x13:case 0x14:case 0x15:
                // empty_target
                break;
            case 0x16:
                // formal_parameter_target
                if (ru1(&(annotation->target_info.formal_parameter_index),
                            input) < 0)
                    return -1;
                break;
            case 0x17:
                // throws_target
                if (ru2(&(annotation->target_info.throws_type_index),
                            input) < 0)
                    return -1;
                break;
            case 0x40:case 0x41:
                // localvar_target
                if (ru2(&table_length, input) < 0)
                    return -1;
                annotation->target_info.table_length = table_length;
                annotation->target_info.table =
                    (struct localvar_table_entry *)
                    allocMemory(table_length,
                            sizeof (struct localvar_table_entry));
                for (j = 0; j < table_length; j++)
                {
                    entry = &(annotation->target_info.table[j]);
                    if (ru2(&(entry->start_pc), input) < 0)
                        return -1;
                    if (ru2(&(entry->length), input) < 0)
                        return -1;
                    if (ru2(&(entry->index), input) < 0)
                        return -1;
                }
                break;
            case 0x42:
                // catch_target
                if (ru2(&(annotation->target_info.exception_table_index),
                            input) < 0)
                    return -1;
                break;
            case 0x43:case 0x44:case 0x45:case 0x46:
                // offset_target
                if (ru2(&(annotation->target_info.offset), input) < 0)
                    return -1;
                break;
            case 0x47:case 0x48:case 0x49:case 0x4a:case 0x4b:
                // type_argument_target
                if (ru2(&(annotation->target_info.offset), input) < 0)
                    return -1;
                if (ru1(&(annotation->target_info.type_argument_index),
                            input) < 0)
                    return -1;
                break;
        } /* switch target_type */

        // retrieve target_path
        if (ru1(&path_length, input) < 0)
            return -1;
        annotation->target_path.path_length = path_length;
        annotation->target_path.path = (struct type_path_entry *)
            allocMemory(path_length,
                    sizeof (struct type_path_entry));
        if (rbs((u1 *) annotation->target_path.path, input,
                    sizeof (struct type_path_entry) *
                    path_length) < 0)
            return -1;

        // retrieve type_index
        if (ru2(&(annotation->type_index), input) < 0)
            return -1;

        // retrieve num_element_value_pairs
        if (ru2(&nevp, input) < 0)
            return -1;
        annotation->num_element_value_pairs = nevp;
        annotation->element_value_pairs = (struct element_value_pair *)
            allocMemory(nevp, sizeof (struct element_value_pair));
        for (j = 0; j < nevp; j++)
        {
            pair = &(annotation->element_value_pairs[j]);
            if (loadElementValuePair(cf, input, pair) < 0)
                return -1;
        }

    }

    return 0;
}

static int
freeAttribute_RuntimeTypeAnnotations(ClassFile *cf, attr_info *info)
{
    u2 i, j, num, nevp;
    attr_RuntimeVisibleTypeAnnotations_info *data;
    struct type_annotation *anno;
    struct element_value_pair *pair;

    data = (attr_RuntimeVisibleTypeAnnotations_info *) info->data;
    num = data->num_annotations;
    for (i = 0; i < num; i++)
    {
        anno = &(data->annotations[i]);
        nevp = anno->num_element_value_pairs;
        for (j = 0; j < nevp; j++)
        {
            pair = &(anno->element_value_pairs[j]);
            freeElementValuePair(cf, pair);
        }
    }
    return 0;
}

static int
loadAttribute_RuntimeVisibleTypeAnnotations(ClassFile *cf,
        struct BufferIO *input, attr_info *info)
{
    info->tag = TAG_ATTR_RUNTIMEVISIBLETYPEANNOTATIONS;
    return loadAttribute_RuntimeTypeAnnotations(cf, input, info);
}

static int
loadAttribute_RuntimeInvisibleTypeAnnotations(ClassFile *cf,
        struct BufferIO *input, attr_info *info)
{
    info->tag = TAG_ATTR_RUNTIMEINVISIBLETYPEANNOTATIONS;
    return loadAttribute_RuntimeTypeAnnotations(cf, input, info);
}

static int
freeAttribute_RuntimeVisibleTypeAnnotations(ClassFile *cf,
        attr_info *info)
{
    if (info->tag != TAG_ATTR_RUNTIMEVISIBLETYPEANNOTATIONS)
        return -1;
    return freeAttribute_RuntimeTypeAnnotations(cf, info);
}

static int
freeAttribute_RuntimeInvisibleTypeAnnotations(ClassFile *cf,
        attr_info *info)
{
    if (info->tag != TAG_ATTR_RUNTIMEINVISIBLETYPEANNOTATIONS)
        return -1;
    return freeAttribute_RuntimeTypeAnnotations(cf, info);
}

#endif /* VERSION 52.0 */

extern int
loadAttribute_class(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    u2 attribute_name_index;
    const_Utf8_data *utf8;
    int attribute_name_length;
    char *attribute_name;
    int res;

    if (ru2(&attribute_name_index, input) < 0)
        return -1;
    if (loadAttribute(input, info) < 0)
        return -1;
    utf8 = getConstant_Utf8(cf, attribute_name_index);
    if (!utf8) return -1;
    attribute_name_length = (int) utf8->length;
    attribute_name = (char *) utf8->bytes;

#if VER_CMP(45, 3)
    if (!strncmp(attribute_name, "SourceFile", 10))
        return loadAttribute_SourceFile(cf, input, info);
    if (!strncmp(attribute_name, "InnerClasses", 12))
    {
        res = loadAttribute_InnerClasses(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "Synthetic", 9))
        return loadAttribute_Synthetic(cf, input, info);
    if (!strncmp(attribute_name, "Deprecated", 10))
        return loadAttribute_Deprecated(cf, input, info);
#endif
#if VER_CMP(49, 0)
    if (!strncmp(attribute_name, "EnclosingMethod", 15))
    {
        res = loadAttribute_EnclosingMethod(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "SourceDebugExtension", 20))
        return loadAttribute_SourceDebugExtension(cf, input, info);
    if (!strncmp(attribute_name, "Signature", 9))
        return loadAttribute_Signature(cf, input, info);
    if (!strncmp(attribute_name, "RuntimeVisibleAnnotations", 25))
    {
        res = loadAttribute_RuntimeVisibleAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleAnnotations", 27))
    {
        res = loadAttribute_RuntimeInvisibleAnnotations(cf, input, info);
        return res;
    }
#endif
#if VER_CMP(51, 0)
    if (!strncmp(attribute_name, "BootstrapMethods", 16))
    {
        res = loadAttribute_BootstrapMethods(cf, input, info);
        return res;
    }
#endif
#if VER_CMP(52, 0)
    if (!strncmp(attribute_name, "RuntimeVisibleTypeAnnotations", 29))
    {
        res = loadAttribute_RuntimeVisibleTypeAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleTypeAnnotations", 31))
    {
        res = loadAttribute_RuntimeInvisibleTypeAnnotations(cf, input, info);
        return res;
    }
#endif
    logError("Fail to load incompatible attribute: %s.\r\n", attribute_name);
    return skipAttribute(input, attribute_name_length, attribute_name, info);
}

extern int
loadAttribute_field(ClassFile *cf, struct BufferIO *input,
        field_info *field, attr_info *info)
{
    u2 attribute_name_index;
    const_Utf8_data *utf8;
    int attribute_name_length;
    char *attribute_name;
    int res;

    if (ru2(&attribute_name_index, input) < 0)
        return -1;
    if (loadAttribute(input, info) < 0)
        return -1;
    utf8 = getConstant_Utf8(cf, attribute_name_index);
    if (!utf8) return -1;
    attribute_name_length = (int) utf8->length;
    attribute_name = (char *) utf8->bytes;

#if VER_CMP(45, 3)
    if (!strncmp(attribute_name, "ConstantValue", 13))
    {
        res = loadAttribute_ConstantValue(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "Synthetic", 9))
        return loadAttribute_Synthetic(cf, input, info);
    if (!strncmp(attribute_name, "Deprecated", 10))
        return loadAttribute_Deprecated(cf, input, info);
#endif
#if VER_CMP(49, 0)
    if (!strncmp(attribute_name, "Signature", 9))
        return loadAttribute_Signature(cf, input, info);
    if (!strncmp(attribute_name, "RuntimeVisibleAnnotations", 25))
    {
        res = loadAttribute_RuntimeVisibleAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleAnnotations", 27))
    {
        res = loadAttribute_RuntimeInvisibleAnnotations(cf, input, info);
        return res;
    }
#endif
#if VER_CMP(52, 0)
    if (!strncmp(attribute_name, "RuntimeVisibleTypeAnnotations", 29))
    {
        res = loadAttribute_RuntimeVisibleTypeAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleTypeAnnotations", 31))
    {
        res = loadAttribute_RuntimeInvisibleTypeAnnotations(cf, input, info);
        return res;
    }
#endif
    logError("Fail to load incompatible attribute: %s.\r\n", attribute_name);
    return skipAttribute(input, attribute_name_length, attribute_name, info);
}

extern int
loadAttribute_method(ClassFile *cf, struct BufferIO *input,
        method_info *method, attr_info *info)
{
    u2 attribute_name_index;
    const_Utf8_data *utf8;
    int attribute_name_length;
    char *attribute_name;
    int res;

    if (ru2(&attribute_name_index, input) < 0)
        return -1;
    if (loadAttribute(input, info) < 0)
        return -1;
    utf8 = getConstant_Utf8(cf, attribute_name_index);
    if (!utf8) return -1;
    attribute_name_length = (int) utf8->length;
    attribute_name = (char *) utf8->bytes;

#if VER_CMP(45, 3)
    if (!strncmp(attribute_name, "Code", 4))
    {
        res = loadAttribute_Code(cf, input, method, info);
        return res;
    }
    if (!strncmp(attribute_name, "Exceptions", 10))
    {
        res = loadAttribute_Exceptions(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "Synthetic", 9))
        return loadAttribute_Synthetic(cf, input, info);
    if (!strncmp(attribute_name, "Deprecated", 10))
        return loadAttribute_Deprecated(cf, input, info);
#endif
#if VER_CMP(49, 0)
    if (!strncmp(attribute_name, "RuntimeVisibleParameterAnnotations", 34))
    {
        res = loadAttribute_RuntimeVisibleParameterAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleParameterAnnotations", 36))
    {
        res = loadAttribute_RuntimeInvisibleParameterAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "AnnotationDefault", 17))
    {
        res = loadAttribute_AnnotationDefault(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "Signature", 9))
        return loadAttribute_Signature(cf, input, info);
    if (!strncmp(attribute_name, "RuntimeVisibleAnnotations", 25))
    {
        res = loadAttribute_RuntimeVisibleAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleAnnotations", 27))
    {
        res = loadAttribute_RuntimeInvisibleAnnotations(cf, input, info);
        return res;
    }
#endif
#if VER_CMP(52, 0)
    if (!strncmp(attribute_name, "MethodParameters", 16))
        return loadAttribute_MethodParameters(cf, input, info);
    if (!strncmp(attribute_name, "RuntimeVisibleTypeAnnotations", 29))
    {
        res = loadAttribute_RuntimeVisibleTypeAnnotations(cf, input, info);
        return res;
    }
    if (!strncmp(attribute_name, "RuntimeInvisibleTypeAnnotations", 31))
    {
        res = loadAttribute_RuntimeInvisibleTypeAnnotations(cf, input, info);
        return res;
    }
#endif
    logError("Fail to load incompatible attribute: %s.\r\n", attribute_name);
    return skipAttribute(input, attribute_name_length, attribute_name, info);
}

extern int
loadAttribute_code(ClassFile *cf, struct BufferIO *input, attr_info *info)
{
    u2 attribute_name_index;
    const_Utf8_data *utf8;
    int attribute_name_length;
    char *attribute_name;

    if (ru2(&attribute_name_index, input) < 0)
        return -1;
    if (loadAttribute(input, info) < 0)
        return -1;
    utf8 = getConstant_Utf8(cf, attribute_name_index);
    if (!utf8) return -1;
    attribute_name_length = (int) utf8->length;
    attribute_name = (char *) utf8->bytes;

#if VER_CMP(45, 3)
    if (!strncmp(attribute_name, "LineNumberTable", 15))
        return loadAttribute_LineNumberTable(cf, input, info);
    if (!strncmp(attribute_name, "LocalVariableTable", 18))
        return loadAttribute_LocalVariableTable(cf, input, info);
#endif
#if VER_CMP(49, 0)
    if (!strncmp(attribute_name, "LocalVariableTypeTable", 22))
        return loadAttribute_LocalVariableTypeTable(cf, input, info);
#endif
#if VER_CMP(50, 0)
    if (!strncmp(attribute_name, "StackMapTable", 13))
        return loadAttribute_StackMapTable(cf, input, info);
#endif
#if VER_CMP(52, 0)
    if (!strncmp(attribute_name, "RuntimeVisibleTypeAnnotations", 29))
        return loadAttribute_RuntimeVisibleTypeAnnotations(cf, input, info);
    if (!strncmp(attribute_name, "RuntimeInvisibleTypeAnnotations", 31))
        return loadAttribute_RuntimeInvisibleTypeAnnotations(cf, input, info);
#endif
    logError("Fail to load incompatible attribute: %s.\r\n", attribute_name);
    return skipAttribute(input, attribute_name_length, attribute_name, info);
}

extern int
loadAttributes_class(ClassFile *cf, struct BufferIO *input, u2 *attributes_count, attr_info **attributes)
{
    u2 i;

    if (!cf)
        return -1;
    if (ru2(attributes_count, input) < 0)
        return -1;
    *attributes = (attr_info *) malloc(*attributes_count * sizeof (attr_info));
    for (i = 0u; i < *attributes_count; i++)
        loadAttribute_class(cf, input, &((*attributes)[i]));
    return 0;
}

static int
freeAttribute_class(ClassFile * cf, attr_info *info)
{
#if VER_CMP(45, 3)
    if (!freeAttribute_SourceFile(info))
        return 0;
    if (!freeAttribute_InnerClasses(info))
        return 0;
#endif
#if VER_CMP(49, 0)
    if (!freeAttribute_InnerClasses(info))
        return 0;
    if (!freeAttribute_EnclosingMethod(info))
        return 0;
    if (!freeAttribute_Signature(info))
        return 0;
    if (!freeAttribute_RuntimeVisibleAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleAnnotations(cf, info))
        return 0;
#endif
#if VER_CMP(51, 0)
    if (!freeAttribute_BootstrapMethods(cf, info))
        return 0;
#endif
#if VER_CMP(52, 0)
    if (!freeAttribute_RuntimeVisibleTypeAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleTypeAnnotations(cf, info))
        return 0;
#endif
    logError("Fail to free incompatible attribute[%i].\r\n", info->tag);
    return -1;
}

extern int
freeAttributes_class(ClassFile * cf, u2 attributes_count, attr_info *attributes)
{
    u2 i;

    if (!attributes)
        return 0;

    for (i = 0; i < attributes_count; i++)
        freeAttribute_class(cf, &(attributes[i]));

    return 0;
}

static int
freeAttribute_field(ClassFile * cf, attr_info *info)
{
#if VER_CMP(45, 3)
    if (!freeAttribute_ConstantValue(info))
        return 0;
#endif
#if VER_CMP(49, 0)
    if (!freeAttribute_Signature(info))
        return 0;
    if (!freeAttribute_RuntimeVisibleAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleAnnotations(cf, info))
        return 0;
#endif
#if VER_CMP(52, 0)
    if (!freeAttribute_RuntimeVisibleTypeAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleTypeAnnotations(cf, info))
        return 0;
#endif
    logError("Fail to free incompatible attribute[%i].\r\n", info->tag);
    return -1;
}

extern int
freeAttributes_field(ClassFile * cf, u2 attributes_count, attr_info *attributes)
{
    u2 i;

    if (!attributes)
        return 0;

    for (i = 0; i < attributes_count; i++)
        freeAttribute_field(cf, &(attributes[i]));

    return 0;
}


extern int
loadAttributes_field(ClassFile *cf,
        struct BufferIO *input,
        field_info *field,
        u2 *attributes_count,
        attr_info **attributes)
{
    u2 i;

    if (!cf)
        return -1;
    if (ru2(attributes_count, input) < 0)
        return -1;
    //*attributes = (attr_info *) malloc(*attributes_count * sizeof (attr_info));
    *attributes = (attr_info *) allocMemory(*attributes_count, sizeof (attr_info));
    if (!*attributes) return -1;
    for (i = 0u; i < *attributes_count; i++)
        loadAttribute_field(cf, input, field, &((*attributes)[i]));
    return 0;
}

static int
freeAttribute_method(ClassFile * cf, attr_info *info)
{
#if VER_CMP(45, 3)
    if (!freeAttribute_Code(cf, info))
        return 0;
    if (!freeAttribute_Exceptions(info))
        return 0;
#endif
#if VER_CMP(49, 0)
    if (!freeAttribute_RuntimeVisibleParameterAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleParameterAnnotations(cf, info))
        return 0;
    if (!freeAttribute_AnnotationDefault(cf, info))
        return 0;
    if (!freeAttribute_Signature(info))
        return 0;
    if (!freeAttribute_RuntimeVisibleAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleAnnotations(cf, info))
        return 0;
#endif
#if VER_CMP(52, 0)
    if (!freeAttribute_MethodParameters(cf, info))
        return 0;
    if (!freeAttribute_RuntimeVisibleTypeAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleTypeAnnotations(cf, info))
        return 0;
#endif
    logError("Fail to free incompatible attribute[%i].\r\n", info->tag);
    return -1;
}

extern int
freeAttributes_method(ClassFile * cf, u2 attributes_count, attr_info *attributes)
{
    u2 i;

    if (!attributes)
        return 0;

    for (i = 0; i < attributes_count; i++)
        freeAttribute_method(cf, &(attributes[i]));

    return 0;
}

extern int
loadAttributes_method(ClassFile *cf,
        struct BufferIO *input,
        method_info *method,
        u2 *attributes_count,
        attr_info **attributes)
{
    u2 i;

    if (!cf)
        return -1;
    if (ru2(attributes_count, input) < 0)
        return -1;
    *attributes = (attr_info *) malloc(*attributes_count * sizeof (attr_info));
    for (i = 0u; i < *attributes_count; i++)
        loadAttribute_method(cf, input, method, &((*attributes)[i]));

    return 0;
}

static int
freeAttribute_code(ClassFile * cf, attr_info *info)
{
#if VER_CMP(45, 3)
    if (!freeAttribute_LineNumberTable(info))
        return 0;
    if (!freeAttribute_LocalVariableTable(info))
        return 0;
#endif
#if VER_CMP(49, 0)
    if (!freeAttribute_LocalVariableTypeTable(info))
        return 0;
#endif
#if VER_CMP(50, 0)
    if (!freeAttribute_StackMapTable(cf, info))
        return 0;
#endif
#if VER_CMP(52, 0)
    if (!freeAttribute_RuntimeVisibleTypeAnnotations(cf, info))
        return 0;
    if (!freeAttribute_RuntimeInvisibleTypeAnnotations(cf, info))
        return 0;
#endif
    logError("Fail to free incompatible attribute[%i].\r\n", info->tag);
    return -1;
}

extern int
freeAttributes_code(ClassFile * cf, u2 attributes_count, attr_info *attributes)
{
    u2 i;

    if (!attributes)
        return 0;

    for (i = 0; i < attributes_count; i++)
        freeAttribute_code(cf, &(attributes[i]));

    return 0;
}

extern int
loadAttributes_code(ClassFile *cf,
        struct BufferIO *input,
        attr_Code_info *code,
        u2 *attributes_count,
        attr_info **attributes)
{
    u2 i;

    if (!cf)
        return -1;
    if (ru2(attributes_count, input) < 0)
        return -1;
    *attributes = (attr_info *) malloc(*attributes_count * sizeof (attr_info));
    for (i = 0u; i < *attributes_count; i++)
        loadAttribute_code(cf, input, &((*attributes)[i]));
    return 0;
}

extern u4
getAttributeTag(size_t len, char * str)
{
    if (strncmp("ConstantValue", str, len) == 0)
        return TAG_ATTR_CONSTANTVALUE;
    if (strncmp("Code", str, len) == 0)
        return TAG_ATTR_CODE;
    if (strncmp("StackMapTable", str, len) == 0)
        return TAG_ATTR_STACKMAPTABLE;
    if (strncmp("Exceptions", str, len) == 0)
        return TAG_ATTR_EXCEPTIONS;
    if (strncmp("InnerClasses", str, len) == 0)
        return TAG_ATTR_INNERCLASSES;
    if (strncmp("EnclosingMethod", str, len) == 0)
        return TAG_ATTR_ENCLOSINGMETHOD;
    if (strncmp("Synthetic", str, len) == 0)
        return TAG_ATTR_SYNTHETIC;
    if (strncmp("Signature", str, len) == 0)
        return TAG_ATTR_SIGNATURE;
    if (strncmp("SourceFile", str, len) == 0)
        return TAG_ATTR_SOURCEFILE;
    if (strncmp("SourceDebugExtension", str, len) == 0)
        return TAG_ATTR_SOURCEDEBUGEXTENSION;
    if (strncmp("LineNumberTable", str, len) == 0)
        return TAG_ATTR_LINENUMBERTABLE;
    if (strncmp("LocalVariableTable", str, len) == 0)
        return TAG_ATTR_LOCALVARIABLETABLE;
    if (strncmp("LocalVariableTypeTable", str, len) == 0)
        return TAG_ATTR_LOCALVARIABLETYPETABLE;
    if (strncmp("Deprecated", str, len) == 0)
        return TAG_ATTR_DEPRECATED;
    if (strncmp("RuntimeVisibleAnnotations", str, len) == 0)
        return TAG_ATTR_RUNTIMEVISIBLEANNOTATIONS;
    if (strncmp("RuntimeInvisibleAnnotations", str, len) == 0)
        return TAG_ATTR_RUNTIMEINVISIBLEANNOTATIONS;
    if (strncmp("RuntimeVisibleParameterAnnotations", str, len) == 0)
        return TAG_ATTR_RUNTIMEVISIBLEPARAMETERANNOTATIONS;
    if (strncmp("RuntimeInvisibleParameterAnnotations", str, len) == 0)
        return TAG_ATTR_RUNTIMEINVISIBLEPARAMETERANNOTATIONS;
    if (strncmp("AnnotationDefault", str, len) == 0)
        return TAG_ATTR_ANNOTATIONDEFAULT;
    if (strncmp("BootstrapMethods", str, len) == 0)
        return TAG_ATTR_BOOTSTRAPMETHODS;
    if (strncmp("RuntimeVisibleTypeAnnotations", str, len) == 0)
        return TAG_ATTR_RUNTIMEVISIBLETYPEANNOTATIONS;
    if (strncmp("RuntimeInvisibleTypeAnnotations", str, len) == 0)
        return TAG_ATTR_RUNTIMEINVISIBLETYPEANNOTATIONS;
    if (strncmp("MethodParameters", str, len) == 0)
        return TAG_ATTR_METHODPARAMETERS;
    return 0;
}
