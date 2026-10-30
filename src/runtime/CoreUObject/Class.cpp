#include <algorithm>
#include <cstring>

#include "CoreUObjectPrivate.h"

#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

IMPLEMENT_CLASS(UField, "Field")
IMPLEMENT_CLASS(UStruct, "Struct")
IMPLEMENT_CLASS(UScriptStruct, "ScriptStruct")
IMPLEMENT_CLASS(UFunction, "Function")
IMPLEMENT_CLASS(UDelegateFunction, "DelegateFunction")
IMPLEMENT_CLASS(USparseDelegateFunction, "SparseDelegateFunction")
IMPLEMENT_CLASS(UClass, "Class")
IMPLEMENT_CLASS(UEnum, "Enum")
IMPLEMENT_CLASS(UPackage, "Package")
IMPLEMENT_CLASS(UInterface, "Interface")

UField::UField(const FObjectInitializer& ObjectInitializer) : UObject(ObjectInitializer) {}
UStruct::UStruct(const FObjectInitializer& ObjectInitializer) : UField(ObjectInitializer) {}
UScriptStruct::UScriptStruct(const FObjectInitializer& ObjectInitializer) : UStruct(ObjectInitializer) {}
UFunction::UFunction(const FObjectInitializer& ObjectInitializer) : UStruct(ObjectInitializer) {}
UDelegateFunction::UDelegateFunction(const FObjectInitializer& ObjectInitializer) : UFunction(ObjectInitializer) {}
USparseDelegateFunction::USparseDelegateFunction(const FObjectInitializer& ObjectInitializer) : UDelegateFunction(ObjectInitializer) {}
UClass::UClass(const FObjectInitializer& ObjectInitializer) : UStruct(ObjectInitializer) {}
UEnum::UEnum(const FObjectInitializer& ObjectInitializer) : UField(ObjectInitializer) {}
UPackage::UPackage(const FObjectInitializer& ObjectInitializer) : UObject(ObjectInitializer) {}
UInterface::UInterface(const FObjectInitializer& ObjectInitializer) : UObject(ObjectInitializer) {}

UStruct::~UStruct() {
    for (FField* Field = ChildProperties; Field != nullptr;) {
        FField* Next = Field->Next;
        delete Field;
        Field = Next;
    }
}

bool UStruct::IsChildOf(const UStruct* SomeBase) const {
    if (SomeBase == nullptr) return false;
    for (const UStruct* Struct = this; Struct != nullptr; Struct = Struct->GetSuperStruct())
        if (Struct == SomeBase) return true;
    return false;
}

FProperty* UStruct::FindPropertyByName(FName InName) const {
    for (const UStruct* Struct = this; Struct != nullptr; Struct = Struct->GetSuperStruct()) {
        for (FField* Field = Struct->ChildProperties; Field != nullptr; Field = Field->Next)
            if (Field->GetFName() == InName) return CastField<FProperty>(Field);
        // A function's frame does not include its super function's properties.
        if (IsA<UFunction>()) break;
    }
    return nullptr;
}

void UStruct::AddCppProperty(FProperty* Property) {
    FField** Tail = &ChildProperties;
    while (*Tail != nullptr) Tail = &(*Tail)->Next;
    *Tail = Property;
}

void UStruct::Link() {
    // A function's layout does not inherit from its super function (UFunction::GetInheritanceSuper).
    UStruct* InheritanceSuper = IsA<UFunction>() ? nullptr : GetSuperStruct();
    PropertiesSize = 0;
    MinAlignment = 1;
    if (InheritanceSuper != nullptr) {
        PropertiesSize = InheritanceSuper->GetPropertiesSize();
        MinAlignment = InheritanceSuper->GetMinAlignment();
    }
    for (FField* Field = ChildProperties; Field != nullptr; Field = Field->Next) {
        FProperty* Property = CastField<FProperty>(Field);
        if (Property == nullptr) continue;
        Property->Link();
        PropertiesSize = Property->SetupOffset(PropertiesSize);
        MinAlignment = std::max(MinAlignment, Property->GetMinAlignment());
    }
    BuildPropertyChains();
}

