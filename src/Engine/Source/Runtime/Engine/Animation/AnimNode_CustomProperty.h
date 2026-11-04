#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimNodeBase.h"

#include "AnimNode_CustomProperty.generated.h"

namespace ue5parse::runtime {

class FProperty;

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
    UPROPERTY()
    TArray<FName> SourcePropertyNames;
    UPROPERTY()
    TArray<FName> DestPropertyNames;
    UPROPERTY()
    TObjectPtr<UObject> TargetInstance = nullptr;
    TArray<FProperty*> SourceProperties;
    TArray<FProperty*> DestProperties;

    virtual void InitializeProperties(const UObject* InSourceInstance, UClass* InTargetClass);
    virtual void PropagateInputProperties(const UObject* InSourceInstance);
    virtual UClass* GetTargetClass() const { return nullptr; }
};

}
