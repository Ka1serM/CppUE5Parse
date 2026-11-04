#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

namespace ue5parse::runtime {

void GetObjectsWithOuter(const UObject* Outer, TArray<UObject*>& Results, bool bIncludeNestedObjects = true);

}