void UStruct::BuildPropertyChains() {
    UStruct* InheritanceSuper = IsA<UFunction>() ? nullptr : GetSuperStruct();
    // The chains, own properties first, then the super's (UE's TFieldIterator with IncludeSuper).
    FProperty** PropertyLinkPtr = &PropertyLink;
    FProperty** DestructorLinkPtr = &DestructorLink;
    FProperty** RefLinkPtr = &RefLink;
    FProperty** PostConstructLinkPtr = &PostConstructLink;
    for (const UStruct* Struct = this; Struct != nullptr; Struct = Struct == this ? InheritanceSuper : Struct->GetSuperStruct()) {
        for (FField* Field = Struct->ChildProperties; Field != nullptr; Field = Field->Next) {
            FProperty* Property = CastField<FProperty>(Field);
            if (Property == nullptr) continue;
            if (Property->IsA<FObjectPropertyBase>() || Property->IsA<FStructProperty>() || Property->IsA<FArrayProperty>() ||
                Property->IsA<FSetProperty>() || Property->IsA<FMapProperty>() || Property->IsA<FInterfaceProperty>() ||
                Property->IsA<FDelegateProperty>() || Property->IsA<FMulticastDelegateProperty>()) {
                *RefLinkPtr = Property;
                RefLinkPtr = &Property->NextRef;
            }
            // Members of a C++ object are constructed and destroyed by its constructor and destructor.
            bool bOwnedByCpp = Property->HasAnyFlags(RF_MarkAsNative);
            if (!Property->HasAnyPropertyFlags(CPF_IsPlainOldData | CPF_NoDestructor) && !bOwnedByCpp) {
                *DestructorLinkPtr = Property;
                DestructorLinkPtr = &Property->DestructorLinkNext;
            }
            if (Property->GetOwnerClass() != nullptr && !bOwnedByCpp) {
                *PostConstructLinkPtr = Property;
                PostConstructLinkPtr = &Property->PostConstructLinkNext;
            }
            *PropertyLinkPtr = Property;
            PropertyLinkPtr = &Property->PropertyLinkNext;
        }
    }
    *PropertyLinkPtr = nullptr;
    *DestructorLinkPtr = nullptr;
    *RefLinkPtr = nullptr;
    *PostConstructLinkPtr = nullptr;
    BuildUnversionedSchema();
    bLinked = true;
}

void UStruct::InitializeStruct(void* InDest, int32 ArrayDim) const {
    uint8* Dest = static_cast<uint8*>(InDest);
    int32 Stride = GetStructureSize();
    std::memset(Dest, 0, static_cast<std::size_t>(ArrayDim) * Stride);
    for (int32 ArrayIndex = 0; ArrayIndex < ArrayDim; ArrayIndex++) {
        for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
            if (!Property->HasAnyPropertyFlags(CPF_ZeroConstructor)) Property->InitializeValue_InContainer(Dest + ArrayIndex * Stride);
    }
}

void UStruct::DestroyStruct(void* InDest, int32 ArrayDim) const {
    uint8* Dest = static_cast<uint8*>(InDest);
    int32 Stride = GetStructureSize();
    for (int32 ArrayIndex = 0; ArrayIndex < ArrayDim; ArrayIndex++) {
        for (FProperty* Property = DestructorLink; Property != nullptr; Property = Property->DestructorLinkNext)
            Property->DestroyValue_InContainer(Dest + ArrayIndex * Stride);
    }
}

