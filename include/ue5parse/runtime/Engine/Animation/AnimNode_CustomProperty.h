#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_CustomProperty.generated.h"

namespace ue5parse::runtime {

class FProperty;

// A node that runs another object and copies properties of its own instance to it (UE's
// Animation/AnimNode_CustomProperty.h).
USTRUCT()
struct FAnimNode_CustomProperty : public FAnimNode_Base {
    GENERATED_BODY()
public:
    void SetTargetInstance(UObject* InInstance) { TargetInstance = InInstance; }
    template <class T>
    T* GetTargetInstance() const {
        return TargetInstance ? Cast<T>(TargetInstance) : nullptr;
    }

protected:
    // The properties of the source instance, one to one with the properties of the target class they copy to.
    UPROPERTY()
    TArray<FName> SourcePropertyNames;
    UPROPERTY()
    TArray<FName> DestPropertyNames;
    UPROPERTY()
    UObject* TargetInstance = nullptr;
    TArray<FProperty*> SourceProperties;
    TArray<FProperty*> DestProperties;

    virtual void InitializeProperties(const UObject* InSourceInstance, UClass* InTargetClass);
    virtual void PropagateInputProperties(const UObject* InSourceInstance);
    virtual UClass* GetTargetClass() const { return nullptr; }
};

}
