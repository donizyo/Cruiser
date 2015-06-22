/* 
 * File:   java.h
 * Author: donizyo
 *
 * Created on April 23, 2015, 7:13 PM
 */

#ifndef JAVA_H
#define	JAVA_H

#include <zip.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define MINOR_VERSION              3
#define MAJOR_VERSION              45
    
#if (defined MAJOR_VERSION && defined MINOR_VERSION)
#define VER_CMP(major, minor)  (MAJOR_VERSION > major || MAJOR_VERSION == major && MINOR_VERSION >= minor)
#else
#error "Macro 'MAJOR_VERSION' and 'MINOR_VERSION' is missing!"
#endif

    typedef unsigned char u1;
    typedef unsigned short u2;
    typedef unsigned int u4;
    typedef unsigned long long u8;

#define CONSTANT_Class                  7
#define CONSTANT_Fieldref               9
#define CONSTANT_Methodref              10
#define CONSTANT_InterfaceMethodref     11
#define CONSTANT_String                 8
#define CONSTANT_Integer                3
#define CONSTANT_Float                  4
#define CONSTANT_Long                   5
#define CONSTANT_Double                 6
#define CONSTANT_NameAndType            12
#define CONSTANT_Utf8                   1
#define CONSTANT_MethodHandle           15
#define CONSTANT_MethodType             16
#define CONSTANT_InvokeDynamic          18

// Class access
#define ACC_PUBLIC                      0x0001 // Declared public
#define ACC_FINAL                       0x0010 // Declared final
#define ACC_SUPER                       0x0020 // Tread superclass methods specially when invoked by the invokespecial instruction
#define ACC_INTERFACE                   0x0200 // Is an interface, not a class
#define ACC_ABSTRACT                    0x0400 // Declared abstract; must not be instantiated
#define ACC_SYNTHETIC                   0x1000 // Declared synthetic; not present in the source code
#define ACC_ANNOTATION                  0x2000 // Declared as an annotation type
#define ACC_ENUM                        0x4000 // Declared as an enum type
    
// Field access (Additional)
#define ACC_PRIVATE                     0x0002 // Declared private
#define ACC_PROTECTED                   0x0004 // Declared protected
#define ACC_STATIC                      0x0008 // Declared static
#define ACC_VOLATILE                    0x0040 // Declared volatile; cannot be cached
#define ACC_TRANSIENT                   0x0080 // Declared transient

// Method access (additional)
#define ACC_SYNCHRONIZED                0x0020 // Declared synchronized; invocation is wrapped by a monitor use
#define ACC_BRIDGE                      0x0040 // A bridge method, generated by the compiler
#define ACC_VARARGS                     0x0080 // Declared with variable number of arguments
#define ACC_NATIVE                      0x0100 // Declared native; implemented in a language other than Java
#define ACC_STRICT                      0x0800 // Declared strictfp; floating-point mode is FP-strict

#define ACC_CLASS                       (ACC_PUBLIC | ACC_FINAL | ACC_SUPER | ACC_INTERFACE | ACC_ABSTRACT | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM)
#define ACC_NESTED_CLASS                (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL | ACC_INTERFACE | ACC_ABSTRACT | ACC_SYNTHETIC | ACC_ANNOTATION | ACC_ENUM)
#define ACC_FIELD                       (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL | ACC_VOLATILE | ACC_TRANSIENT | ACC_SYNTHETIC | ACC_ENUM)
#define ACC_METHOD                      (ACC_PUBLIC | ACC_PRIVATE | ACC_PROTECTED | ACC_STATIC | ACC_FINAL | ACC_SYNCHRONIZED | ACC_BRIDGE | ACC_VARARGS | ACC_NATIVE | ACC_ABSTRACT | ACC_STRICT | ACC_SYNTHETIC)

