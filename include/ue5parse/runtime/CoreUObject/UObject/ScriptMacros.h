#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"

// The parameter macros of native function thunks, as UE's ScriptMacros.h (usable in any namespace). A thunk reads each
// parameter from the caller's bytecode (or the compiled-in property chain), then P_FINISH steps over EX_EndFunctionParms:
//
//   DEFINE_FUNCTION(UKismetMathLibrary::execAdd_IntInt) {
//       P_GET_PROPERTY(FIntProperty, A);
//       P_GET_PROPERTY(FIntProperty, B);
//       P_FINISH;
//       *(int32*)RESULT_PARAM = UKismetMathLibrary::Add_IntInt(A, B);
//   }

#define PARAM_PASSED_BY_VAL(ParamName, ParamType)                                                                      \
    ParamType ParamName{};                                                                                             \
    Stack.StepCompiledIn(&ParamName);
#define PARAM_PASSED_BY_REF(ParamName, ParamType)                                                                      \
    ParamType ParamName##Temp{};                                                                                       \
    ParamType& ParamName = Stack.StepCompiledInRef<ParamType>(&ParamName##Temp);

#define P_GET_PROPERTY(PropertyType, ParamName)                                                                        \
    PropertyType::TCppType ParamName = PropertyType::GetDefaultPropertyValue();                                        \
    Stack.StepCompiledIn(&ParamName);
#define P_GET_PROPERTY_REF(PropertyType, ParamName)                                                                    \
    PropertyType::TCppType ParamName##Temp = PropertyType::GetDefaultPropertyValue();                                  \
    PropertyType::TCppType& ParamName = Stack.StepCompiledInRef<PropertyType::TCppType>(&ParamName##Temp);
// A bool reads through a 32-bit temporary, as in UE.
#define P_GET_UBOOL(ParamName)                                                                                         \
    ::ue5parse::runtime::uint32 ParamName##32 = 0;                                                                                          \
    Stack.StepCompiledIn(&ParamName##32);                                                                              \
    bool ParamName = (ParamName##32 & 0xff) != 0;
#define P_GET_UBOOL_REF(ParamName) PARAM_PASSED_BY_REF(ParamName, bool)
#define P_GET_STRUCT(StructType, ParamName) PARAM_PASSED_BY_VAL(ParamName, StructType)
#define P_GET_STRUCT_REF(StructType, ParamName) PARAM_PASSED_BY_REF(ParamName, StructType)
#define P_GET_OBJECT(ObjectType, ParamName)                                                                            \
    ObjectType* ParamName = nullptr;                                                                                   \
    Stack.StepCompiledIn(&ParamName);
#define P_GET_OBJECT_REF(ObjectType, ParamName)                                                                        \
    ObjectType* ParamName##Temp = nullptr;                                                                             \
    ObjectType*& ParamName = Stack.StepCompiledInRef<ObjectType*>(&ParamName##Temp);
#define P_GET_TARRAY(ElementType, ParamName) PARAM_PASSED_BY_VAL(ParamName, ::ue5parse::runtime::TArray<ElementType>)
#define P_GET_TARRAY_REF(ElementType, ParamName) PARAM_PASSED_BY_REF(ParamName, ::ue5parse::runtime::TArray<ElementType>)
#define P_GET_TINTERFACE(ParamName) PARAM_PASSED_BY_VAL(ParamName, ::ue5parse::runtime::FScriptInterface)
#define P_GET_TINTERFACE_REF(ParamName) PARAM_PASSED_BY_REF(ParamName, ::ue5parse::runtime::FScriptInterface)
#define P_GET_WEAKOBJECT(ParamName) PARAM_PASSED_BY_VAL(ParamName, ::ue5parse::runtime::FWeakObjectPtr)
#define P_GET_SOFTOBJECT(ParamName) PARAM_PASSED_BY_VAL(ParamName, ::ue5parse::runtime::FSoftObjectPtr)
#define P_GET_SOFTOBJECT_REF(ParamName) PARAM_PASSED_BY_REF(ParamName, ::ue5parse::runtime::FSoftObjectPtr)
#define P_GET_TFIELDPATH(ParamName) PARAM_PASSED_BY_VAL(ParamName, ::ue5parse::runtime::FFieldPath)
#define P_GET_ENUM(EnumType, ParamName)                                                                                \
    EnumType ParamName = (EnumType)0;                                                                                  \
    Stack.StepCompiledIn(&ParamName);
#define P_GET_ENUM_REF(EnumType, ParamName) PARAM_PASSED_BY_REF(ParamName, EnumType)
#define P_FINISH Stack.Code += !!Stack.Code;
#define P_THIS_OBJECT (Context)
#define P_THIS_CAST(ClassType) ((ClassType*)P_THIS_OBJECT)
#define P_THIS P_THIS_CAST(ThisClass)
#define P_NATIVE_BEGIN {
#define P_NATIVE_END }
