#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"

namespace ue5parse::runtime {

// The objects whose outer is Outer (UE's UObjectHash.h); with bIncludeNestedObjects also their subobjects, depth first.
void GetObjectsWithOuter(const UObject* Outer, TArray<UObject*>& Results, bool bIncludeNestedObjects = true);

}
