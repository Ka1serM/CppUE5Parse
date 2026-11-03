#include "ue5parse/runtime/Engine/Materials/MaterialParameterCollection.h"

// UE's Materials/MaterialParameterCollection.cpp (UE 5.3).

namespace ue5parse::runtime {

const FCollectionScalarParameter* UMaterialParameterCollection::GetScalarParameterByName(const FName ParameterName) const {
    for (const FCollectionScalarParameter& Parameter : ScalarParameters)
        if (Parameter.ParameterName == ParameterName)
            return &Parameter;
    return nullptr;
}

const FCollectionVectorParameter* UMaterialParameterCollection::GetVectorParameterByName(const FName ParameterName) const {
    for (const FCollectionVectorParameter& Parameter : VectorParameters)
        if (Parameter.ParameterName == ParameterName)
            return &Parameter;
    return nullptr;
}

bool UMaterialParameterCollectionInstance::SetScalarParameterValue(const FName ParameterName, const float ParameterValue) {
    if (Collection == nullptr || Collection->GetScalarParameterByName(ParameterName) == nullptr)
        return false;
    ScalarParameterValues.FindOrAdd(ParameterName) = ParameterValue;
    return true;
}

bool UMaterialParameterCollectionInstance::SetVectorParameterValue(const FName ParameterName, const FLinearColor& ParameterValue) {
    if (Collection == nullptr || Collection->GetVectorParameterByName(ParameterName) == nullptr)
        return false;
    VectorParameterValues.FindOrAdd(ParameterName) = ParameterValue;
    return true;
}

bool UMaterialParameterCollectionInstance::GetScalarParameterValue(const FName ParameterName, float& OutParameterValue) const {
    const FCollectionScalarParameter* Parameter = Collection != nullptr ? Collection->GetScalarParameterByName(ParameterName) : nullptr;
    if (Parameter == nullptr)
        return false;
    const float* Value = ScalarParameterValues.Find(ParameterName);
    OutParameterValue = Value != nullptr ? *Value : Parameter->DefaultValue;
    return true;
}

bool UMaterialParameterCollectionInstance::GetVectorParameterValue(const FName ParameterName, FLinearColor& OutParameterValue) const {
    const FCollectionVectorParameter* Parameter = Collection != nullptr ? Collection->GetVectorParameterByName(ParameterName) : nullptr;
    if (Parameter == nullptr)
        return false;
    const FLinearColor* Value = VectorParameterValues.Find(ParameterName);
    OutParameterValue = Value != nullptr ? *Value : Parameter->DefaultValue;
    return true;
}

}
