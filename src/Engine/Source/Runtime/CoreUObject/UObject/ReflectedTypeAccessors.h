#pragma once

namespace ue5parse::runtime {

class UClass;
class UEnum;
class UScriptStruct;

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