namespace {

// A nesting of inner properties deeper than this is corrupt data.
constexpr int32 MaxFieldDepth = 16;

// The FField flags, as cooked, with the runtime's marker for C++ members cleared.
EObjectFlags CookedFieldFlags(uint32 Flags) { return EObjectFlags(Flags & ~uint32(RF_MarkAsNative)); }

FProperty* SerializeFieldOfType(FArchive& Ar, FName FieldType, FFieldVariant Owner, int32 Depth, int32& OutArrayDim);

// SerializeSingleField: the class name, then the field; None for no field.
FProperty* SerializeSingleField(FArchive& Ar, FFieldVariant Owner, int32 Depth, bool& bOk) {
    FName FieldType;
    Ar << FieldType;
    if (Ar.IsError() || FieldType.isNone()) {
        bOk = false;
        return nullptr;
    }
    int32 ArrayDim = 1;
    FProperty* Property = SerializeFieldOfType(Ar, FieldType, Owner, Depth, ArrayDim);
    bOk = Property != nullptr;
    return Property;
}

UObject* SerializeObject(FArchive& Ar) {
    UObject* Object = nullptr;
    Ar << Object;
    return Object;
}

// FField::Serialize and the property classes' Serialize (UE's FField serialization of a cooked UStruct's
// ChildProperties). A property class the runtime does not have is read and dropped: null, with its ArrayDim.
FProperty* SerializeFieldOfType(FArchive& Ar, FName FieldType, FFieldVariant Owner, int32 Depth, int32& OutArrayDim) {
    if (Depth > MaxFieldDepth) {
        Ar.SetError(ErrorCode::InvalidData, "FField.Depth");
        return nullptr;
    }
    std::string_view TypeName = FieldType.getText();

    // FField: the name, and the flags (cooked 5.8 data leaves them out). FProperty.
    FName Name;
    uint32 Flags = 0;
    Ar << Name;
    if (Ar.EngineVer() < EngineVersion::UE5_8) Ar << Flags;
    int32 ArrayDim = 1, ElementSize = 0;
    uint64 PropertyFlags = 0;
    uint16 RepIndex = 0;
    FName RepNotifyFunc;
    uint8 BlueprintReplicationCondition = 0;
    Ar << ArrayDim << ElementSize << PropertyFlags << RepIndex << RepNotifyFunc << BlueprintReplicationCondition;
    OutArrayDim = std::max<int32>(ArrayDim, 1);
    if (Ar.IsError()) return nullptr;

    FFieldClass* FieldClass = FindPropertyClass(TypeName);
    FProperty* Property = FieldClass ? ConstructProperty(FieldClass, Owner, Name, CookedFieldFlags(Flags)) : nullptr;
    if (Property != nullptr) {
        Property->ArrayDim = OutArrayDim;
        Property->PropertyFlags = EPropertyFlags(PropertyFlags);
        Property->RepIndex = RepIndex;
        Property->RepNotifyFunc = RepNotifyFunc;
        Property->BlueprintReplicationCondition = BlueprintReplicationCondition;
    }

    bool bOk = true;
    if (TypeName == "BoolProperty") {
        uint8 FieldSize = 0, ByteOffset = 0, ByteMask = 0, FieldMask = 0, BoolSize = 0, NativeBool = 0;
        Ar << FieldSize << ByteOffset << ByteMask << FieldMask << BoolSize << NativeBool;
        if (auto* Bool = CastField<FBoolProperty>(Property)) Bool->SetBoolSize(BoolSize ? BoolSize : 1, NativeBool != 0);
    } else if (TypeName == "ByteProperty") {
        UObject* EnumObject = SerializeObject(Ar);
        if (auto* Byte = CastField<FByteProperty>(Property)) Byte->Enum = Cast<UEnum>(EnumObject);
    } else if (TypeName == "EnumProperty") {
        UObject* EnumObject = SerializeObject(Ar);
        bool bInnerOk = false;
        FProperty* Underlying = SerializeSingleField(Ar, Property ? FFieldVariant(Property) : Owner, Depth + 1, bInnerOk);
        if (auto* Enum = CastField<FEnumProperty>(Property)) {
            Enum->Enum = Cast<UEnum>(EnumObject);
            Enum->UnderlyingProp = CastField<FNumericProperty>(Underlying);
            bOk = Enum->UnderlyingProp != nullptr;
            if (!bOk) delete Underlying;
        } else {
            delete Underlying;
        }
    } else if (TypeName == "StructProperty") {
        UObject* StructObject = SerializeObject(Ar);
        if (auto* Struct = CastField<FStructProperty>(Property)) {
            Struct->Struct = Cast<UScriptStruct>(StructObject);
            bOk = Struct->Struct != nullptr && Struct->Struct->IsLinked();
            if (!bOk)
                ReportLoadProblem("load.property_struct", FString(Name.toString()) + FString(": ") +
                                                              (StructObject ? StructObject->GetFullName() : FString("null")) +
                                                              (Struct->Struct && !Struct->Struct->IsLinked() ? FString(" (not linked)") : FString()));
        }
    } else if (TypeName == "ObjectProperty" || TypeName == "ObjectPtrProperty" || TypeName == "WeakObjectProperty" ||
               TypeName == "LazyObjectProperty" || TypeName == "SoftObjectProperty") {
        UObject* PropertyClass = SerializeObject(Ar);
        if (auto* Object = CastField<FObjectPropertyBase>(Property)) Object->PropertyClass = Cast<UClass>(PropertyClass);
    } else if (TypeName == "ClassProperty" || TypeName == "ClassPtrProperty" || TypeName == "SoftClassProperty") {
        UObject* PropertyClass = SerializeObject(Ar);
        UObject* MetaClass = SerializeObject(Ar);
        if (auto* Class = CastField<FClassProperty>(Property)) {
            Class->PropertyClass = Cast<UClass>(PropertyClass);
            Class->MetaClass = Cast<UClass>(MetaClass);
        } else if (auto* SoftClass = CastField<FSoftClassProperty>(Property)) {
            SoftClass->PropertyClass = Cast<UClass>(PropertyClass);
            SoftClass->MetaClass = Cast<UClass>(MetaClass);
        }
    } else if (TypeName == "VerseClassProperty") {
        SerializeObject(Ar);
        SerializeObject(Ar);
        Ar.SkipBytes(2);   // bRequiresConcrete, bRequiresCastable
    } else if (TypeName == "InterfaceProperty") {
        UObject* InterfaceClass = SerializeObject(Ar);
        if (auto* Interface = CastField<FInterfaceProperty>(Property)) Interface->InterfaceClass = Cast<UClass>(InterfaceClass);
    } else if (TypeName == "DelegateProperty" || TypeName == "MulticastDelegateProperty" || TypeName == "MulticastInlineDelegateProperty" ||
               TypeName == "MulticastSparseDelegateProperty" || TypeName == "VerseFunctionProperty") {
        UObject* Signature = SerializeObject(Ar);
        if (auto* Delegate = CastField<FDelegateProperty>(Property)) {
            Delegate->SignatureFunction = Cast<UFunction>(Signature);
        } else if (auto* Multicast = CastField<FMulticastDelegateProperty>(Property)) {
            Multicast->SignatureFunction = Cast<UFunction>(Signature);
        }
    } else if (TypeName == "FieldPathProperty") {
        FName PropertyClassName;
        Ar << PropertyClassName;
        if (auto* FieldPath = CastField<FFieldPathProperty>(Property)) FieldPath->PropertyClass = FindPropertyClass(PropertyClassName.getText());
    } else if (TypeName == "ArrayProperty" || TypeName == "SetProperty" || TypeName == "OptionalProperty" || TypeName == "VerseStringProperty") {
        bool bInnerOk = false;
        FProperty* Inner = SerializeSingleField(Ar, Property ? FFieldVariant(Property) : Owner, Depth + 1, bInnerOk);
        if (auto* Array = CastField<FArrayProperty>(Property)) {
            Array->Inner = Inner;
            bOk = Inner != nullptr;
        } else if (auto* Set = CastField<FSetProperty>(Property)) {
            Set->ElementProp = Inner;
            bOk = Inner != nullptr;
        } else {
            delete Inner;
        }
    } else if (TypeName == "MapProperty") {
        bool bKeyOk = false, bValueOk = false;
        FProperty* Key = SerializeSingleField(Ar, Property ? FFieldVariant(Property) : Owner, Depth + 1, bKeyOk);
        FProperty* Value = SerializeSingleField(Ar, Property ? FFieldVariant(Property) : Owner, Depth + 1, bValueOk);
        if (auto* Map = CastField<FMapProperty>(Property)) {
            Map->KeyProp = Key;
            Map->ValueProp = Value;
            bOk = Key != nullptr && Value != nullptr;
        } else {
            delete Key;
            delete Value;
        }
    } else if (FieldClass == nullptr && TypeName != "VerseDynamicProperty" && TypeName != "ReferenceProperty" &&
               TypeName != "Utf8StrProperty" && TypeName != "AnsiStrProperty") {
        Ar.SetError(ErrorCode::UnknownPropertyType, "FField.Type");
    }
    if (Property == nullptr) {
        ReportLoadProblem("load.property_class", FString(FieldType.toString()));
        return nullptr;
    }
    if (!bOk || Ar.IsError()) {
        if (!Ar.IsError()) ReportLoadProblem("load.property_type", FString(Name.toString()) + FString(" (") + FString(FieldType.toString()) + FString(")"));
        delete Property;
        return nullptr;
    }
    return Property;
}

}