#define VER_ATTR_CONSTANTVALUE                          45.3f
#define VER_ATTR_CODE                                   45.3f
#define VER_ATTR_STACKMAPTABLE                          50.0f
#define VER_ATTR_EXCEPTIONS                             45.3f
#define VER_ATTR_INNERCLASSES                           45.3f
#define VER_ATTR_ENCLOSINGMETHOD                        49.0f
#define VER_ATTR_SYNTHETIC                              45.3f
#define VER_ATTR_SIGNATURE                              49.0f
#define VER_ATTR_SOURCEFILE                             45.3f
#define VER_ATTR_SOURCEDEBUGEXTENSION                   49.0f
#define VER_ATTR_LINENUMBERTABLE                        45.3f
#define VER_ATTR_LOCALVARIABLETABLE                     45.3f
#define VER_ATTR_LOCALVARIABLETYPETABLE                 49.0f
#define VER_ATTR_DEPRECATED                             45.3f
#define VER_ATTR_RUNTIMEVISIBLEANNOTATIONS              49.0f
#define VER_ATTR_RUNTIMEINVISIBLEANNOTATIONS            49.0f
#define VER_ATTR_RUNTIMEVISIBLEPARAMETERANNOTATIONS     49.0f
#define VER_ATTR_RUNTIMEINVISIBLEPARAMETERANNOTATIONS   49.0f
#define VER_ATTR_ANNOTATIONDEFAULT                      49.0f
#define VER_ATTR_BOOTSTRAPMETHODS                       51.0f

// Cruise-specific constants
#define TAG_ATTR_CONSTANTVALUE                          0x1
#define TAG_ATTR_CODE                                   0x2
#define TAG_ATTR_STACKMAPTABLE                          0x4
#define TAG_ATTR_EXCEPTIONS                             0x8
#define TAG_ATTR_INNERCLASSES                           0x10
#define TAG_ATTR_ENCLOSINGMETHOD                        0x20
#define TAG_ATTR_SYNTHETIC                              0x40
#define TAG_ATTR_SIGNATURE                              0x80
#define TAG_ATTR_SOURCEFILE                             0x100
#define TAG_ATTR_SOURCEDEBUGEXTENSION                   0x200
#define TAG_ATTR_LINENUMBERTABLE                        0x400
#define TAG_ATTR_LOCALVARIABLETABLE                     0x800
#define TAG_ATTR_LOCALVARIABLETYPETABLE                 0x1000
#define TAG_ATTR_DEPRECATED                             0x2000
#define TAG_ATTR_RUNTIMEVISIBLEANNOTATIONS              0x4000
#define TAG_ATTR_RUNTIMEINVISIBLEANNOTATIONS            0x8000
#define TAG_ATTR_RUNTIMEVISIBLEPARAMETERANNOTATIONS     0x10000
#define TAG_ATTR_RUNTIMEINVISIBLEPARAMETERANNOTATIONS   0x20000
#define TAG_ATTR_ANNOTATIONDEFAULT                      0x40000
#define TAG_ATTR_BOOTSTRAPMETHODS                       0x80000

    typedef struct
    {
        u1 tag;
        void * data;
    } cp_info;

    typedef struct
    {
        u2 attribute_name_index;
        u4 attribute_length;
        // temporary storage
        u1 *info;
        // Cruise-specific members
        u2 tag;
        void *data;
    } attr_info;

