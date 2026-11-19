#pragma once

#include "Engine/Source/Runtime/Core/Math/Axis.h"
#include "Engine/Source/Runtime/Engine/Engine/DataTable.h"
#include "Engine/Source/Runtime/Engine/EngineNativeMissing.h"

#include "MirrorDataTable.generated.h"

namespace ue5parse::runtime {

class USkeleton;

namespace EMirrorRowType {

UENUM()
enum Type : int32 { Bone, AnimationNotify, Curve, SyncMarker, Custom };

}

namespace EMirrorFindReplaceMethod {

UENUM()
enum Type : int32 { Prefix, Suffix, RegularExpression };

}

USTRUCT()
struct FMirrorTableRow : public FTableRowBase {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    FName MirroredName;
    UPROPERTY()
    TEnumAsByte<EMirrorRowType::Type> MirrorEntryType = EMirrorRowType::Bone;
    UPROPERTY()
    bool bEnabled = true;
};

USTRUCT()
struct FMirrorFindReplaceExpression {
    GENERATED_BODY()
    UPROPERTY()
    FName FindExpression;
    UPROPERTY()
    FName ReplaceExpression;
    UPROPERTY()
    TEnumAsByte<EMirrorFindReplaceMethod::Type> FindReplaceMethod = EMirrorFindReplaceMethod::Prefix;
};

UCLASS()
class UMirrorDataTable : public UDataTable {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<FMirrorFindReplaceExpression> MirrorFindReplaceExpressions;
    UPROPERTY()
    TEnumAsByte<EAxis::Type> MirrorAxis = EAxis::X;
    UPROPERTY()
    bool bMirrorRootMotion = true;
    UPROPERTY()
    TObjectPtr<USkeleton> Skeleton = nullptr;
};

}
