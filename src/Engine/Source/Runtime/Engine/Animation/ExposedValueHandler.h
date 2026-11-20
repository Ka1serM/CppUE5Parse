#pragma once

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/FieldPath.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "ExposedValueHandler.generated.h"

namespace ue5parse::runtime {

struct FPropertyAccessLibrary;
struct FAnimationBaseContext;
struct FAnimNodeExposedValueHandler;

namespace EPostCopyOperation {

UENUM()
enum Type : uint8 { None, LogicalNegateBool };

}

UENUM()
enum class ECopyType : uint8 { MemCopy, BoolProperty, StructProperty, ObjectProperty };

USTRUCT()
struct FExposedValueCopyRecord {
    GENERATED_BODY()
    UPROPERTY()
    int32 CopyIndex = INDEX_NONE;
    UPROPERTY()
    TEnumAsByte<EPostCopyOperation::Type> PostCopyOperation = EPostCopyOperation::None;
    UPROPERTY()
    bool bOnlyUpdateWhenActive = false;
    UPROPERTY()
    TObjectPtr<UObject> SourceProperty_DEPRECATED = nullptr;
    UPROPERTY()
    FName SourcePropertyName;
    UPROPERTY()
    FName SourceSubPropertyName;
    UPROPERTY()
    int32 SourceArrayIndex = 0;
    UPROPERTY()
    TFieldPath<FProperty> DestProperty;
    UPROPERTY()
    int32 DestArrayIndex = 0;
    UPROPERTY()
    int32 Size = 0;
    UPROPERTY()
    bool bInstanceIsTarget = false;
    UPROPERTY()
    TFieldPath<FProperty> CachedSourceProperty;
    UPROPERTY()
    TFieldPath<FProperty> CachedSourceStructSubProperty;
    //! UE 4.17 to 4.21 save the copy type of the record.
    UPROPERTY()
    ECopyType CopyType_DEPRECATED = ECopyType::MemCopy;
    //! UE 4.12 to 4.16 save the cached bool properties as object references.
    UPROPERTY()
    TObjectPtr<UObject> CachedBoolSourceProperty_DEPRECATED = nullptr;
    UPROPERTY()
    TObjectPtr<UObject> CachedBoolDestProperty_DEPRECATED = nullptr;
    //! UE 4.14 to 4.16 save the cached struct property as an object reference.
    UPROPERTY()
    TObjectPtr<UObject> CachedStructDestProperty_DEPRECATED = nullptr;
};

USTRUCT()
struct FExposedValueHandler {
    GENERATED_BODY()
    //! UE 5.0 to 5.3 save the handler data here, UE 5.4 moves it to FAnimNodeExposedValueHandler.
    UPROPERTY()
    TArray<FExposedValueCopyRecord> CopyRecords;
    UPROPERTY()
    TObjectPtr<UFunction> Function = nullptr;
    UPROPERTY()
    FName BoundFunction;
    UPROPERTY()
    TFieldPath<FStructProperty> ValueHandlerNodeProperty;
    const FPropertyAccessLibrary* PropertyAccessLibrary = nullptr;
    UScriptStruct* HandlerStruct = nullptr;
    FAnimNodeExposedValueHandler* Handler = nullptr;
    //! UE 4.25 and older copy the records from the anim instance into the node of this property, without a property access library.
    const FStructProperty* NodeProperty = nullptr;

    void Initialize(UClass* InClass, const FPropertyAccessLibrary& InPropertyAccessLibrary);
    void InitializeDirectCopies(const UClass* InClass, const FStructProperty* InNodeProperty);
    void Execute(const FAnimationBaseContext& Context) const;
};

USTRUCT()
struct FAnimNodeExposedValueHandler {
    GENERATED_BODY()
    FAnimNodeExposedValueHandler() = default;
    FAnimNodeExposedValueHandler(const FAnimNodeExposedValueHandler&) = default;
    FAnimNodeExposedValueHandler& operator=(const FAnimNodeExposedValueHandler&) = default;
    virtual ~FAnimNodeExposedValueHandler() = default;

    virtual void Initialize(const UClass* InClass) {}
    virtual void Execute(const FAnimationBaseContext& InContext) const {}
};

USTRUCT()
struct FAnimNodeExposedValueHandler_Base : public FAnimNodeExposedValueHandler {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UFunction> Function = nullptr;
    UPROPERTY()
    FName BoundFunction;

    void Initialize(const UClass* InClass) override;
    void Execute(const FAnimationBaseContext& InContext) const override;
};

USTRUCT()
struct FAnimNodeExposedValueHandler_PropertyAccess : public FAnimNodeExposedValueHandler_Base {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FExposedValueCopyRecord> CopyRecords;
    const FPropertyAccessLibrary* PropertyAccessLibrary = nullptr;

    void Initialize(const UClass* InClass) override;
    void Execute(const FAnimationBaseContext& InContext) const override;
};

}
