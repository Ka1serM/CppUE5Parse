#include <cstdio>
#include <cstring>
#include <mutex>

#include <ankerl/unordered_dense.h>

#include "CoreUObjectPrivate.h"

#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/CoreUObject/UObject/Class.h"
#include "ue5parse/runtime/CoreUObject/UObject/Stack.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/schema/Mappings.h"

// The registry of the compiled-in (native) classes, structs and functions, and the native types the runtime builds from
// them and from the game's mappings (UE's compiled-in registration in UObjectBase.cpp and the generated code).

namespace ue5parse::runtime {

namespace {

struct FNativeRegistry {
    ankerl::unordered_dense::map<FName, const FNativeClassInfo*, FNameHasher> ClassInfos;
    ankerl::unordered_dense::map<FName, const FNativeStructInfo*, FNameHasher> StructInfos;
    ankerl::unordered_dense::map<FName, const FNativeDelegateFunctionInfo*, FNameHasher> DelegateInfos;
    ankerl::unordered_dense::map<FName, TArray<std::span<const FNativeFunction>>, FNameHasher> FunctionTables;

    const FRuntime* Runtime = nullptr;
    const FileProvider* Provider = nullptr;
    // The names of the native classes of the game (the script objects that have a class default object), and their
    // packages.
    ankerl::unordered_dense::map<FName, FName, FNameHasher> ScriptClassPackages;
    ankerl::unordered_dense::map<FName, FName, FNameHasher> ScriptObjectPackages;