#if VER_CMP(45, 3)
    struct attr_SourceFile_info
    {
        u2 sourcefile_index;
    };
    struct attr_InnerClasses_info
    {
        u2 number_of_classes;
        struct
        {
            u2 inner_class_info_index;
            u2 outer_class_info_index;
            u2 inner_name_index;
            u2 inner_class_access_flags;
        } *classes;
    };
    struct attr_Synthetic_info {};
    struct attr_Deprecated_info {};
    struct line_number_table_entry
    {
        u2 start_pc;
        u2 line_number;
    };
    struct attr_LineNumberTable_info
    {
        u2 line_number_table_length;
        struct line_number_table_entry line_number_table[];
    };
    struct local_variable_table_entry
    {
        u2 start_pc;
        u2 length;
        u2 name_index;
        u2 descriptor_index;
        u2 index;
    };
    struct attr_LocalVariableTable_info
    {
        u2 local_variable_table_length;
        struct local_variable_table_entry local_variable_table[];
    };

    struct attr_ConstantValue_info
    {
        u2 constantvalue_index;
    };

    struct attr_Code_info
    {
        u2 max_stack;
        u2 max_locals;
        u4 code_length;
        u1 *code;
        u2 exception_table_length;
        struct
        {
            u2 start_pc;
            u2 end_pc;
            u2 handler_pc;
            u2 catch_type;
        } *exception_table;
        u2 attributes_count;
        attr_info *attributes;
    };

    struct attr_Exceptions_info
    {
        u2 number_of_exceptions;
        u2 *exception_index_table;
    };
#endif /* VERSION 45.3 */
#if VER_CMP(49, 0)
    struct attr_EnclosingMethod_info
    {
        u2 class_index;
        u2 method_index;
    };
    struct attr_RuntimeVisibleParameterAnnotations;
    struct attr_RuntimeInvisibleParameterAnnotations;
    struct attr_AnnotationDefault_info;
    struct attr_Signature_info
    {
        u2 signature_index;
    };
    struct attr_RuntimeVisibleAnnotations;
    struct attr_RuntimeInvisibleAnnotations;
    struct local_variable_type_table_entry
    {
        u2 start_pc;
        u2 length;
        u2 name_index;
        u2 signature_index;
        u2 index;
    };
    struct attr_LocalVariableTypeTable_info
    {
        u2 local_variable_type_table_length;
        struct local_variable_type_table_entry local_variable_type_table[];
    };
#endif /* VERSION 49.0 */
#if VER_CMP(50, 0)
    struct attr_StackMapTable_info
    {
        u2 number_of_entries;
        struct stack_map_frame
        {
        } *entries;
    };
#endif /* VERSION 50.0 */
#if VER_CMP(51, 0)
    struct BootstrapMethods;
#endif /* VERSION 51.0 */
#if VER_CMP(52, 0)
    struct MethodParameters;
    struct RuntimeVisibleTypeAnnotations;
    struct RuntimeInvisibleTypeAnnotations;
