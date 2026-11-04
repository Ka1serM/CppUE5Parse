#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/WeakObjectPtr.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class UFunction;

struct FScriptInterface {
    UObject* GetObject() const { return ObjectPointer; }
    void* GetInterface() const { return ObjectPointer ? InterfacePointer : nullptr; }
    void SetObject(UObject* Object) {
        ObjectPointer = Object;
        if (Object == nullptr) InterfacePointer = nullptr;
    }
    void SetInterface(void* Interface) { InterfacePointer = Interface; }
    bool operator==(const FScriptInterface& Other) const {
        return ObjectPointer == Other.ObjectPointer && GetInterface() == Other.GetInterface();
    }

    UObject* ObjectPointer = nullptr;
    void* InterfacePointer = nullptr;
};

class FScriptDelegate {
public:
    void BindUFunction(UObject* InObject, FName InFunctionName);
    void Unbind() {
        Object.Reset();
        FunctionName = FName();
    }
    bool IsBound() const;
    bool IsBoundToObject(const UObject* InObject) const;
    bool IsCompactable() const { return FunctionName.isNone() || !Object.Get(); }
    UObject* GetUObject() const { return Object.Get(); }
    UObject* GetUObjectEvenIfUnreachable() const { return Object.GetEvenIfUnreachable(); }
    FName GetFunctionName() const { return FunctionName; }
    void ProcessDelegate(void* Parameters) const;
    bool operator==(const FScriptDelegate& Other) const {
        return Object == Other.Object && FunctionName == Other.FunctionName;
    }

private:
    FWeakObjectPtr Object;
    FName FunctionName;
};

class FMulticastScriptDelegate {
public:
    bool IsBound() const;
    bool Contains(const FScriptDelegate& Delegate) const { return InvocationList.Contains(Delegate); }
    void AddUnique(const FScriptDelegate& Delegate);
    void Add(const FScriptDelegate& Delegate);
    void Remove(const FScriptDelegate& Delegate);
    void Remove(const UObject* InObject, FName InFunctionName);
    void RemoveAll(const UObject* Object);
    void Clear() { InvocationList.Empty(); }
    void ProcessMulticastDelegate(void* Parameters) const;
    bool operator==(const FMulticastScriptDelegate& Other) const { return InvocationList == Other.InvocationList; }

    mutable TArray<FScriptDelegate> InvocationList;
};

}