void UStruct::Serialize(FArchive& Ar) {
    UField::Serialize(Ar);
    if (Ar.IsError()) return;
    UObject* SuperObject = nullptr;
    Ar << SuperObject;
    SuperStruct = Cast<UStruct>(SuperObject);

    // Children: the functions of a class.
    int32 ChildCount = Ar.ReadCount(4);
    UField** ChildTail = &Children;
    for (int32 i = 0; i < ChildCount && !Ar.IsError(); i++) {
        UObject* ChildObject = nullptr;
        Ar << ChildObject;
        UField* Child = Cast<UField>(ChildObject);
        if (Child == nullptr || Child == this) continue;
        *ChildTail = Child;
        ChildTail = &Child->Next;
    }
    *ChildTail = nullptr;

    // ChildProperties, and the unversioned schema of this struct's own properties (one slot per static array element,
    // also for properties the runtime cannot build, so that the indices after them stay right).
    int32 PropertyCount = Ar.ReadCount(12);
    FField** PropertyTail = &ChildProperties;
    OwnUnversionedSlots.Empty(PropertyCount);
    for (int32 i = 0; i < PropertyCount && !Ar.IsError(); i++) {
        FName FieldType;
        Ar << FieldType;
        int32 ArrayDim = 1;
        FProperty* Property = SerializeFieldOfType(Ar, FieldType, this, 0, ArrayDim);
        for (int32 Element = 0; Element < ArrayDim; Element++) OwnUnversionedSlots.Add(FUnversionedSlot{Property, Element});
        if (Property == nullptr) continue;
        *PropertyTail = Property;
        PropertyTail = &Property->Next;
    }
    *PropertyTail = nullptr;

    // The bytecode: its size in memory format, then the serialized script.
    int32 SerializedScriptSize = 0;
    Ar << BytecodeBufferSize << SerializedScriptSize;
    if (SerializedScriptSize < 0 || SerializedScriptSize > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::InvalidData, "UStruct.ScriptSize");
        return;
    }
    SerializeRaw(Ar, SerializedScript, SerializedScriptSize);
}

