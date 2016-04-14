#ifndef RT_H
#define	RT_H

#include "java.h"

#ifndef	__cplusplus
#error C++ TOOLCHAIN IS NEEDED TO COMPILE CRUISER!
#endif

class rt_Class;
class rt_Field;
class rt_Method;

typedef struct
{
    u2              off_parameter_descriptor;
    u2              len_parameter_descirptor;
    // TODO
    // If the value of the name_index item is zero,
    // then this parameters element
    // indicates a formal parameter with no name.
    // WTF???
    u2              name_index;
    u2              name_length;
    u1 *            name_bytes;
    // The value of the access_flags item is as follows:
    // 0x0010 (ACC_FINAL)
    // 0x1000 (ACC_SYNTHETIC)
    // 0x8000 (ACC_MANDATED)
    u2              access_flags;
}                                   rt_Parameter;

typedef struct
{
    u2              start_pc;
    u2              length;
    u2              name_index;
    u2              descriptor_index;
    u2              signature_index;
    u2              index;
}                                   rt_Local;

typedef struct
{
    // value type
    u2              descriptor_index;
    u2              off_descriptor;
    u2              len_descriptor;
}                                   rt_Value;

typedef struct
{
    u2              off_return_descriptor;
    u2              len_return_descriptor;
    u1              parameters_count;
    u1              parameters_length;
    rt_Parameter *  parameters;
}                                   rt_Descriptor;
typedef struct
{
    u2              attributes_count;
    attr_info *     attributes;
    u4              attributes_mark;
}                                   rt_Attributes;

typedef struct
{
    // NULL placeholder
    rt_Method *     caller;
    // the method which this frame belongs to
    rt_Method *     callee;
    // when a local variable or
    // a parameter is referenced,
    // `locals_offset` is used to retrieve
    // its offset in `locals`
    u2 *            locals_offset;
    rt_Local *      locals;
    u2 *            stack_offset;
    rt_Value *      stack;
}                                   rt_Frame;


class rt_Accessible
{
public:
    u2              getAccessFlags();
    rt_Attributes * getAttributes();
protected:
    attr_info *     getAttribute(u2);
    attr_info *     getAttribute(u2, u4);
private:
    u2              access_flags;
    rt_Attributes   attributes;
};

/*
 * Run-time structures and functions
 */
class rt_Class :
    public rt_Accessible
{
public:
    u2              getFieldsCount();
    rt_Field *      getFields();
    u2              getMethodsCount();
    rt_Method *     getMethods();

    u1              getConstantTag(u2);
    const_Class_data *
                    getConstant_Class(u2);
    const_Fieldref_data *
                    getConstant_Fieldref(u2);
    const_Methodref_data *
                    getConstant_Methodref(u2);
    const_InterfaceMethodref_data *
                    getConstant_InterfaceMethodref(u2);
    const_String_data *
                    getConstant_String(u2);
    const_Integer_data *
                    getConstant_Integer(u2);
    const_Float_data *
                    getConstant_Float(u2);
    const_Long_data *
                    getConstant_Long(u2);
    const_Double_data *
                    getConstant_Double(u2);
    const_Utf8_data *
                    getConstant_Utf8(u2);
    const_MethodHandle_data *
                    getConstant_MethodHandle(u2);
    const_MethodType_data *
                    getConstant_MethodType(u2);
    const_InvokeDynamic_data *
                    getConstant_InvokeDynamic(u2);
private:
    int             hash;
    u2              constant_pool_count;
    cp_info *       constant_pool;
    u2              this_class;
    u2              super_class;
    u2              interfaces_count;
    u2 *            interfaces;
    u2              fields_count;
    rt_Field *      fields;
    u2              methods_count;
    rt_Method *     methods;

#if VER_CMP(45, 3)
    u2 off_InnerClasses;
#endif
#if VER_CMP(49, 0)
    u2 off_EnclosingMethod;
    u2 off_RuntimeVisibleAnnotations;
    u2 off_RuntimeInvisibleAnnotations;
#endif
#if VER_CMP(51, 0)
    u2 off_BootstrapMethods;
#endif
#if VER_CMP(52, 0)
    u2 off_RuntimeVisibleTypeAnnotations;
    u2 off_RuntimeInvisibleTypeAnnotations;
#endif
public:
    bool            isInterface();
    bool            isAnnotation();
    bool            isSynthetic();
    const_Class_data *
                    getThisClass();
    const_Class_data *
                    getSuperClass();
    const_Utf8_data *
                    getClassName();
    u2              getInterfacesCount();
    const_Class_data **
                    getInterfaces(const_Class_data **);
}; // rt_Class

class rt_Member
{
public:
    const_Utf8_data *
                    getName();
    const_Utf8_data *
                    getDescriptor();
    rt_Class *      getDefClass();
private:
    rt_Class *      def_class;
    u2              name_index;
    u2              descriptor_index;
};

class rt_Field :
    public rt_Accessible,
    public rt_Member
{
#if VER_CMP(45, 3)
    u2              off_ConstantValue;
#endif
#if VER_CMP(49, 0)
    u2              off_RuntimeVisibleAnnotations;
    u2              off_RuntimeInvisibleAnnotations;
#endif
#if VER_CMP(52, 0)
    u2              off_RuntimeVisibleTypeAnnotations;
    u2              off_RuntimeInvisibleTypeAnnotations;
#endif
}; // rt_Field

class rt_Method :
    public rt_Accessible,
    public rt_Member
{
private:
    attr_Code_info *getAttribute_Code(u2);
public:
                    rt_Method(method_info *);
    void            initFrame(rt_Frame *);
private:
    rt_Descriptor   descriptor;

#if VER_CMP(45, 3)
    u2              off_Code;
    u2              off_Exceptions;
#endif
#if VER_CMP(49, 0)
    u2              off_RuntimeVisibleParameterAnnotations;
    u2              off_RuntimeInvisibleParameterAnnotations;
    u2              off_AnnotationDefault;
    u2              off_RuntimeVisibleAnnotations;
    u2              off_RuntimeInvisibleAnnotations;
#endif
#if VER_CMP(52, 0)
    u2              off_RuntimeVisibleTypeAnnotations;
    u2              off_RuntimeInvisibleTypeAnnotations;
#endif
}; // rt_Method

#endif	/* RT_H */