    ankerl::unordered_dense::map<FName, UClass*, FNameHasher> Classes;
    ankerl::unordered_dense::map<FName, UScriptStruct*, FNameHasher> Structs;
    ankerl::unordered_dense::map<FName, UEnum*, FNameHasher> Enums;
    ankerl::unordered_dense::map<FName, UFunction*, FNameHasher> DelegateFunctions;
    ankerl::unordered_dense::map<FName, int32, FNameHasher> ReservedSizes;
    // Native types whose outer package is set once the package class can be used.
    TArray<std::pair<UObject*, FName>> PendingOuters;
    bool bBootstrapping = false;
};

FNativeRegistry& GetRegistry() {
    static FNativeRegistry Registry;
    return Registry;
}

const StructSchema* FindSchema(FName Name) {
    FNativeRegistry& Registry = GetRegistry();
    return Registry.Provider ? Registry.Provider->getMappings().findStruct(Name) : nullptr;
}

const char* const IntrinsicClassNames[] = {"Object",   "Field",           "Struct",  "ScriptStruct", "Function", "DelegateFunction",
                                           "SparseDelegateFunction", "Class", "Enum", "Package", "Interface"};

bool IsIntrinsic(FName Name) {
    for (const char* Intrinsic : IntrinsicClassNames)
        if (Name == FName(Intrinsic)) return true;
    return false;
}

FName GetPackageName(FName ObjectName) {
    FNativeRegistry& Registry = GetRegistry();
    if (auto Info = Registry.ClassInfos.find(ObjectName); Info != Registry.ClassInfos.end()) return FName(Info->second->Package);
    if (auto Info = Registry.StructInfos.find(ObjectName); Info != Registry.StructInfos.end()) return FName(Info->second->Package);
    if (auto Found = Registry.ScriptObjectPackages.find(ObjectName); Found != Registry.ScriptObjectPackages.end()) return Found->second;
    return FName("/Script/CoreUObject");
}

// The size reserved for the C++ part of a registered class: the largest registered class of its subtree. Native
// properties without a C++ member go after it, so that no subclass's C++ members overlap them.
int32 GetReservedSize(FName Name) {
    FNativeRegistry& Registry = GetRegistry();
    if (auto Found = Registry.ReservedSizes.find(Name); Found != Registry.ReservedSizes.end()) return Found->second;
    int32 Reserved = 0;
    for (const auto& [ClassName, Info] : Registry.ClassInfos) {
        for (const FNativeClassInfo* Walk = Info; Walk != nullptr;) {
            if (FName(Walk->Name) == Name) {
                Reserved = std::max(Reserved, Info->Size);
                break;
            }
            if (Walk->SuperName == nullptr) break;
            auto Super = Registry.ClassInfos.find(FName(Walk->SuperName));
            Walk = Super != Registry.ClassInfos.end() ? Super->second : nullptr;
        }
    }
    Registry.ReservedSizes[Name] = Reserved;
    return Reserved;
}

UFunction* CreateNativeFunction(UClass* Class, const FNativeFunction& Native);

// Whether a property's memory matches the C++ type of a member of the same size: plain values, object pointers,
// strings, and structs and arrays of registered native structs. A struct the runtime lays out from the mappings has no
// C++ type, and maps have no typed C++ view, so neither can bind.
bool IsCppCompatible(const FProperty* Property) {
    // A struct whose mapped properties do not all have C++ members is larger than its C++ type: an array of it has
    // another element stride.
    if (const FStructProperty* Struct = CastField<FStructProperty>(Property))
        return Struct->Struct != nullptr && Struct->Struct->CppStructOps != nullptr &&
               Struct->Struct->GetStructureSize() == Struct->Struct->CppStructOps->Size;
    if (const FArrayProperty* Array = CastField<FArrayProperty>(Property)) return Array->Inner != nullptr && IsCppCompatible(Array->Inner);
    if (CastField<FSetProperty>(Property) != nullptr || CastField<FMapProperty>(Property) != nullptr) return false;
    if (const FBoolProperty* Bool = CastField<FBoolProperty>(Property)) return Bool->IsNativeBool();
    return true;
}

// Adds the properties of a mappings schema to a native struct or class, binding those that the C++ type has as members,
// and the unversioned schema of its own indices (with the mappings' gaps).
void AddMappedProperties(UStruct* Struct, const StructSchema* Schema, std::span<const FNativeMember> Members, TArray<FProperty*>& Unbound) {
    Struct->OwnUnversionedSlots.Empty();
    if (Schema == nullptr) return;
    TArray<std::pair<FName, FProperty*>> Created;
    for (const PropertyInfo& Info : Schema->properties) {
        if (Info.arrayIndex != 0 || Info.type == nullptr) continue;
        FProperty* Property = CreatePropertyFromMappings(Info.name, *Info.type, Info.arrayDim, Struct);
        if (Property == nullptr) {
            ReportLoadProblem("native.property_type", Struct->GetName() + FString(".") + FString(Info.name.toString()));
            continue;
        }
        const FNativeMember* Member = nullptr;
        for (const FNativeMember& Candidate : Members)
            if (Info.name == FName(Candidate.Name)) Member = &Candidate;
        if (Member != nullptr) {
            Property->Link();
            if (Property->GetSize() != Member->Size || !IsCppCompatible(Property)) {
                ReportLoadProblem("native.member_layout", Struct->GetName() + FString(".") + FString(Member->Name));
                Member = nullptr;
            }
        }
        if (Member != nullptr) {
            Property->Offset_Internal = Member->Offset;
            Property->FlagsPrivate = Property->FlagsPrivate | RF_MarkAsNative;
            Property->PropertyFlags = EPropertyFlags(uint64(Property->PropertyFlags) | Member->PropertyFlags);
            if (Member->RepNotifyFunc != nullptr) Property->RepNotifyFunc = FName(Member->RepNotifyFunc);
        } else {
            Unbound.Add(Property);
        }
        Struct->AddCppProperty(Property);
        Created.Add({Info.name, Property});
    }
    Struct->OwnUnversionedSlots.Reserve(Schema->propertyCount);
    for (uint16 Index = 0; Index < Schema->propertyCount; Index++) {
        const PropertyInfo* Slot = Schema->slots[Index];
        FProperty* Property = nullptr;
        if (Slot != nullptr)
            for (const auto& [Name, CreatedProperty] : Created)
                if (Name == Slot->name) {
                    Property = CreatedProperty;
                    break;
                }
        Struct->OwnUnversionedSlots.Add(UStruct::FUnversionedSlot{Property, Slot ? static_cast<int32>(Slot->arrayIndex) : 0});
    }
}

// UStruct::Link for a native type: the C++ members keep their offsets; the other properties follow from Start.
void LinkNativeStruct(UStruct* Struct, int32 Start) {
    int32 Size = Start;
    int32 Alignment = Struct->GetSuperStruct() ? Struct->GetSuperStruct()->GetMinAlignment() : 1;
    for (FField* Field = Struct->ChildProperties; Field != nullptr; Field = Field->Next) {
        FProperty* Property = CastField<FProperty>(Field);
        if (Property == nullptr) continue;
        if (Property->HasAnyFlags(RF_MarkAsNative)) {
            int32 Offset = Property->Offset_Internal;
            Property->Link();
            Property->Offset_Internal = Offset;
        } else {
            Property->Link();
            Size = Property->SetupOffset(Size);
        }
        Alignment = std::max(Alignment, Property->GetMinAlignment());
    }
    Struct->PropertiesSize = Size;
    Struct->MinAlignment = Alignment;
    Struct->BuildPropertyChains();
    if (UScriptStruct* ScriptStruct = Cast<UScriptStruct>(Struct)) ScriptStruct->PrepareCppStructOps();
    if (UClass* Class = Cast<UClass>(Struct))
        for (UField* Child = Class->Children; Child != nullptr; Child = Child->Next)
            if (UFunction* Function = Cast<UFunction>(Child)) Class->AddFunctionToFunctionMap(Function, Function->GetFName());
}

UClass* BuildNativeClass(FName Name);

void ProcessPendingOuters() {
    FNativeRegistry& Registry = GetRegistry();
    if (Registry.bBootstrapping) return;
    while (Registry.PendingOuters.Num() > 0) {
        auto [Object, PackageName] = Registry.PendingOuters.Pop();
        RenameObject(Object, Object->GetFName(), CreatePackage(NameToString(PackageName)));
    }
}

// Constructs a native type object of a C++ type (UClass, UScriptStruct, UEnum) without the object system's full
// path: the bootstrap types cannot be laid out by their own (unbuilt) classes yet.
template <class T>
T* ConstructTypeObject(UClass* MetaClass, FName Name) {
    UClass* AllocationClass = MetaClass;
    int32 Size = sizeof(T);
    if (AllocationClass != nullptr && AllocationClass->IsLinked()) Size = std::max(Size, AllocationClass->GetPropertiesSize());
    void* Memory = RuntimeMalloc(static_cast<std::size_t>(Size), 16);
    std::memset(Memory, 0, static_cast<std::size_t>(Size));
    T* Object = static_cast<T*>(Memory);
    SetPendingIdentity(Object, MetaClass ? MetaClass : reinterpret_cast<UClass*>(Object), nullptr, Name, RF_Public | RF_MarkAsNative);
    InternalConstructor<T>(FObjectInitializer(Object, nullptr, false, false));
    return Object;
}

UClass* BuildNativeClass(FName Name) {
    FNativeRegistry& Registry = GetRegistry();
    if (auto Found = Registry.Classes.find(Name); Found != Registry.Classes.end()) return Found->second;
    auto InfoIt = Registry.ClassInfos.find(Name);
    const FNativeClassInfo* Info = InfoIt != Registry.ClassInfos.end() ? InfoIt->second : nullptr;
    const StructSchema* Schema = FindSchema(Name);
    bool bScriptClass = Registry.ScriptClassPackages.contains(Name);
    if (Info == nullptr && !bScriptClass) return nullptr;

    bool bIsMetaClass = Name == FName("Class");
    UClass* MetaClass = bIsMetaClass ? nullptr : BuildNativeClass(FName("Class"));
    // Building the metaclass builds its supers, which can include this class (Object): build each class once.
    if (auto Found = Registry.Classes.find(Name); Found != Registry.Classes.end()) return Found->second;
    UClass* Class = ConstructTypeObject<UClass>(MetaClass, MakeName(Name.getText()));
    Registry.Classes[Name] = Class;

    FName SuperName;
    if (Info != nullptr) {
        SuperName = Info->SuperName ? FName(Info->SuperName) : FName();
    } else if (Schema != nullptr) {
        SuperName = Schema->superName;
    } else if (Name != FName("Object")) {
        SuperName = FName("Object");
    }
    UClass* Super = SuperName.isNone() ? nullptr : BuildNativeClass(SuperName);
    if (!SuperName.isNone() && Super == nullptr) {
        ReportLoadProblem("native.super_missing", NameToString(Name) + FString(" : ") + NameToString(SuperName));
        Super = BuildNativeClass(FName("Object"));
    }
    Class->SetSuperStruct(Super);
    Class->ClassFlags = EClassFlags(uint32(CLASS_Native) | (IsIntrinsic(Name) ? uint32(CLASS_Intrinsic) : 0u) | (Info ? Info->ClassFlags : 0u));
    if (Super != nullptr) Class->ClassFlags = EClassFlags(Class->ClassFlags | (Super->ClassFlags & (CLASS_DefaultToInstanced | CLASS_HasInstancedReference)));
    Class->ClassCastFlags = (Super ? Super->ClassCastFlags : 0) | (Info ? Info->CastFlags : 0);
    // The nearest registered C++ class builds the object.
    if (Info != nullptr) {
        Class->ClassConstructor = Info->Constructor;
        Class->NativeSize = Info->Size;
    } else if (Super != nullptr) {
        Class->ClassConstructor = Super->ClassConstructor;
        Class->NativeSize = Super->NativeSize;
    }

    // Reflected properties: intrinsic classes keep only their C++ members.
    TArray<FProperty*> Unbound;
    AddMappedProperties(Class, IsIntrinsic(Name) ? nullptr : Schema, Info ? Info->Members : std::span<const FNativeMember>(), Unbound);
    int32 Start = Super ? Super->GetPropertiesSize() : 0;
    Start = std::max(Start, Class->NativeSize);
    if (Unbound.Num() > 0 && Info != nullptr) Start = std::max(Start, GetReservedSize(Name));
    LinkNativeStruct(Class, Start);

    // Native functions: the class's own registration, then the tables other modules add for it.
    if (Info != nullptr)
        for (const FNativeFunction& Native : Info->Functions) CreateNativeFunction(Class, Native);
    if (auto Tables = Registry.FunctionTables.find(Name); Tables != Registry.FunctionTables.end())
        for (std::span<const FNativeFunction> Table : Tables->second)
            for (const FNativeFunction& Native : Table) CreateNativeFunction(Class, Native);

    Registry.PendingOuters.Add({Class, Info ? FName(Info->Package) : GetPackageName(Name)});
    ProcessPendingOuters();
    return Class;
}

// A property of a native parameter type string (FNativeParam). Consumes the type from Type; null when it names a
// type the runtime does not have.
FProperty* CreatePropertyFromTypeString(FName Name, std::string_view& Type, FFieldVariant Owner) {
    auto TakeIdentifier = [&Type]() {
        std::size_t End = 0;
        while (End < Type.size() && Type[End] != ':' && Type[End] != '<' && Type[End] != '>' && Type[End] != ',') End++;
        std::string_view Identifier = Type.substr(0, End);
        Type.remove_prefix(End);
        return Identifier;
    };
    auto Expect = [&Type](char Char) {
        if (Type.empty() || Type.front() != Char) return false;
        Type.remove_prefix(1);
        return true;
    };
    std::string_view Kind = TakeIdentifier();
    if (Kind == "bool") {
        auto* Bool = new FBoolProperty(Owner, Name);
        Bool->SetBoolSize(1, true);
        return Bool;
    }
    if (Kind == "int8") return new FInt8Property(Owner, Name);
    if (Kind == "int16") return new FInt16Property(Owner, Name);
    if (Kind == "int32") return new FIntProperty(Owner, Name);
    if (Kind == "int64") return new FInt64Property(Owner, Name);
    if (Kind == "uint8") return new FByteProperty(Owner, Name);
    if (Kind == "uint16") return new FUInt16Property(Owner, Name);
    if (Kind == "uint32") return new FUInt32Property(Owner, Name);
    if (Kind == "uint64") return new FUInt64Property(Owner, Name);
    if (Kind == "float") return new FFloatProperty(Owner, Name);
    if (Kind == "double") return new FDoubleProperty(Owner, Name);
    if (Kind == "Name") return new FNameProperty(Owner, Name);
    if (Kind == "Str") return new FStrProperty(Owner, Name);
    if (Kind == "Text") return new FTextProperty(Owner, Name);
    if (Kind == "FieldPath") return new FFieldPathProperty(Owner, Name);
    if (Kind == "Delegate") return new FDelegateProperty(Owner, Name);
    if (Kind == "MulticastDelegate") return new FMulticastInlineDelegateProperty(Owner, Name);
    if (Kind == "Array" || Kind == "Set" || Kind == "Map") {
        if (!Expect('<')) return nullptr;
        FProperty* Result = nullptr;
        if (Kind == "Array") {
            auto* Array = new FArrayProperty(Owner, Name);
            Array->Inner = CreatePropertyFromTypeString(Name, Type, Array);
            Result = Array;
            if (Array->Inner == nullptr) Result = nullptr;
        } else if (Kind == "Set") {
            auto* Set = new FSetProperty(Owner, Name);
            Set->ElementProp = CreatePropertyFromTypeString(Name, Type, Set);
            Result = Set;
            if (Set->ElementProp == nullptr) Result = nullptr;
        } else {
            auto* Map = new FMapProperty(Owner, Name);
            Map->KeyProp = CreatePropertyFromTypeString(Name, Type, Map);
            Map->ValueProp = Map->KeyProp && Expect(',') ? CreatePropertyFromTypeString(Name, Type, Map) : nullptr;
            Result = Map;
            if (Map->KeyProp == nullptr || Map->ValueProp == nullptr) Result = nullptr;
        }
        if (Result == nullptr || !Expect('>')) return nullptr;
        return Result;
    }
    if (!Expect(':')) return nullptr;
    FName TypeName(TakeIdentifier());
    if (Kind == "Struct") {
        UScriptStruct* Struct = FindNativeStruct(TypeName);
        if (Struct == nullptr) return nullptr;
        auto* StructProperty = new FStructProperty(Owner, Name);
        StructProperty->Struct = Struct;
        return StructProperty;
    }
    if (Kind == "Byte") {
        auto* Byte = new FByteProperty(Owner, Name);
        Byte->Enum = FindNativeEnum(TypeName);
        return Byte;
    }
    if (Kind == "Enum") {
        if (!Expect(':')) return nullptr;
        auto* Enum = new FEnumProperty(Owner, Name);
        Enum->Enum = FindNativeEnum(TypeName);
        Enum->UnderlyingProp = CastField<FNumericProperty>(CreatePropertyFromTypeString(Name, Type, Enum));
        if (Enum->UnderlyingProp == nullptr) return nullptr;
        return Enum;
    }
    UClass* Class = BuildNativeClass(TypeName);
    if (Class == nullptr) Class = UObject::StaticClass();
    if (Kind == "Object") {
        auto* Object = new FObjectProperty(Owner, Name);
        Object->PropertyClass = Class;
        return Object;
    }
    if (Kind == "Class") {
        auto* ClassProperty = new FClassProperty(Owner, Name);
        ClassProperty->PropertyClass = UClass::StaticClass();
        ClassProperty->MetaClass = Class;
        return ClassProperty;
    }
    if (Kind == "SoftObject") {
        auto* Soft = new FSoftObjectProperty(Owner, Name);
        Soft->PropertyClass = Class;
        return Soft;
    }
    if (Kind == "SoftClass") {
        auto* SoftClass = new FSoftClassProperty(Owner, Name);
        SoftClass->PropertyClass = UClass::StaticClass();
        SoftClass->MetaClass = Class;
        return SoftClass;
    }
    if (Kind == "WeakObject") {
        auto* Weak = new FWeakObjectProperty(Owner, Name);
        Weak->PropertyClass = Class;
        return Weak;
    }
    if (Kind == "Interface") {
        auto* Interface = new FInterfaceProperty(Owner, Name);
        Interface->InterfaceClass = Class;
        return Interface;
    }
    return nullptr;
}

UFunction* CreateNativeFunction(UClass* Class, const FNativeFunction& Native) {
    UFunction* Function = ConstructTypeObject<UFunction>(BuildNativeClass(FName("Function")), MakeName(Native.Name));
    RenameObject(Function, Function->GetFName(), Class);
    Function->FunctionFlags = EFunctionFlags(Native.Flags | (Native.Func ? uint32(FUNC_Native) : 0u));
    Function->Func = Native.Func;
    for (const FNativeParam& Param : Native.Params) {
        std::string_view Type(Param.Type);
        FProperty* Property = CreatePropertyFromTypeString(MakeName(Param.Name), Type, Function);
        if (Property == nullptr || !Type.empty()) {
            std::fprintf(stderr, "ue5parse: parameter %s %s of %s.%s has a type the runtime does not have\n", Param.Type, Param.Name,
                         Class->GetName().ToUtf8().c_str(), Native.Name);
            UE5PARSE_CHECK(Property != nullptr && Type.empty());
        }
        Property->PropertyFlags = EPropertyFlags(Param.Flags);
        Function->AddCppProperty(Property);
    }
    Function->Link();
    Class->AddFunctionToFunctionMap(Function, Function->GetFName());
    return Function;
}

}

UClass* FindNativeClass(FName Name) {
    std::lock_guard Lock(GetLoaderMutex());
    return BuildNativeClass(Name);
}

UClass* GetNativeClass(const char* Name) {
    UClass* Class = FindNativeClass(FName(Name));
    UE5PARSE_CHECK(Class != nullptr);
    return Class;
}

UScriptStruct* FindNativeStruct(FName Name) {
    std::lock_guard Lock(GetLoaderMutex());
    FNativeRegistry& Registry = GetRegistry();
    if (auto Found = Registry.Structs.find(Name); Found != Registry.Structs.end()) return Found->second;
    if (Registry.ScriptClassPackages.contains(Name)) return nullptr;
    auto InfoIt = Registry.StructInfos.find(Name);
    const FNativeStructInfo* Info = InfoIt != Registry.StructInfos.end() ? InfoIt->second : nullptr;
    const StructSchema* Schema = FindSchema(Name);
    if (Info == nullptr && Schema == nullptr) return nullptr;

    UScriptStruct* Struct = ConstructTypeObject<UScriptStruct>(BuildNativeClass(FName("ScriptStruct")), MakeName(Name.getText()));
    Registry.Structs[Name] = Struct;
    UScriptStruct* Super = nullptr;
    if (Schema != nullptr && !Schema->superName.isNone()) Super = FindNativeStruct(Schema->superName);
    // The extra properties of a C++ super go after the super's C++ size, where this C++ type has its own members.
    if (Info != nullptr && Super != nullptr && Super->GetCppStructOps() != nullptr && Super->GetPropertiesSize() > Super->GetCppStructOps()->Size) {
        std::fprintf(stderr, "ue5parse: %s derives from %s, which has properties its C++ type does not have\n",
                     Struct->GetName().ToUtf8().c_str(), Super->GetName().ToUtf8().c_str());
        UE5PARSE_CHECK(false);
    }
    Struct->SetSuperStruct(Super);
    Struct->StructFlags = STRUCT_Native;
    Struct->CppStructOps = Info ? Info->Ops : nullptr;
    TArray<FProperty*> Unbound;
    AddMappedProperties(Struct, Schema, Info ? Info->Members : std::span<const FNativeMember>(), Unbound);
    if (Info != nullptr) {
        // The properties the C++ type does not have go after it (UScriptStruct handles them besides the C++ ops).
        if (Unbound.Num() > 0 && Info->Ops->Serialize == nullptr) {
            FString Names = Struct->GetName() + FString(":");
            for (const FProperty* Property : Unbound) Names += FString(" ") + Property->GetName();
            ReportLoadProblem("native.member_missing", Names);
        }
        LinkNativeStruct(Struct, Info->Ops->Size);
    } else {
        Struct->Link();
    }
    Registry.PendingOuters.Add({Struct, GetPackageName(Name)});
    ProcessPendingOuters();
    return Struct;
}

UEnum* FindNativeEnum(FName Name) {
    std::lock_guard Lock(GetLoaderMutex());
    FNativeRegistry& Registry = GetRegistry();
    if (auto Found = Registry.Enums.find(Name); Found != Registry.Enums.end()) return Found->second;
    const EnumSchema* Schema = Registry.Provider ? Registry.Provider->getMappings().findEnum(Name) : nullptr;
    if (Schema == nullptr) return nullptr;
    UEnum* Enum = ConstructTypeObject<UEnum>(BuildNativeClass(FName("Enum")), MakeName(Name.getText()));
    Registry.Enums[Name] = Enum;
    for (std::size_t i = 0; i < Schema->names.size(); i++)
        Enum->Names.Add({Schema->names[i], static_cast<int64>(Schema->values[i])});
    Registry.PendingOuters.Add({Enum, GetPackageName(Name)});
    ProcessPendingOuters();
    return Enum;
}

UFunction* FindNativeDelegateFunction(FName Name) {
    std::lock_guard Lock(GetLoaderMutex());
    FNativeRegistry& Registry = GetRegistry();
    if (auto Found = Registry.DelegateFunctions.find(Name); Found != Registry.DelegateFunctions.end()) return Found->second;
    auto Found = Registry.DelegateInfos.find(Name);
    if (Found == Registry.DelegateInfos.end()) return nullptr;
    const FNativeDelegateFunctionInfo& Info = *Found->second;
    UPackage* Package = CreatePackage(FString(Info.Package));
    UDelegateFunction* Function = NewObject<UDelegateFunction>(Package, UDelegateFunction::StaticClass(), Name, RF_Public | RF_MarkAsNative);
    Function->FunctionFlags = EFunctionFlags(Info.Flags | FUNC_Public | FUNC_Delegate);
    for (const FNativeParam& Param : Info.Params) {
        std::string_view Type(Param.Type);
        FProperty* Property = CreatePropertyFromTypeString(MakeName(Param.Name), Type, Function);
        if (Property == nullptr || !Type.empty()) {
            std::fprintf(stderr, "ue5parse: parameter %s %s of delegate %s has a type the runtime does not have\n", Param.Type,
                         Param.Name, Info.Name);
            UE5PARSE_CHECK(Property != nullptr && Type.empty());
        }
        Property->PropertyFlags = EPropertyFlags(Param.Flags);
        Function->AddCppProperty(Property);
    }
    Function->Link();
    Registry.DelegateFunctions[Name] = Function;
    return Function;
}

FProperty* CreatePropertyFromMappings(FName Name, const PropertyType& Type, int32 ArrayDim, FFieldVariant Owner) {
    FProperty* Property = nullptr;
    auto Inner = [&](const PropertyType* InnerType, FFieldVariant InnerOwner) -> FProperty* {
        return InnerType ? CreatePropertyFromMappings(Name, *InnerType, 1, InnerOwner) : nullptr;
    };
    switch (Type.type) {
    case EPropertyType::ByteProperty: {
        auto* Byte = new FByteProperty(Owner, Name);
        if (!Type.enumName.isNone()) Byte->Enum = FindNativeEnum(Type.enumName);
        Property = Byte;
        break;
    }
    case EPropertyType::BoolProperty: {
        auto* Bool = new FBoolProperty(Owner, Name);
        Bool->SetBoolSize(1, true);
        Property = Bool;
        break;
    }
    case EPropertyType::IntProperty: Property = new FIntProperty(Owner, Name); break;
    case EPropertyType::Int8Property: Property = new FInt8Property(Owner, Name); break;
    case EPropertyType::Int16Property: Property = new FInt16Property(Owner, Name); break;
    case EPropertyType::Int64Property: Property = new FInt64Property(Owner, Name); break;
    case EPropertyType::UInt16Property: Property = new FUInt16Property(Owner, Name); break;
    case EPropertyType::UInt32Property: Property = new FUInt32Property(Owner, Name); break;
    case EPropertyType::UInt64Property: Property = new FUInt64Property(Owner, Name); break;
    case EPropertyType::FloatProperty: Property = new FFloatProperty(Owner, Name); break;
    case EPropertyType::DoubleProperty: Property = new FDoubleProperty(Owner, Name); break;
    case EPropertyType::NameProperty: Property = new FNameProperty(Owner, Name); break;
    case EPropertyType::StrProperty: Property = new FStrProperty(Owner, Name); break;
    case EPropertyType::TextProperty: Property = new FTextProperty(Owner, Name); break;
    case EPropertyType::ObjectProperty: {
        auto* Object = new FObjectProperty(Owner, Name);
        Object->PropertyClass = UObject::StaticClass();
        Property = Object;
        break;
    }
    case EPropertyType::ClassProperty: {
        auto* Class = new FClassProperty(Owner, Name);
        Class->PropertyClass = UClass::StaticClass();
        Class->MetaClass = UObject::StaticClass();
        Property = Class;
        break;
    }
    case EPropertyType::WeakObjectProperty: {
        auto* Weak = new FWeakObjectProperty(Owner, Name);
        Weak->PropertyClass = UObject::StaticClass();
        Property = Weak;
        break;
    }
    case EPropertyType::LazyObjectProperty: {
        auto* Lazy = new FLazyObjectProperty(Owner, Name);
        Lazy->PropertyClass = UObject::StaticClass();
        Property = Lazy;
        break;
    }
    case EPropertyType::SoftObjectProperty:
    case EPropertyType::AssetObjectProperty: {
        auto* Soft = new FSoftObjectProperty(Owner, Name);
        Soft->PropertyClass = UObject::StaticClass();
        Property = Soft;
        break;
    }
    case EPropertyType::SoftClassProperty: {
        auto* SoftClass = new FSoftClassProperty(Owner, Name);
        SoftClass->PropertyClass = UClass::StaticClass();
        SoftClass->MetaClass = UObject::StaticClass();
        Property = SoftClass;
        break;
    }
    case EPropertyType::InterfaceProperty: Property = new FInterfaceProperty(Owner, Name); break;
    case EPropertyType::DelegateProperty: Property = new FDelegateProperty(Owner, Name); break;
    case EPropertyType::MulticastDelegateProperty:
    case EPropertyType::MulticastInlineDelegateProperty: Property = new FMulticastInlineDelegateProperty(Owner, Name); break;
    case EPropertyType::FieldPathProperty: Property = new FFieldPathProperty(Owner, Name); break;
    case EPropertyType::StructProperty: {
        UScriptStruct* Struct = FindNativeStruct(Type.structName);
        if (Struct == nullptr) return nullptr;
        auto* StructProperty = new FStructProperty(Owner, Name);
        StructProperty->Struct = Struct;
        Property = StructProperty;
        break;
    }
    case EPropertyType::ArrayProperty: {
        auto* Array = new FArrayProperty(Owner, Name);
        Array->Inner = Inner(Type.inner, Array);
        if (Array->Inner == nullptr) {
            delete Array;
            return nullptr;
        }
        Property = Array;
        break;
    }
    case EPropertyType::SetProperty: {
        auto* Set = new FSetProperty(Owner, Name);
        Set->ElementProp = Inner(Type.inner, Set);
        if (Set->ElementProp == nullptr) {
            delete Set;
            return nullptr;
        }
        Property = Set;
        break;
    }
    case EPropertyType::MapProperty: {
        auto* Map = new FMapProperty(Owner, Name);
        Map->KeyProp = Inner(Type.inner, Map);
        Map->ValueProp = Inner(Type.value, Map);
        if (Map->KeyProp == nullptr || Map->ValueProp == nullptr) {
            delete Map;
            return nullptr;
        }
        Property = Map;
        break;
    }
    case EPropertyType::EnumProperty: {
        auto* Enum = new FEnumProperty(Owner, Name);
        Enum->Enum = FindNativeEnum(Type.enumName);
        FProperty* Underlying = Type.inner ? Inner(Type.inner, Enum) : new FByteProperty(Enum, Name);
        Enum->UnderlyingProp = CastField<FNumericProperty>(Underlying);
        if (Enum->UnderlyingProp == nullptr) {
            delete Underlying;
            delete Enum;
            return nullptr;
        }
        Property = Enum;
        break;
    }
    default:
        return nullptr;
    }
    Property->ArrayDim = std::max<int32>(ArrayDim, 1);
    return Property;
}

// The mappings name types without their package, so a name has one C++ part: a second registration of a name is a
// mistake of the program (two modules declare the same class).
void RegisterNativeClasses(std::span<const FNativeClassInfo> Classes) {
    for (const FNativeClassInfo& Info : Classes) {
        auto [It, bInserted] = GetRegistry().ClassInfos.try_emplace(FName(Info.Name), &Info);
        if (!bInserted) {
            std::fprintf(stderr, "ue5parse: native class %s is registered by %s and %s\n", Info.Name, It->second->Package, Info.Package);
            UE5PARSE_CHECK(bInserted);
        }
    }
}

void RegisterNativeStructs(std::span<const FNativeStructInfo> Structs) {
    for (const FNativeStructInfo& Info : Structs) {
        auto [It, bInserted] = GetRegistry().StructInfos.try_emplace(FName(Info.Name), &Info);
        if (!bInserted) {
            std::fprintf(stderr, "ue5parse: native struct %s is registered by %s and %s\n", Info.Name, It->second->Package, Info.Package);
            UE5PARSE_CHECK(bInserted);
        }
    }
}

void RegisterNativeFunctions(std::span<const FNativeFunctionTable> Tables) {
    for (const FNativeFunctionTable& Table : Tables) GetRegistry().FunctionTables[FName(Table.ClassName)].Add(Table.Functions);
}

void RegisterNativeDelegateFunctions(std::span<const FNativeDelegateFunctionInfo> Delegates) {
    for (const FNativeDelegateFunctionInfo& Info : Delegates) GetRegistry().DelegateInfos.try_emplace(FName(Info.Name), &Info);
}

// CoreNative.cpp.
void RegisterCoreUObject();

namespace {

void ShutdownRuntime() {
    FLinkerLoad::ResetAll();
    FNativeRegistry& Registry = GetRegistry();
    Registry.Classes.clear();
    Registry.Structs.clear();
    Registry.Enums.clear();
    Registry.Provider = nullptr;
    Registry.Runtime = nullptr;
}

}

FRuntime::FRuntime(const FileProvider& Provider) {
    static std::once_flag CoreUObject;
    std::call_once(CoreUObject, RegisterCoreUObject);
    std::lock_guard Lock(GetLoaderMutex());
    FNativeRegistry& Registry = GetRegistry();
    if (Registry.Runtime != nullptr) ShutdownRuntime();
    Registry.Runtime = this;
    Registry.Provider = &Provider;
    // Native classes and the packages of all script objects, from the global script objects.
    const ScriptObjects& Script = Provider.getScriptObjects();
    for (std::size_t i = 0; i < Script.getCount(); i++) {
        const FScriptObjectEntry& Entry = Script.getEntry(i);
        const FScriptObjectEntry* Outer = Script.getOuter(Entry);
        if (Outer != nullptr && Script.getOuter(*Outer) == nullptr)
            Registry.ScriptObjectPackages[Script.getName(Entry)] = Script.getName(*Outer);
        if (Entry.cdoClassIndex.isNull()) continue;
        const FScriptObjectEntry* ClassEntry = Script.find(Entry.cdoClassIndex);
        if (ClassEntry == nullptr) continue;
        const FScriptObjectEntry* Package = Script.getOuter(*ClassEntry);
        Registry.ScriptClassPackages[Script.getName(*ClassEntry)] = Package ? Script.getName(*Package) : FName();
    }
    // A registered class must be in the package the game has it in (the PACKAGE of its module).
    bool bPackagesMatch = true;
    for (const auto& [Name, Info] : Registry.ClassInfos) {
        auto Found = Registry.ScriptClassPackages.find(Name);
        if (Found == Registry.ScriptClassPackages.end() || Found->second == FName(Info->Package)) continue;
        std::fprintf(stderr, "ue5parse: native class %s is registered in %s, the game has it in %s\n", Info->Name, Info->Package,
                     Found->second.toString().c_str());
        bPackagesMatch = false;
    }
    UE5PARSE_CHECK(bPackagesMatch);
    // The bootstrap: the CoreUObject types before any package exists.
    Registry.bBootstrapping = true;
    for (const char* Name : IntrinsicClassNames) BuildNativeClass(FName(Name));
    Registry.bBootstrapping = false;
    ProcessPendingOuters();
}

FRuntime::~FRuntime() {
    std::lock_guard Lock(GetLoaderMutex());
    if (GetRegistry().Runtime == this) ShutdownRuntime();
}

const FileProvider& GetRuntimeProvider() {
    UE5PARSE_CHECK(GetRegistry().Provider != nullptr);
    return *GetRegistry().Provider;
}

}
