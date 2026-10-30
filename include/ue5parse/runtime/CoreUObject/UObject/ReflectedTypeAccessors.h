#pragma once

namespace ue5parse::runtime {

class UClass;
class UEnum;
class UScriptStruct;

// The reflected type of a C++ type (UE's ReflectedTypeAccessors.h). UnrealHeaderTool defines StaticEnum for each UENUM.
template <class T>
UClass* StaticClass() {
    return T::StaticClass();
}

template <class T>
UScriptStruct* StaticStruct() {
    return T::StaticStruct();
}

template <class T>
UEnum* StaticEnum();

}