void UScriptStruct::Link() {
    UStruct::Link();
    PrepareCppStructOps();
}

namespace {

// A native struct's mapped properties that its C++ type does not have: laid out after the C++ part, handled besides
// the C++ operations.
bool IsExtraProperty(const FProperty* Property) { return !Property->HasAnyFlags(RF_MarkAsNative); }

}

void UScriptStruct::PrepareCppStructOps() {
    bool bPlainOldData = true;
    bool bZeroConstructor = true;
    bool bNoDestructor = true;
    bool bAnyExtra = false;
    for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext) {
        if (CppStructOps != nullptr && !IsExtraProperty(Property)) continue;
        bAnyExtra = true;
        bPlainOldData &= Property->HasAnyPropertyFlags(CPF_IsPlainOldData);
        bZeroConstructor &= Property->HasAnyPropertyFlags(CPF_ZeroConstructor);
        bNoDestructor &= Property->HasAnyPropertyFlags(CPF_NoDestructor);
    }
    if (CppStructOps != nullptr) {
        PropertiesSize = std::max(PropertiesSize, CppStructOps->Size);
        MinAlignment = std::max(MinAlignment, CppStructOps->Alignment);
        StructFlags = EStructFlags(StructFlags | STRUCT_Native | STRUCT_CopyNative | STRUCT_IdenticalNative);
        bPlainOldData &= CppStructOps->bIsPlainOldData;
        bZeroConstructor &= CppStructOps->bHasZeroConstructor;
        bNoDestructor &= CppStructOps->Destruct == nullptr;
        (void)bAnyExtra;
    }
    uint32 Flags = StructFlags & ~uint32(STRUCT_IsPlainOldData | STRUCT_ZeroConstructor | STRUCT_NoDestructor);
    if (bPlainOldData) Flags |= STRUCT_IsPlainOldData;
    if (bZeroConstructor) Flags |= STRUCT_ZeroConstructor;
    if (bNoDestructor) Flags |= STRUCT_NoDestructor;
    StructFlags = EStructFlags(Flags);
}

void UScriptStruct::InitializeStruct(void* Dest, int32 ArrayDim) const {
    if (CppStructOps == nullptr) {
        UStruct::InitializeStruct(Dest, ArrayDim);
        return;
    }
    int32 Stride = GetStructureSize();
    std::memset(Dest, 0, static_cast<std::size_t>(ArrayDim) * Stride);
    for (int32 i = 0; i < ArrayDim; i++) {
        uint8* Element = static_cast<uint8*>(Dest) + i * Stride;
        if (CppStructOps->Construct != nullptr && !CppStructOps->bHasZeroConstructor) CppStructOps->Construct(Element);
        for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
            if (IsExtraProperty(Property) && !Property->HasAnyPropertyFlags(CPF_ZeroConstructor)) Property->InitializeValue_InContainer(Element);
    }
}

void UScriptStruct::DestroyStruct(void* Dest, int32 ArrayDim) const {
    if (CppStructOps == nullptr) {
        UStruct::DestroyStruct(Dest, ArrayDim);
        return;
    }
    int32 Stride = GetStructureSize();
    for (int32 i = 0; i < ArrayDim; i++) {
        uint8* Element = static_cast<uint8*>(Dest) + i * Stride;
        for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
            if (IsExtraProperty(Property)) Property->DestroyValue_InContainer(Element);
        if (CppStructOps->Destruct != nullptr) CppStructOps->Destruct(Element);
    }
}

void UScriptStruct::CopyScriptStruct(void* InDest, const void* InSrc, int32 ArrayDim) const {
    uint8* Dest = static_cast<uint8*>(InDest);
    const uint8* Src = static_cast<const uint8*>(InSrc);
    int32 Stride = GetStructureSize();
    if (StructFlags & STRUCT_IsPlainOldData) {
        std::memmove(Dest, Src, static_cast<std::size_t>(ArrayDim) * Stride);
        return;
    }
    for (int32 i = 0; i < ArrayDim; i++) {
        if (CppStructOps != nullptr && CppStructOps->Copy != nullptr) CppStructOps->Copy(Dest + i * Stride, Src + i * Stride);
        for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
            if (CppStructOps == nullptr || IsExtraProperty(Property)) Property->CopyCompleteValue_InContainer(Dest + i * Stride, Src + i * Stride);
    }
}

