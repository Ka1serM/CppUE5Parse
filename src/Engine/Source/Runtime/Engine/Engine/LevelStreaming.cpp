#include "Engine/Source/Runtime/Engine/Engine/LevelStreaming.h"

namespace ue5parse::runtime {

FName ULevelStreaming::GetWorldAssetPackageFName() const {
    return WorldAsset.ToSoftObjectPath().AssetPath.PackageName;
}

}
