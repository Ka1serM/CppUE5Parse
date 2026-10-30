#include "ue5parse/runtime/Engine/Engine/LevelStreaming.h"

// UE's LevelStreaming.cpp (UE 5.3).

namespace ue5parse::runtime {

FName ULevelStreaming::GetWorldAssetPackageFName() const { return WorldAsset.ToSoftObjectPath().AssetPath.PackageName; }

}