bool UScriptStruct::CompareScriptStruct(const void* A, const void* B, uint32 PortFlags) const {
    if (B == nullptr) return false;
    if (CppStructOps != nullptr && CppStructOps->Identical != nullptr && !CppStructOps->Identical(A, B)) return false;
    for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext) {
        if (CppStructOps != nullptr && CppStructOps->Identical != nullptr && !IsExtraProperty(Property)) continue;
        for (int32 i = 0; i < Property->ArrayDim; i++) {
            if (!Property->Identical(Property->ContainerPtrToValuePtr<void>(A, i), Property->ContainerPtrToValuePtr<void>(B, i), PortFlags))
                return false;
        }
    }
    return true;
}

void UScriptStruct::ClearScriptStruct(void* Dest, int32 ArrayDim) const {
    DestroyStruct(Dest, ArrayDim);
    InitializeStruct(Dest, ArrayDim);
}

uint32 UScriptStruct::GetStructTypeHash(const void* Src) const {
    if (CppStructOps != nullptr && CppStructOps->GetTypeHash != nullptr) return CppStructOps->GetTypeHash(Src);
    uint32 Hash = 0;
    for (FProperty* Property = PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
        Hash = Hash * 31 + Property->GetValueTypeHash(Property->ContainerPtrToValuePtr<void>(Src));
    return Hash;
}

void UScriptStruct::Serialize(FArchive& Ar) {
    UStruct::Serialize(Ar);
    if (!Ar.IsError() && Ar.GetRemaining() >= 4) {
        uint32 Flags = 0;
        Ar << Flags;
        StructFlags = EStructFlags(Flags & ~uint32(STRUCT_Native));
    }
    Link();
}

FProperty* UFunction::GetReturnProperty() const {
    for (FField* Field = ChildProperties; Field != nullptr; Field = Field->Next) {
        FProperty* Property = CastField<FProperty>(Field);
        if (Property == nullptr || !(Property->PropertyFlags & CPF_Parm)) break;
        if (Property->PropertyFlags & CPF_ReturnParm) return Property;
    }
    return nullptr;
}

UClass* UFunction::GetOuterUClass() const { return Cast<UClass>(GetOuter()); }

void UFunction::Invoke(UObject* Obj, FFrame& Stack, RESULT_DECL) {
    if (Func != nullptr) Func(Obj, Stack, RESULT_PARAM);
}

void UFunction::Link() {
    UStruct::Link();
    NumParms = 0;
    ParmsSize = 0;
    ReturnValueOffset = 0xffff;
    FirstPropertyToInit = nullptr;
    FProperty** ConstructLink = &FirstPropertyToInit;
    for (FField* Field = ChildProperties; Field != nullptr; Field = Field->Next) {
        FProperty* Property = CastField<FProperty>(Field);
        if (Property == nullptr) continue;
        if (Property->PropertyFlags & CPF_Parm) {
            NumParms++;
            ParmsSize = static_cast<uint16>(Property->Offset_Internal + Property->GetSize());
            if (Property->PropertyFlags & CPF_ReturnParm) ReturnValueOffset = static_cast<uint16>(Property->Offset_Internal);
        } else if ((FunctionFlags & FUNC_HasDefaults) == 0) {
            break;
        } else if (!Property->HasAnyPropertyFlags(CPF_ZeroConstructor)) {
            *ConstructLink = Property;
            Property->PostConstructLinkNext = nullptr;
            ConstructLink = &Property->PostConstructLinkNext;
        }
    }
}

void UFunction::Serialize(FArchive& Ar) {
    UStruct::Serialize(Ar);
    uint32 Flags = 0;
    Ar << Flags;
    if ((Flags & FUNC_Net) != 0) {
        uint16 RepOffset = 0;
        Ar << RepOffset;
    }
    UObject* EventGraph = nullptr;
    Ar << EventGraph << EventGraphCallOffset;
    FunctionFlags = EFunctionFlags(Flags & ~uint32(FUNC_Native));
    EventGraphFunction = Cast<UFunction>(EventGraph);
    Func = &UObject::ProcessInternal;
    Link();
}

UObject* UClass::GetDefaultObject(bool bCreateIfNeeded) {
    if (ClassDefaultObject != nullptr || !bCreateIfNeeded) return ClassDefaultObject;
    std::lock_guard Lock(GetLoaderMutex());
    if (ClassDefaultObject == nullptr) ClassDefaultObject = CreateDefaultObject();
    return ClassDefaultObject;
}

UObject* UClass::CreateDefaultObject() {
    // A cooked class's default object is an export of its package.
    if (FLinkerLoad* ClassLinker = GetLinker())
        if (ClassDefaultObjectIndex != 0) return ClassLinker->IndexToObject(FPackageIndex(ClassDefaultObjectIndex));
    // A native class's default object runs the C++ constructors, without copying the super's defaults (UE's
    // CreateDefaultObject passes no InitializeProperties option).
    UClass* ParentClass = GetSuperClass();
    UObject* ParentDefaultObject = ParentClass ? ParentClass->GetDefaultObject() : nullptr;
    FString DefaultName = FString("Default__") + GetName();
    UObject* Object = StaticAllocateObject(this, GetOuter(), MakeName(DefaultName),
                                           EObjectFlags(RF_Public | RF_ClassDefaultObject | RF_ArchetypeObject));
    ClassDefaultObject = Object;
    ClassConstructor(FObjectInitializer(Object, ParentDefaultObject, false, false));
    return Object;
}

UFunction* UClass::FindFunctionByName(FName InName, bool bIncludeSuper) const {
    for (const auto& [Name, Function] : FuncMap)
        if (Name == InName) return Function;
    if (!bIncludeSuper) return nullptr;
    if (UClass* Super = GetSuperClass())
        if (UFunction* Function = Super->FindFunctionByName(InName, true)) return Function;
    for (const FImplementedInterface& Interface : Interfaces)
        if (Interface.Class != nullptr)
            if (UFunction* Function = Interface.Class->FindFunctionByName(InName, true)) return Function;
    return nullptr;
}

void UClass::AddFunctionToFunctionMap(UFunction* Function, FName FuncName) {
    for (auto& [Name, Existing] : FuncMap)
        if (Name == FuncName) {
            Existing = Function;
            return;
        }
    FuncMap.Add({FuncName, Function});
}

bool UClass::ImplementsInterface(const UClass* SomeInterface) const {
    if (SomeInterface == nullptr || !SomeInterface->HasAnyClassFlags(CLASS_Interface)) return false;
    for (const UClass* Class = this; Class != nullptr; Class = Class->GetSuperClass()) {
        for (const FImplementedInterface& Interface : Class->Interfaces)
            if (Interface.Class != nullptr && Interface.Class->IsChildOf(SomeInterface)) return true;
    }
    return false;
}

void* UClass::GetInterfaceAddress(UObject* Object, const UClass*) const { return Object; }

void UClass::Link() {
    UStruct::Link();
    for (UField* Child = Children; Child != nullptr; Child = Child->Next)
        if (UFunction* Function = Cast<UFunction>(Child)) AddFunctionToFunctionMap(Function, Function->GetFName());
}

UClass::~UClass() {
    if (SparseClassData != nullptr) {
        SparseClassDataStruct->DestroyStruct(SparseClassData);
        RuntimeFree(SparseClassData);
    }
}

void UClass::Serialize(FArchive& Ar) {
    UStruct::Serialize(Ar);
    // FuncMap: the functions by name (the same objects as the children).
    int32 FuncCount = Ar.ReadCount(12);
    for (int32 i = 0; i < FuncCount && !Ar.IsError(); i++) {
        FName FunctionName;
        UObject* FunctionObject = nullptr;
        Ar << FunctionName << FunctionObject;
        if (UFunction* Function = Cast<UFunction>(FunctionObject)) AddFunctionToFunctionMap(Function, FunctionName);
    }
    uint32 Flags = 0;
    UObject* Within = nullptr;
    UObject* GeneratedBy = nullptr;
    Ar << Flags << Within << ClassConfigName << GeneratedBy;
    ClassFlags = EClassFlags(Flags & ~uint32(CLASS_Native | CLASS_Intrinsic));
    ClassWithin = Cast<UClass>(Within);
    int32 InterfaceCount = Ar.ReadCount(12);
    for (int32 i = 0; i < InterfaceCount && !Ar.IsError(); i++) {
        UObject* InterfaceClass = nullptr;
        FImplementedInterface Implemented;
        Ar << InterfaceClass << Implemented.PointerOffset << Implemented.bImplementedByK2;
        Implemented.Class = Cast<UClass>(InterfaceClass);
        if (Implemented.Class != nullptr) Interfaces.Add(Implemented);
    }
    bool bDeprecatedForceScriptOrder = false;
    FName Dummy;
    bool bCooked = false;
    FPackageIndex DefaultObjectIndex;
    Ar << bDeprecatedForceScriptOrder << Dummy << bCooked << DefaultObjectIndex;
    // The default object loads when it is first asked for, after this class is linked.
    ClassDefaultObjectIndex = DefaultObjectIndex.Index;
    UClass* Super = GetSuperClass();
    if (Super != nullptr) {
        ClassConstructor = Super->ClassConstructor;
        NativeSize = Super->NativeSize;
        ClassCastFlags = Super->ClassCastFlags;
        ClassFlags = EClassFlags(ClassFlags | (Super->ClassFlags & (CLASS_HasInstancedReference | CLASS_DefaultToInstanced)));
    }
    Link();
}

FName UEnum::GetNameByIndex(int32 Index) const { return Names.IsValidIndex(Index) ? Names[Index].first : FName(); }

int64 UEnum::GetValueByIndex(int32 Index) const { return Names.IsValidIndex(Index) ? Names[Index].second : INDEX_NONE; }

int32 UEnum::GetIndexByValue(int64 Value) const {
    for (int32 i = 0; i < Names.Num(); i++)
        if (Names[i].second == Value) return i;
    return INDEX_NONE;
}

int32 UEnum::GetIndexByName(FName InName) const {
    for (int32 i = 0; i < Names.Num(); i++)
        if (Names[i].first == InName) return i;
    // A short name of a namespaced enum ("Value" for "EMyEnum::Value").
    std::string_view Wanted = InName.getText();
    for (int32 i = 0; i < Names.Num(); i++) {
        std::string_view Full = Names[i].first.getText();
        std::size_t Colon = Full.rfind("::");
        if (Colon == std::string_view::npos || Names[i].first.getNumber() != InName.getNumber()) continue;
        std::string_view Short = Full.substr(Colon + 2);
        if (Short.size() == Wanted.size() && FString(Short).Equals(FString(Wanted), ESearchCase::IgnoreCase)) return i;
    }
    return INDEX_NONE;
}

int64 UEnum::GetValueByName(FName InName) const {
    int32 Index = GetIndexByName(InName);
    return Index == INDEX_NONE ? INDEX_NONE : Names[Index].second;
}

FName UEnum::GetNameByValue(int64 Value) const { return GetNameByIndex(GetIndexByValue(Value)); }

FString UEnum::GetNameStringByIndex(int32 Index) const {
    if (!Names.IsValidIndex(Index)) return FString();
    std::string Full = Names[Index].first.toString();
    std::size_t Colon = Full.rfind("::");
    return FString(Colon == std::string::npos ? std::string_view(Full) : std::string_view(Full).substr(Colon + 2));
}

FString UEnum::GetNameStringByValue(int64 Value) const { return GetNameStringByIndex(GetIndexByValue(Value)); }

FText UEnum::GetDisplayNameTextByIndex(int32 Index) const { return FText::FromString(GetNameStringByIndex(Index)); }

int64 UEnum::GetMaxEnumValue() const {
    int64 Max = 0;
    bool bAny = false;
    for (const auto& [Name, Value] : Names) {
        // The generated _MAX entry is not a value.
        std::string_view Text = Name.getText();
        if (Text.size() >= 4 && Text.substr(Text.size() - 4) == "_MAX") continue;
        if (!bAny || Value > Max) Max = Value;
        bAny = true;
    }
    return Max;
}

void UEnum::Serialize(FArchive& Ar) {
    UField::Serialize(Ar);
    int32 Count = Ar.ReadCount(16);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FName EntryName;
        int64 Value = 0;
        Ar << EntryName << Value;
        Names.Add({EntryName, Value});
    }
    uint8 Form = 0;
    Ar << Form;
    CppForm = static_cast<ECppForm>(Form);
    // EnumUnderlyingType, since 5.8.
    if (Ar.EngineVer() >= EngineVersion::UE5_8 && Ar.GetRemaining() > 0) Ar.SkipBytes(1);
}

void UClass::SerializeDefaultObject(UObject* Object, FArchive& Ar) {
    // Laid out as this class; a default object has no vtable-dependent native data in cooked form.
    SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(Object));
    if (Ar.IsError() || Ar.GetRemaining() < 4 || !HasAnyClassFlags(CLASS_CompiledFromBlueprint)) return;
    // The sparse class data: its struct, then its values.
    UObject* StructObject = nullptr;
    Ar << StructObject;
    UScriptStruct* Struct = Cast<UScriptStruct>(StructObject);
    if (Struct == nullptr) return;
    if (SparseClassData == nullptr) {
        SparseClassDataStruct = Struct;
        SparseClassData = RuntimeMalloc(static_cast<std::size_t>(std::max(Struct->GetStructureSize(), 1)),
                                        static_cast<std::size_t>(std::max(Struct->GetMinAlignment(), 16)));
        Struct->InitializeStruct(SparseClassData);
    }
    Struct->SerializeTaggedProperties(Ar, static_cast<uint8*>(SparseClassData));
}

}
