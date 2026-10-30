#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtr.h"
#include "ue5parse/runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class UFunction;

// FScriptInterface: an object and the address of its interface (the object itself for a Blueprint interface).
struct FScriptInterface {
    UObject* GetObject() const { return ObjectPointer; }
    UObject*& GetObjectRef() { return ObjectPointer; }
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

// FScriptDelegate: a weak object and a function name, as UE's TScriptDelegate.
class FScriptDelegate {
public:
    void BindUFunction(UObject* InObject, FName InFunctionName);
    void Unbind() {
        Object.Reset();
        FunctionName = FName();
    }
    // Bound to a live object that has the function.
    bool IsBound() const;
    bool IsBoundToObject(const UObject* InObject) const;
    bool IsCompactable() const { return FunctionName.isNone() || !Object.Get(); }
    UObject* GetUObject() const { return Object.Get(); }
    UObject* GetUObjectEvenIfUnreachable() const { return Object.GetEvenIfUnreachable(); }
    FName GetFunctionName() const { return FunctionName; }
    // Calls the function with a parameter struct laid out as its signature. Does nothing when not bound.
    void ProcessDelegate(void* Parameters) const;
    bool operator==(const FScriptDelegate& Other) const {
        return Object == Other.Object && FunctionName == Other.FunctionName;
    }

private:
    FWeakObjectPtr Object;
    FName FunctionName;
};

// FMulticastScriptDelegate: the invocation list of an event dispatcher.
class FMulticastScriptDelegate {
public:
    bool IsBound() const;
    bool Contains(const FScriptDelegate& Delegate) const { return InvocationList.Contains(Delegate); }
    // Adds the delegate unless it is in the list.
    void AddUnique(const FScriptDelegate& Delegate);
    void Add(const FScriptDelegate& Delegate);
    void Remove(const FScriptDelegate& Delegate);
    void Remove(const UObject* InObject, FName InFunctionName);
    void RemoveAll(const UObject* Object);
    void Clear() { InvocationList.Empty(); }
    // Calls every bound delegate with the parameters, in order, on a copy of the list (UE's
    // ProcessMulticastDelegate), then drops delegates whose objects are gone.
    void ProcessMulticastDelegate(void* Parameters) const;
    TArray<UObject*> GetAllObjects() const;
    bool operator==(const FMulticastScriptDelegate& Other) const { return InvocationList == Other.InvocationList; }

    mutable TArray<FScriptDelegate> InvocationList;
};

}