#endif /* VERSION 52.0 */

    typedef struct {
        u2 access_flags;            // 0x1, 0x2, 0x4, 0x8, 0x10, 0x40, 0x80, 0x1000, 0x4000
        u2 name_index;              // CONSTANT_Utf8_info
        u2 descriptor_index;        // CONSTANT_Utf8_info
        u2 attributes_count;
        attr_info * attributes;
    } field_info, method_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u2 name_index;
        } * data;
    } CONSTANT_Class_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u2 class_index;
            u2 name_and_type_index;
        } * data;
    } CONSTANT_Fieldref_info,
        CONSTANT_Methodref_info,
        CONSTANT_InterfaceMethodref_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u2 string_index;
        } * data;
    } CONSTANT_String_info;
    
    typedef struct
    {
        u1 tag;
        union
        {
            u4 bytes;
            float float_value;
        } * data;
    } CONSTANT_Integer_info,
        CONSTANT_Float_info;
    
    typedef struct
    {
        u1 tag;
        union
        {
            struct
            {
                u4 low_bytes;
                u4 high_bytes;
            };
            u8 long_value;
            double double_value;
        } * data;
    } CONSTANT_Long_info,
        CONSTANT_Double_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u2 name_index;
            u2 descriptor_index;
        } * data;
    } CONSTANT_NameAndType_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u2 length;
            u1 * bytes;
        } * data;
    } CONSTANT_Utf8_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u1 reference_kind;
            u2 reference_index;
        } * data;
    } CONSTANT_MethodHandle_info;
    
    typedef struct
    {
        u1 tag;
        struct {
            u2 descriptor_index;
        } * data;
    } CONSTANT_MethodType_info;

    typedef struct
    {
        u1 tag;
        struct {
            u2 bootstrap_method_attr_index;
            u2 name_and_type_index;
        } * data;
    } CONSTANT_InvokeDynamic_info;

    typedef struct
    {
        u4 magic;
        u2 minor_version, major_version;
        u2 constant_pool_count;
        cp_info * constant_pool;
        /*
         * The value of the access_flags item is a mask
         * of flags used to denote access permissions to
         * and properties of this class or interface.
         */
        u2 access_flags;
        /*
         * The value of this_class must be
         * a valid index into the constant_pool table.
         * The constant_pool entry at that index must be
         * a CONSTANT_Class_info structure
         */
        u2 this_class;
        /*
         * The value of super_class item must be zero
         * or must be a valid index into
         * the constant_pool table. If the value of
         * the super_class item is nonzero, the constant_pool
         * entry at that index must be a CONSTANT_Class_info
         * structure presenting the direct superclass
         * of the class defined by this class file. Neither
         * the direct superclass nor any of its superclasses
         * may have the ACC_FINAL flag set in the access_flags
         * item of its ClassFile structure.
         * If the value of the super_class item is zero, then
         * this class file must represent the class Object,
         * the only class or interface without
         * a direct superclass.
         */
        u2 super_class;
        /*
         * The value of the interfaces_count item gives
         * the number of direct superinterfaces of
         * this class or interface type.
         */
        u2 interfaces_count;
        /*
         * Each value in the interfaces array must be
         * a valid index into the constant_pool table.
         * The constant_pool entry at each value of
         * interfaces[i], where 0 <= i < interfaces_count,
         * must be a CONSTANT_Class_info structure
         * representing an interface that is a direct
         * superinterface of this class or interface type,
         * in ther left-to right ordr given in the source
         * for the type.
         */
        u2 * interfaces;
        /*
         * The value of the fields_count item gives
         * the number of field_info structures
         * in the fields table. The field_info structures
         * represent all fields, both class variables and
         * instance variables, declared by this class or
         * interface type.
         */
        u2 fields_count;
        /*
         * Each value in the fields table must be
         * a field_info structure giving a complete
         * description of a field in this class or
         * interface. The fields table includes only
         * those fields that are declared by this class
         * or interface. It does not include items
         * representing fields that are inherited
         * from superclasses or superinterfaces.
         */
        field_info * fields;
        /*
         * The value of the methods_count item gives
         * the number of method_info structures in
         * the methods table.
         */
        u2 methods_count;
        /*
         * Each value in the methods table must be a method_info
         * structure giving a complete description of a method
         * in this class or interface. If neither of the
         * ACC_NATIVE and ACC_ABSTRACT flags are set in the
         * access_flags item of a method_info structure,
         * the JVM instructions implementing the method are
         * also supplied.
         * The method_info structures represent all methods
         * declared by this class or interface type,
         * including instance methods, class methods,
         * instance initialization methods, and any class
         * or interface initialization method. The methods
         * table does not include items representing
         * methods that are inherited from superclasses
         * or superinterfaces.
         */
        method_info * methods;
        /*
         * The value of the attributes_count item gives
         * the number of attributes in the attributes
         * table of this class.
         */
        u2 attributes_count;
        /*
         * Each value of the attributes table must be
         * an attribute_info structure.
         * The attributes defined by this specification
         * as appearing in the attributes table of
         * a ClassFile structure are the InnerClasses,
         * EnclosingMethod, Synthetic, Signature, SourceFile,
         * SourceDebugExtension, Deprecated,
         * RuntimeVisibleAnnotations,
         * RuntimeInvisibleAnnotations,
         * BootstrapMethods attributes.
         * If a JVM implementation recognizes class files whose
         * version number is 49.0 or above, it must recognize
         * and correctly read Signature, RuntimeVisibleAnnotations,
         * and RuntimeInvisibleAnnotations attributes found in the
         * attributes table of a ClassFile structure of a class
         * file whose version number is 49.0 or above.
         * If a JVM implementation recognizes class files whose
         * version number is 51.0 or above, it must recognize
         * and correctly read BootstrapMethods attributes found in
         * the attributes table of a ClassFile structure of a class
         * file whose version number is 51.0 or above.
         * A JVM implementation is required to silently ignore any
         * or all attributes in the attributes table of a ClassFile
         * structure that it does not recognize. Attributes not
         * defined in this specification are not allowed to affect
         * the semantics of the class file, but only to provide
         * additional descriptive information.
         */
        attr_info * attributes;
    } ClassFile;

    extern CONSTANT_Class_info *getConstant_Class(ClassFile *, u2);
    extern CONSTANT_Fieldref_info *getConstant_Fieldref(ClassFile *, u2);
    extern CONSTANT_Methodref_info *getConstant_Methodref(ClassFile *, u2);
    extern CONSTANT_InterfaceMethodref_info *getConstant_InterfaceMethodref(ClassFile *, u2);
    extern CONSTANT_String_info *getConstant_String(ClassFile *, u2);
    extern CONSTANT_Integer_info *getConstant_Integer(ClassFile *, u2);
    extern CONSTANT_Float_info *getConstant_Float(ClassFile *, u2);
    extern CONSTANT_Long_info *getConstant_Long(ClassFile *, u2);
    extern CONSTANT_Double_info *getConstant_Double(ClassFile *, u2);
    extern CONSTANT_NameAndType_info *getConstant_NameAndType(ClassFile *, u2);
    extern CONSTANT_Utf8_info *getConstant_Utf8(ClassFile *, u2);
    extern CONSTANT_MethodHandle_info *getConstant_MethodHandle(ClassFile *, u2);
    extern CONSTANT_MethodType_info *getConstant_MethodType(ClassFile *, u2);
    extern CONSTANT_InvokeDynamic_info *getConstant_InvokeDynamic(ClassFile *, u2);

    extern char *getConstant_Utf8String(ClassFile *, u2);

    struct BufferInput;
    typedef char *(*func_fillBuffer)(struct BufferInput *, int);

    // Encapsulate FILE and struct zip_file pointer
    struct BufferInput
    {
        union {
            struct zip_file *entry;
            FILE *file;
        };
        int bufsize;
        char *buffer;
        int bufsrc;
        int bufdst;
        func_fillBuffer fp;
        char more;
    };

    extern int loadAttributes_class(ClassFile *, struct BufferInput *, u2 *, attr_info **);
    extern int loadAttributes_field(ClassFile *, struct BufferInput *, u2 *, attr_info **);
    extern int loadAttributes_method(ClassFile *, struct BufferInput *, u2 *, attr_info **);
    extern int loadAttributes_code(ClassFile *, struct BufferInput *, u2 *, attr_info **);

    extern int freeAttributes_class(u2, attr_info *);
    extern int freeAttributes_field(u2, attr_info *);
    extern int freeAttributes_method(u2, attr_info *);
    extern int freeAttributes_code(u2, attr_info *);

    extern int ru1(u1 *, struct BufferInput *);
    extern int ru2(u2 *, struct BufferInput *);
    extern int ru4(u4 *, struct BufferInput *);
    extern int rbs(char *, struct BufferInput *, int);
    extern int skp(struct BufferInput *, int);
    extern char *fillBuffer_f(struct BufferInput *, int);
    extern char *fillBuffer_z(struct BufferInput *, int);

    extern int parseClassfile(struct BufferInput *, ClassFile *);

    extern int freeClassfile(ClassFile *);

    extern int compareVersion(u2, u2);
    extern int checkInput(struct BufferInput *);
#ifdef	__cplusplus
}
#endif

#endif	/* JAVA_H */

