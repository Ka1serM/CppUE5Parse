// ue5load <Paks folder> <file.usmap> object <object path>...
// ue5load <Paks folder> <file.usmap> scan <class>|all [<limit>]
// ue5load <Paks folder> <file.usmap> props <object path>...
//
// Loads objects through the runtime on real game data (UE5_AES_KEY holds the key):
//   object   loads each object and prints its class, its path and a summary of its native data
//   scan     loads every export of a class (or all) and counts the load problems by code; a limit stops early

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"
#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/package/Package.h"
#include "ue5parse/vfs/IoStore.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"
#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_Slot.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotify.h"
#include "ue5parse/runtime/Engine/Animation/AnimNotifies/AnimNotifyState.h"
#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"
#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"
#include "ue5parse/runtime/Engine/Engine/StaticMesh.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/Engine/Texture2D.h"
#include "ue5parse/runtime/Engine/Materials/MaterialInstance.h"

using namespace ue5parse;
namespace rt = ue5parse::runtime;

namespace {

std::map<std::string, int> Problems;
std::map<std::string, std::string> ProblemExamples;
bool bPrintProblems = true;

void OnProblem(const char* Code, const rt::FString& Message) {
    Problems[Code]++;
    ProblemExamples.try_emplace(Code, Message.ToUtf8());
    if (bPrintProblems) std::printf("  problem %s: %s\n", Code, Message.ToUtf8().c_str());
}

// Sets bool, numeric and object variables of an object from "Name=Value,Name=Value". A name is a path through struct
// variables, for example "Params.bFlag"; an object value is the path of an object to load. Fails on a path that does
// not end at such a variable, and on an object that does not load or is not of the variable's class.
bool SetVariables(rt::UObject* Object, std::string_view Assignments) {
    while (!Assignments.empty()) {
        const std::string_view Assignment = Assignments.substr(0, Assignments.find(','));
        Assignments.remove_prefix(std::min(Assignments.size(), Assignment.size() + 1));
        const std::size_t Equals = Assignment.find('=');
        std::string_view Path = Assignment.substr(0, Equals);
        const std::string Value(Equals == std::string_view::npos ? "" : Assignment.substr(Equals + 1));
        const rt::UStruct* Owner = Object->GetClass();
        void* Container = Object;
        rt::FProperty* Property = nullptr;
        while (Owner != nullptr) {
            const std::string_view Name = Path.substr(0, Path.find('.'));
            Path.remove_prefix(std::min(Path.size(), Name.size() + 1));
            Property = Owner->FindPropertyByName(rt::FName(std::string(Name)));
            const auto* Struct = rt::CastField<rt::FStructProperty>(Property);
            if (Path.empty() || Struct == nullptr) break;
            Container = Struct->ContainerPtrToValuePtr<void>(Container);
            Owner = Struct->Struct;
        }
        void* Data = Property ? Property->ContainerPtrToValuePtr<void>(Container) : nullptr;
        if (auto* Bool = rt::CastField<rt::FBoolProperty>(Property); Bool && Path.empty()) {
            Bool->SetPropertyValue(Data, Value == "1" || Value == "true");
        } else if (auto* Numeric = rt::CastField<rt::FNumericProperty>(Property); Numeric && Path.empty()) {
            if (Numeric->IsFloatingPoint()) Numeric->SetFloatingPointPropertyValue(Data, std::strtod(Value.c_str(), nullptr));
            else Numeric->SetIntPropertyValue(Data, static_cast<rt::int64>(std::strtoll(Value.c_str(), nullptr, 10)));
        } else if (auto* ObjectProperty = rt::CastField<rt::FObjectPropertyBase>(Property); ObjectProperty && Path.empty()) {
            rt::UObject* Loaded = rt::StaticLoadObject(nullptr, nullptr, rt::FString(Value));
            if (Loaded == nullptr || !Loaded->IsA(ObjectProperty->PropertyClass)) {
                std::fprintf(stderr, "cannot load %s as a %s\n", Value.c_str(), ObjectProperty->PropertyClass->GetName().ToUtf8().c_str());
                return false;
            }
            ObjectProperty->SetObjectPropertyValue(Data, Loaded);
        } else {
            std::fprintf(stderr, "no bool, numeric or object variable %.*s\n", static_cast<int>(Equals == std::string_view::npos ? Assignment.size() : Equals),
                         Assignment.data());
            return false;
        }
    }
    return true;
}

bool bSampleAnimations = false;
long Sampled = 0, BadPoses = 0;

// Samples a sequence on its own skeleton at its start, middle and end; a non-finite or unnormalized bone is a bad pose.
void SampleOnSkeleton(rt::UAnimSequence* Sequence) {
    if (Sequence == nullptr || Sequence->GetSkeleton() == nullptr) return;
    rt::USkeleton* Skeleton = Sequence->GetSkeleton();
    rt::TArray<rt::FBoneIndexType> RequiredBones;
    for (rt::int32 b = 0; b < Skeleton->GetReferenceSkeleton().GetNum(); b++) RequiredBones.Add(static_cast<rt::FBoneIndexType>(b));
    rt::FBoneContainer BoneContainer(RequiredBones, *Skeleton);
    if (!BoneContainer.IsValid()) return;
    Sampled++;
    for (double Fraction : {0.0, 0.5, 1.0}) {
        rt::FCompactPose Pose;
        Pose.SetBoneContainer(&BoneContainer);
        rt::FBlendedCurve Curve;
        rt::FAnimationPoseData PoseData(Pose, Curve);
        Sequence->GetAnimationPose(PoseData, rt::FAnimExtractContext(Fraction * Sequence->GetPlayLength()));
        for (const rt::FTransform& T : Pose.GetBones()) {
            const rt::FQuat Q = T.GetRotation();
            const double Length = std::sqrt(Q.X * Q.X + Q.Y * Q.Y + Q.Z * Q.Z + Q.W * Q.W);
            if (!std::isfinite(T.GetTranslation().X + T.GetTranslation().Y + T.GetTranslation().Z) || std::abs(Length - 1.0) > 0.01) {
                if (BadPoses++ < 10) std::printf("  bad pose: %s at %.3f\n", Sequence->GetPathName().ToUtf8().c_str(), Fraction);
                return;
            }
        }
    }
}

// Prints a property value in one line: bools, numbers, enums, names, objects, structs and arrays of them.
void PrintValue(const rt::FProperty* Property, const void* Data, int Depth) {
    if (const auto* Bool = rt::CastField<rt::FBoolProperty>(Property)) {
        std::printf("%d", Bool->GetPropertyValue(Data) ? 1 : 0);
    } else if (const auto* Enum = rt::CastField<rt::FEnumProperty>(Property)) {
        std::printf("%s", Enum->GetEnum()->GetNameStringByValue(Enum->GetUnderlyingProperty()->GetSignedIntPropertyValue(Data)).ToUtf8().c_str());
    } else if (const auto* Numeric = rt::CastField<rt::FNumericProperty>(Property)) {
        if (Numeric->IsEnum()) std::printf("%s", Numeric->GetIntPropertyEnum()->GetNameStringByValue(Numeric->GetSignedIntPropertyValue(Data)).ToUtf8().c_str());
        else if (Numeric->IsFloatingPoint()) std::printf("%g", Numeric->GetFloatingPointPropertyValue(Data));
        else std::printf("%lld", static_cast<long long>(Numeric->GetSignedIntPropertyValue(Data)));
    } else if (rt::CastField<rt::FNameProperty>(Property) != nullptr) {
        std::printf("%s", static_cast<const rt::FName*>(Data)->toString().c_str());
    } else if (const auto* Object = rt::CastField<rt::FObjectPropertyBase>(Property)) {
        std::printf("%s", rt::GetPathNameSafe(Object->GetObjectPropertyValue(Data)).ToUtf8().c_str());
    } else if (const auto* Struct = rt::CastField<rt::FStructProperty>(Property); Struct && Depth < 3) {
        std::printf("{");
        for (const rt::FProperty* Member = Struct->Struct->PropertyLink; Member != nullptr; Member = Member->PropertyLinkNext) {
            std::printf("%s=", Member->GetName().ToUtf8().c_str());
            PrintValue(Member, Member->ContainerPtrToValuePtr<void>(Data), Depth + 1);
            std::printf(" ");
        }
        std::printf("}");
    } else if (const auto* Array = rt::CastField<rt::FArrayProperty>(Property); Array && Depth < 3) {
        rt::FScriptArrayHelper Helper(Array, Data);
        std::printf("[");
        for (rt::int32 i = 0; i < Helper.Num(); i++) {
            PrintValue(Array->Inner, Helper.GetRawPtr(i), Depth + 1);
            std::printf(", ");
        }
        std::printf("]");
    } else {
        std::printf("<%s>", Property->GetClass()->GetName());
    }
}

void PrintProperties(const rt::UObject* Object) {
    std::printf("%s %s\n", Object->GetClass()->GetName().ToUtf8().c_str(), Object->GetPathName().ToUtf8().c_str());
    for (const rt::FProperty* Property = Object->GetClass()->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext) {
        std::printf("  %s = ", Property->GetName().ToUtf8().c_str());
        PrintValue(Property, Property->ContainerPtrToValuePtr<void>(Object), 0);
        std::printf("\n");
    }
}

void PrintSummary(const rt::UObject* Object) {
    std::printf("%s %s\n", Object->GetClass()->GetName().ToUtf8().c_str(), Object->GetPathName().ToUtf8().c_str());
    if (const rt::UTexture2D* Texture = rt::Cast<rt::UTexture2D>(Object)) {
        const rt::FTexturePlatformData* Data = Texture->GetPlatformData();
        if (Data == nullptr) {
            std::printf("  no platform data\n");
            return;
        }
        std::printf("  %dx%d format %d mips %d srgb %d\n", Data->SizeX, Data->SizeY, static_cast<int>(Data->PixelFormat),
                    Data->Mips.Num(), Texture->SRGB ? 1 : 0);
    } else if (const rt::UStaticMesh* Mesh = rt::Cast<rt::UStaticMesh>(Object)) {
        std::printf("  lods %d materials %d sockets %d\n", Mesh->GetNumLODs(), Mesh->GetStaticMaterials().Num(), Mesh->Sockets.Num());
        if (const rt::FStaticMeshRenderData* Data = Mesh->GetRenderData()) {
            for (rt::int32 i = 0; i < Data->LODResources.Num(); i++) {
                const rt::FStaticMeshLODResources& Lod = Data->LODResources[i];
                std::printf("  lod %d: sections %d vertices %u indices %d\n", i, Lod.Sections.Num(),
                            Lod.VertexBuffers.PositionVertexBuffer.GetNumVertices(), Lod.IndexBuffer.GetNumIndices());
            }
        }
    } else if (const auto* AnimClass = rt::Cast<rt::UAnimBlueprintGeneratedClass>(Object)) {
        const_cast<rt::UAnimBlueprintGeneratedClass*>(AnimClass)->GetDefaultObject();
        std::printf("  graph nodes %d node data %d machines %d root %s target %s\n", AnimClass->GetAnimNodeProperties().Num(),
                    AnimClass->AnimNodeData.Num(), AnimClass->BakedStateMachines.Num(),
                    AnimClass->GetRootAnimClass()->GetPathName().ToUtf8().c_str(),
                    rt::GetPathNameSafe(AnimClass->TargetSkeleton).ToUtf8().c_str());
        if (std::getenv("UE5LOAD_NODES"))
            for (rt::int32 Index = 0; Index < AnimClass->GetAnimNodeProperties().Num(); ++Index)
                std::printf("  node %d: %s -> %d\n", Index, AnimClass->GetAnimNodeProperties()[Index]->Struct->GetName().ToUtf8().c_str(),
                            AnimClass->GetLinkTarget(Index));
    } else if (const rt::USkeletalMesh* Mesh = rt::Cast<rt::USkeletalMesh>(Object)) {
        std::printf("  bones %d materials %d skeleton %s\n", Mesh->GetRefSkeleton().GetRawBoneNum(), Mesh->GetMaterials().Num(),
                    rt::GetPathNameSafe(Mesh->GetSkeleton()).ToUtf8().c_str());
        if (const rt::FSkeletalMeshRenderData* Data = Mesh->GetResourceForRendering()) {
            for (rt::int32 i = 0; i < Data->LODRenderData.Num(); i++) {
                const rt::FSkeletalMeshLODRenderData& Lod = Data->LODRenderData[i];
                int Disabled = 0;
                for (const rt::FSkelMeshRenderSection& Section : Lod.RenderSections) Disabled += Section.bDisabled ? 1 : 0;
                std::printf("  lod %d: sections %d (disabled %d) vertices %u\n", i, Lod.RenderSections.Num(), Disabled, Lod.GetNumVertices());
            }
        }
    } else if (const rt::UAnimSequence* Sequence = rt::Cast<rt::UAnimSequence>(Object)) {
        const rt::FCompressedAnimSequence& Data = Sequence->CompressedData;
        std::printf("  length %g keys %d tracks %d curves %d codec %s (%s) bytes %zu curve bytes %d notifies %d additive %d\n",
                    Sequence->SequenceLength, Sequence->GetNumberOfSampledKeys(), Data.CompressedTrackToSkeletonMapTable.Num(),
                    Data.IndexedCurveNames.Num(), rt::GetNameSafe(Data.BoneCompressionCodec ? Data.BoneCompressionCodec->GetClass() : nullptr).ToUtf8().c_str(),
                    Data.BoneCodecDDCHandle.ToUtf8().c_str(), Data.CompressedByteStream.size(), Data.CompressedCurveByteStream.Num(),
                    Sequence->Notifies.Num(), static_cast<int>(Sequence->AdditiveAnimType.GetValue()));
        std::printf("  skeleton %s\n", rt::GetPathNameSafe(Sequence->GetSkeleton()).ToUtf8().c_str());
    } else if (const rt::UAnimMontage* Montage = rt::Cast<rt::UAnimMontage>(Object)) {
        std::printf("  length %g sections %d slots %d notifies %d blend in %g out %g\n", Montage->SequenceLength,
                    Montage->CompositeSections.Num(), Montage->SlotAnimTracks.Num(), Montage->Notifies.Num(),
                    Montage->BlendIn.BlendTime, Montage->BlendOut.BlendTime);
    } else if (const rt::UBlendSpace* Blend = rt::Cast<rt::UBlendSpace>(Object)) {
        std::printf("  samples %d grid %d axes %d x %s [%g, %g]\n", Blend->SampleData.Num(), Blend->GridSamples.Num(),
                    Blend->DimensionIndices.Num(), Blend->BlendParameters[0].DisplayName.ToUtf8().c_str(),
                    Blend->BlendParameters[0].Min, Blend->BlendParameters[0].Max);
    } else if (const rt::USkeleton* Skeleton = rt::Cast<rt::USkeleton>(Object)) {
        std::printf("  bones %d tree %d modes", Skeleton->GetReferenceSkeleton().GetRawBoneNum(), Skeleton->BoneTree.Num());
        for (rt::int32 b = 0; b < Skeleton->BoneTree.Num() && b < 12; b++) std::printf(" %s=%d", Skeleton->GetReferenceSkeleton().GetBoneName(b).toString().c_str(), static_cast<int>(Skeleton->BoneTree[b].TranslationRetargetingMode.GetValue()));
        std::printf("\n");
    } else if (const rt::UMaterialInstance* Material = rt::Cast<rt::UMaterialInstance>(Object)) {
        std::printf("  parent %s scalars %d vectors %d textures %d\n", rt::GetPathNameSafe(Material->Parent).ToUtf8().c_str(),
                    Material->ScalarParameterValues.Num(), Material->VectorParameterValues.Num(), Material->TextureParameterValues.Num());
    }
}

}

// The pose links and the name of an anim node, for the graph command. A link's ID is the index of its target node.
// The number of elements of an array property found by name on an object (0 when it has none).
rt::int32 ArrayNumByName(const rt::UObject* Object, const char* Name) {
    const auto* Property = rt::FindFProperty<rt::FArrayProperty>(Object->GetClass(), rt::FName(Name));
    return Property ? rt::FScriptArrayHelper(Property, Property->ContainerPtrToValuePtr<void>(Object)).Num() : 0;
}

// Counts of the animation features the game's assets use: node types, montage blends, transitions, sync markers,
// root motion and notify classes.
struct AnimCensus {
    std::map<std::string, int> Counts;
    void Add(const std::string& Key, int Amount = 1) { Counts[Key] += Amount; }

    void AddNotifies(const rt::TArray<rt::FAnimNotifyEvent>& Notifies, const char* Owner) {
        for (const rt::FAnimNotifyEvent& Event : Notifies) {
            if (Event.NotifyStateClass) Add(std::string("notify state ") + Event.NotifyStateClass->GetClass()->GetName().ToUtf8());
            else if (Event.Notify) Add(std::string("notify ") + Event.Notify->GetClass()->GetName().ToUtf8());
            else if (!Event.NotifyName.isNone()) Add(std::string("notify named (") + Owner + ")");
            if (Event.MontageTickType == rt::EMontageNotifyTickType::BranchingPoint) Add("notify branching point");
            if (Event.NotifyFilterType != rt::ENotifyFilterType::NoFiltering) Add("notify LOD filter");
            if (Event.NotifyTriggerChance < 1.0f) Add("notify trigger chance < 1");
            if (Event.TriggerWeightThreshold > 0.00001f) Add("notify weight threshold");
            if (Event.bTriggerOnFollower) Add("notify on follower");
        }
    }

    void AddObject(rt::UObject* Object) {
        if (auto* Class = rt::Cast<rt::UAnimBlueprintGeneratedClass>(Object)) {
            Add("anim class");
            for (rt::FStructProperty* Node : Class->GetAnimNodeProperties()) Add("node " + Node->Struct->GetName().ToUtf8());
            for (const rt::FBakedAnimationStateMachine& Machine : Class->BakedStateMachines)
                for (const rt::FAnimationTransitionBetweenStates& Transition : Machine.Transitions) {
                    static const char* Logic[] = {"standard", "inertialization", "custom"};
                    Add(std::string("transition ") + Logic[std::min<int>(Transition.LogicType, 2)]);
                    if (Transition.BlendProfile) Add("transition blend profile");
                }
            if (Class->SyncGroupNames.Num()) Add("anim class with sync groups");
            AddNotifies(Class->AnimNotifies, "anim class");
            if (std::getenv("CENSUS_TRACE")) std::fprintf(stderr, "class %s super %s\n", Class->GetPathName().ToUtf8().c_str(), Class->GetSuperStruct()->GetPathName().ToUtf8().c_str());
            if (auto* Defaults = rt::Cast<rt::UAnimInstance>(Class->GetDefaultObject()))
                if (Defaults->RootMotionMode != rt::ERootMotionMode::RootMotionFromMontagesOnly)
                    Add("anim class root motion mode " + std::to_string(Defaults->RootMotionMode.GetValue()));
            return;
        }
        if (auto* Montage = rt::Cast<rt::UAnimMontage>(Object)) {
            Add("montage");
            if (Montage->BlendModeIn == rt::EMontageBlendMode::Inertialization) Add("montage inertial blend in");
            if (Montage->BlendModeOut == rt::EMontageBlendMode::Inertialization) Add("montage inertial blend out");
            if (Montage->BlendProfileIn || Montage->BlendProfileOut) Add("montage blend profile");
            if (Montage->TimeStretchCurve.Markers.Num()) Add("montage time stretch curve");
            if (!Montage->SyncGroup.isNone()) Add("montage sync group");
            if (Montage->bEnableRootMotionTranslation || Montage->bEnableRootMotionRotation) Add("montage root motion");
            if (Montage->SlotAnimTracks.Num() > 1) Add("montage with several slots");
            if (Montage->BranchingPointMarkers.Num()) Add("montage with branching points");
            AddNotifies(Montage->Notifies, "montage");
            return;
        }
        if (auto* Sequence = rt::Cast<rt::UAnimSequence>(Object)) {
            Add("sequence");
            if (ArrayNumByName(Sequence, "AuthoredSyncMarkers")) Add("sequence with sync markers");
            if (Sequence->bEnableRootMotion) Add("sequence root motion");
            if (Sequence->bForceRootLock) Add("sequence force root lock");
            AddNotifies(Sequence->Notifies, "sequence");
            return;
        }
        if (auto* Base = rt::Cast<rt::UAnimSequenceBase>(Object)) {
            Add(Base->GetClass()->GetName().ToUtf8());
            AddNotifies(Base->Notifies, "other");
        }
    }
};

void PrintPoseLinks(const rt::UStruct* Struct, const void* Node) {
    const auto PrintLink = [](const char* Name, const void* Link) {
        const rt::int32 LinkID = *static_cast<const rt::int32*>(Link);
        if (LinkID != rt::INDEX_NONE) std::printf(" %s->%d", Name, LinkID);
    };
    for (const rt::UStruct* Type = Struct; Type != nullptr; Type = Type->GetSuperStruct())
        for (rt::FField* Field = Type->ChildProperties; Field != nullptr; Field = Field->Next) {
            const auto* Property = rt::CastField<rt::FProperty>(Field);
            if (Property == nullptr) continue;
            const std::string Name = Property->GetName().ToUtf8();
            if (const auto* NameProperty = rt::CastField<rt::FNameProperty>(Property); NameProperty && (Name == "SlotName" || Name == "Name"))
                std::printf(" %s=%s", Name.c_str(), NameProperty->ContainerPtrToValuePtr<rt::FName>(Node)->toString().c_str());
            const rt::FStructProperty* LinkStruct = rt::CastField<rt::FStructProperty>(Property);
            const rt::FArrayProperty* LinkArray = rt::CastField<rt::FArrayProperty>(Property);
            if (LinkArray != nullptr) LinkStruct = rt::CastField<rt::FStructProperty>(LinkArray->Inner);
            if (LinkStruct == nullptr || !LinkStruct->Struct->IsChildOf(rt::FPoseLinkBase::StaticStruct())) continue;
            if (LinkArray == nullptr) {
                PrintLink(Name.c_str(), Property->ContainerPtrToValuePtr<void>(Node));
                continue;
            }
            rt::FScriptArrayHelper Array(LinkArray, Property->ContainerPtrToValuePtr<void>(Node));
            for (rt::int32 Element = 0; Element < Array.Num(); Element++) PrintLink(Name.c_str(), Array.GetRawPtr(Element));
        }
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: ue5load <Paks folder> <file.usmap> object <path>... | scan <class> [<limit>]\n");
        return 2;
    }
    MountOptions Options;
    Result<std::vector<std::unique_ptr<FileSource>>> Archives = openLocalFiles(argv[1]);
    if (!Archives) {
        std::fprintf(stderr, "cannot open %s: %s at %s\n", argv[1], getErrorName(Archives.error().code), Archives.error().where);
        return 1;
    }
    Options.archives = std::move(*Archives);
    if (const char* Key = std::getenv("UE5_AES_KEY")) {
        Result<AesKey> Parsed = parseAesKey(Key);
        if (!Parsed) {
            std::fprintf(stderr, "UE5_AES_KEY is not a key\n");
            return 1;
        }
        KeyRing Keys;
        Keys.add(FGuid{}, *Parsed);
        Options.decryptor = std::make_unique<AesDecryptor>(Keys);
    }
    Result<std::unique_ptr<FileSource>> Mappings = openLocalFile(argv[2]);
    if (!Mappings) return 1;
    Options.mappings = std::move(*Mappings);
    Options.game = EGame::GAME_Valorant;
    Result<MountedGame> Game = mount(std::move(Options));
    if (!Game) {
        std::fprintf(stderr, "mount failed: %s\n", getErrorName(Game.error().code));
        return 1;
    }
    FileProvider Provider(std::move(*Game));
    rt::FRuntime Runtime(Provider);
    rt::SetLoadProblemHandler(&OnProblem);

    std::string_view Command = argv[3];
    if (Command == "props") {
        // Every property of each object, with its value.
        for (int i = 4; i < argc; i++) {
            const rt::UObject* Object = rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[i]));
            if (Object == nullptr) std::printf("cannot load %s\n", argv[i]);
            else PrintProperties(Object);
        }
        return 0;
    }
    if (Command == "schema") {
        // The mapped properties of each struct and its supers: the layout a native C++ type must match by name.
        for (int i = 4; i < argc; i++) {
            for (const StructSchema* Schema = Provider.getMappings().findStruct(FName(argv[i])); Schema != nullptr;
                 Schema = Provider.getMappings().findStruct(Schema->superName)) {
                std::printf("%s : %s\n", Schema->name.toString().c_str(), Schema->superName.toString().c_str());
                for (const PropertyInfo& Info : Schema->properties) {
                    if (Info.arrayIndex != 0) continue;
                    std::printf("    %s", getPropertyTypeName(Info.type->type));
                    for (const PropertyType* Type = Info.type; Type != nullptr; Type = Type->inner) {
                        if (!Type->structName.isNone()) std::printf(" %s", Type->structName.toString().c_str());
                        if (!Type->enumName.isNone()) std::printf(" %s", Type->enumName.toString().c_str());
                        if (Type->inner != nullptr) std::printf(" <%s>", getPropertyTypeName(Type->inner->type));
                        if (Type->value != nullptr) std::printf(" -> %s", getPropertyTypeName(Type->value->type));
                    }
                    std::printf(" %s", Info.name.toString().c_str());
                    if (Info.arrayDim > 1) std::printf("[%d]", Info.arrayDim);
                    std::printf("\n");
                }
                if (Schema->superName.isNone()) break;
            }
        }
    } else if (Command == "sample" && argc >= 8) {
        // Samples a sequence on a mesh: <sequence> <mesh> <bone> <time>. Prints the bone's local transform.
        auto* Sequence = rt::Cast<rt::UAnimSequence>(rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[4])));
        auto* Mesh = rt::Cast<rt::USkeletalMesh>(rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[5])));
        if (Sequence == nullptr || Mesh == nullptr) {
            std::fprintf(stderr, "cannot load the sequence or the mesh\n");
            return 1;
        }
        rt::TArray<rt::FBoneIndexType> RequiredBones;
        for (rt::int32 Bone = 0; Bone < Mesh->GetRefSkeleton().GetNum(); Bone++) RequiredBones.Add(static_cast<rt::FBoneIndexType>(Bone));
        rt::FBoneContainer BoneContainer(RequiredBones, *Mesh);
        rt::FCompactPose Pose;
        Pose.SetBoneContainer(&BoneContainer);
        rt::FBlendedCurve Curve;
        rt::FAnimationPoseData PoseData(Pose, Curve);
        Sequence->GetAnimationPose(PoseData, rt::FAnimExtractContext(std::strtod(argv[7], nullptr)));
        const rt::int32 Bone = Mesh->GetRefSkeleton().FindBoneIndex(rt::FName(argv[6]));
        if (Bone == rt::INDEX_NONE) {
            std::fprintf(stderr, "the mesh has no bone %s\n", argv[6]);
            return 1;
        }
        const rt::FTransform& T = Pose[BoneContainer.MakeCompactPoseIndex(rt::FMeshPoseBoneIndex(Bone))];
        std::printf("%s t(%.1f %.1f %.1f) q(%.3f %.3f %.3f %.3f) curves %d\n", argv[6], T.GetTranslation().X, T.GetTranslation().Y,
                    T.GetTranslation().Z, T.GetRotation().X, T.GetRotation().Y, T.GetRotation().Z, T.GetRotation().W, Curve.Num());
    } else if (Command == "anim" && argc >= 8) {
        // Runs an anim class on a mesh: <class> <mesh> <frames> <delta seconds>. Prints the component space
        // translation of a few bones at the end, and fails on a bone transform that is not finite.
        auto* AnimClass = rt::Cast<rt::UClass>(rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[4])));
        auto* Mesh = rt::Cast<rt::USkeletalMesh>(rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[5])));
        if (AnimClass == nullptr || Mesh == nullptr) {
            std::fprintf(stderr, "cannot load the class or the mesh\n");
            return 1;
        }
        const long Frames = std::strtol(argv[6], nullptr, 10);
        const float DeltaSeconds = std::strtof(argv[7], nullptr);
        auto* Component = rt::NewObject<rt::USkeletalMeshComponent>(nullptr);
        Component->SkinnedAsset = Mesh;
        Component->AnimClass = AnimClass;
        // Registration makes the anim instances (InitAnim), as in a world.
        Component->RegisterComponentWithWorld(rt::UWorld::CreateWorld(rt::EWorldType::Game));
        // UE5LOAD_NO_POST=1 runs without the mesh's post process anim Blueprint.
        if (std::getenv("UE5LOAD_NO_POST")) Component->SetDisablePostProcessBlueprint(true);
        if (Component->GetPostProcessInstance() != nullptr)
            std::printf("post process instance %s\n", Component->GetPostProcessInstance()->GetClass()->GetName().ToUtf8().c_str());
        // UE5LOAD_MONTAGE=<path> plays a montage from the first frame and prints its weight and position each frame.
        rt::UAnimMontage* Montage = nullptr;
        if (const char* MontagePath = std::getenv("UE5LOAD_MONTAGE")) {
            Montage = rt::LoadObject<rt::UAnimMontage>(nullptr, rt::FString(MontagePath));
            // UE5LOAD_MONTAGE_SLOT=<slot> plays the montage's first animation in that slot as a dynamic montage.
            if (const char* Slot = std::getenv("UE5LOAD_MONTAGE_SLOT"); Slot != nullptr && Montage != nullptr)
                Montage = Component->GetAnimInstance()->PlaySlotAnimationAsDynamicMontage(
                    Montage->SlotAnimTracks[0].AnimTrack.AnimSegments[0].GetAnimReference(), rt::FName(Slot));
            else if (Montage != nullptr)
                Component->GetAnimInstance()->Montage_Play(Montage);
            const float Length = Montage ? Montage->GetPlayLength() : 0.0f;
            std::printf("montage %s: play length %.3f, slot %s\n", MontagePath, Length,
                        Montage ? Montage->SlotAnimTracks[0].SlotName.toString().c_str() : "-");
            if (Montage != nullptr)
                for (const rt::FCompositeSection& Section : Montage->CompositeSections)
                    std::printf("  section %s at %.3f, next %s\n", Section.SectionName.toString().c_str(), Section.GetTime(),
                                Section.NextSectionName.toString().c_str());
            const auto* Class = rt::Cast<rt::UAnimBlueprintGeneratedClass>(AnimClass);
            for (rt::FStructProperty* Property : Class->GetAnimNodeProperties())
                if (Property->Struct == rt::FAnimNode_Slot::StaticStruct())
                    std::printf("  graph slot %s\n",
                                Property->ContainerPtrToValuePtr<rt::FAnimNode_Slot>(Component->GetAnimInstance())->SlotName.toString().c_str());
        }
        // UE5LOAD_SET=<name>=<value>,... sets variables of the anim instance before each frame.
        const char* Variables = std::getenv("UE5LOAD_SET");
        long BadFrames = 0;
        const auto Start = std::chrono::steady_clock::now();
        for (long Frame = 0; Frame < Frames; Frame++) {
            if (Variables != nullptr && !SetVariables(Component->GetAnimInstance(), Variables)) return 1;
            Component->TickAnimation(DeltaSeconds);
            Component->RefreshBoneTransforms();
            if (Montage != nullptr) {
                const rt::UAnimInstance* Instance = Component->GetAnimInstance();
                const rt::FAnimMontageInstance* Playing = Instance->GetActiveInstanceForMontage(Montage);
                float SlotWeight, SourceWeight, TotalWeight;
                Component->GetAnimInstance()->GetProxyOnGameThread().GetSlotWeight(Montage->SlotAnimTracks[0].SlotName, SlotWeight, SourceWeight, TotalWeight);
                std::printf("  frame %ld: montages %d, active %d, position %.3f, weight %.3f, section %s, slot weight %.3f (evaluation %.3f)\n", Frame,
                            Instance->MontageInstances.Num(), Playing != nullptr, Instance->Montage_GetPosition(Montage),
                            Instance->MontageInstances.Num() ? Instance->MontageInstances[0]->GetWeight() : 0.0f,
                            Instance->Montage_GetCurrentSection(Montage).toString().c_str(),
                            Instance->Blueprint_GetSlotMontageLocalWeight(Montage->SlotAnimTracks[0].SlotName), SlotWeight);
            }
            for (const rt::FTransform& Transform : Component->GetComponentSpaceTransforms())
                if (Transform.ContainsNaN()) {
                    BadFrames++;
                    break;
                }
        }
        const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
        const rt::TArray<rt::FTransform>& Bones = Component->GetComponentSpaceTransforms();
        const rt::FReferenceSkeleton& Skeleton = Mesh->GetRefSkeleton();
        for (rt::int32 Bone = 0; Bone < Bones.Num(); Bone += std::max(1, Bones.Num() / 8)) {
            const rt::FVector T = Bones[Bone].GetTranslation();
            std::printf("  %-24s %9.3f %9.3f %9.3f\n", Skeleton.GetBoneName(Bone).toString().c_str(), T.X, T.Y, T.Z);
        }
        int MovedBones = 0;
        const rt::TArray<rt::FTransform>& Local = Component->GetBoneSpaceTransforms();
        for (rt::int32 Bone = 0; Bone < Local.Num(); Bone++)
            if (!Local[Bone].Equals(Skeleton.GetRefBonePose()[Bone], 1e-3)) MovedBones++;
        // UE5LOAD_DUMP=<file> writes each bone's local transform at the end.
        if (const char* DumpPath = std::getenv("UE5LOAD_DUMP"))
            if (FILE* Dump = std::fopen(DumpPath, "w")) {
                for (rt::int32 Bone = 0; Bone < Local.Num(); Bone++) {
                    const rt::FTransform& T = Local[Bone];
                    std::fprintf(Dump, "%s %.4f %.4f %.4f %.5f %.5f %.5f %.5f\n", Skeleton.GetBoneName(Bone).toString().c_str(), T.GetTranslation().X,
                                 T.GetTranslation().Y, T.GetTranslation().Z, T.GetRotation().X, T.GetRotation().Y, T.GetRotation().Z, T.GetRotation().W);
                }
                std::fclose(Dump);
            }
        std::printf("%ld frames, %ld with bad poses, %d of %d bones off the reference pose, %.3f ms per frame\n", Frames, BadFrames, MovedBones,
                    Local.Num(), Seconds * 1000.0 / double(Frames));
    } else if (Command == "graph" && argc >= 5) {
        // The anim nodes of a class: each node's index, struct, slot or state machine name, and the nodes its pose
        // links point to.
        auto* AnimClass = rt::Cast<rt::UAnimBlueprintGeneratedClass>(rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[4])));
        if (AnimClass == nullptr) {
            std::fprintf(stderr, "cannot load the class\n");
            return 1;
        }
        const rt::UObject* Defaults = AnimClass->GetDefaultObject();
        const rt::TArray<rt::FStructProperty*>& Nodes = AnimClass->GetAnimNodeProperties();
        for (rt::int32 NodeIndex = 0; NodeIndex < Nodes.Num(); NodeIndex++) {
            const void* Node = Nodes[NodeIndex]->ContainerPtrToValuePtr<void>(Defaults);
            std::printf("%4d %s", NodeIndex, Nodes[NodeIndex]->Struct->GetName().ToUtf8().c_str());
            PrintPoseLinks(Nodes[NodeIndex]->Struct, Node);
            std::printf("\n");
        }
    } else if (Command == "object") {
        for (int i = 4; i < argc; i++) {
            rt::UObject* Object = rt::StaticLoadObject(nullptr, nullptr, rt::FString(argv[i]));
            if (Object == nullptr) {
                std::printf("%s: not loaded\n", argv[i]);
                continue;
            }
            PrintSummary(Object);
            if (std::getenv("UE5LOAD_CHAIN"))
                for (const rt::UStruct* C = Object->GetClass(); C != nullptr; C = C->GetSuperStruct())
                    std::printf("  chain %s %p\n", C->GetPathName().ToUtf8().c_str(), static_cast<const void*>(C));
        }
    } else if (Command == "sample" && argc >= 10) {
        // sample <clip> <mesh> <bone> <start> <end> <step>: the local transforms of the bone's chain from the root.
        rt::UAnimSequence* Sequence = rt::LoadObject<rt::UAnimSequence>(nullptr, rt::FString(argv[4]));
        rt::USkeletalMesh* Mesh = rt::LoadObject<rt::USkeletalMesh>(nullptr, rt::FString(argv[5]));
        if (Sequence == nullptr || Mesh == nullptr) {
            std::printf("%s on %s: not loaded\n", argv[4], argv[5]);
            return 1;
        }
        const rt::FReferenceSkeleton& RefSkeleton = Mesh->GetRefSkeleton();
        rt::TArray<rt::FBoneIndexType> RequiredBones;
        for (rt::int32 i = 0; i < RefSkeleton.GetNum(); i++) RequiredBones.Add(static_cast<rt::FBoneIndexType>(i));
        rt::FBoneContainer BoneContainer(RequiredBones, *Mesh);
        std::vector<int> Chain;
        for (int Bone = RefSkeleton.FindBoneIndex(FName(argv[6])); Bone != rt::INDEX_NONE; Bone = RefSkeleton.GetParentIndex(Bone)) Chain.insert(Chain.begin(), Bone);
        std::printf("%s on %s: length %.3f\n", argv[4], argv[5], Sequence->GetPlayLength());
        const double Start = std::strtod(argv[7], nullptr), End = std::strtod(argv[8], nullptr), Step = std::strtod(argv[9], nullptr);
        for (int Index = 0; Start + Index * Step <= End + 1e-9; Index++) {
            const double Time = Start + Index * Step;
            rt::FCompactPose Pose;
            Pose.SetBoneContainer(&BoneContainer);
            rt::FBlendedCurve Curve;
            rt::FAnimationPoseData PoseData(Pose, Curve);
            Sequence->GetAnimationPose(PoseData, rt::FAnimExtractContext(std::min(Time, static_cast<double>(Sequence->GetPlayLength()))));
            std::printf("  %.3f", Time);
            for (int Bone : Chain) {
                const rt::FTransform& T = Pose[rt::FCompactPoseBoneIndex(Bone)];
                std::printf("  %s t(%.1f %.1f %.1f) q(%.3f %.3f %.3f %.3f)", RefSkeleton.GetBoneName(Bone).toString().c_str(), T.GetTranslation().X,
                            T.GetTranslation().Y, T.GetTranslation().Z, T.GetRotation().X, T.GetRotation().Y, T.GetRotation().Z, T.GetRotation().W);
            }
            std::printf("\n");
        }
    } else if (Command == "census") {
        // Every anim class, montage, sequence and composite of the IoStore containers, counted by feature.
        AnimCensus Census;
        const FileSystem& Files = Provider.getFiles();
        const FName Kinds[] = {FName("AnimBlueprintGeneratedClass"), FName("AnimMontage"), FName("AnimSequence"), FName("AnimComposite"),
                               FName("AnimStreamable")};
        for (std::size_t Archive = 0; Archive < Files.getArchiveCount(); Archive++) {
            const IoStoreContainer* Container = Files.getArchive(Archive).ioStore.get();
            if (Container == nullptr) continue;
            const IoStoreToc& Toc = Container->getToc();
            for (std::size_t i = 0; i < Toc.getChunkCount(); i++) {
                const FIoChunkId& Id = Toc.getChunkId(i);
                if (Id.getType() != EIoChunkType::ExportBundleData || Id.getIndex() != 0) continue;
                Result<const Package*> Loaded = Provider.loadPackage(FPackageId{Id.getId()});
                if (!Loaded) continue;
                const Package& Pkg = **Loaded;
                for (std::size_t Export = 0; Export < Pkg.getExportCount(); Export++) {
                    const FName ClassName = Pkg.resolve(Pkg.getExportMapEntry(Export).classIndex).getName();
                    if (std::find(std::begin(Kinds), std::end(Kinds), ClassName) == std::end(Kinds)) continue;
                    if (rt::UObject* Object = rt::FLinkerLoad::FindOrCreate(Pkg)->CreateExport(static_cast<rt::int32>(Export))) Census.AddObject(Object);
                }
            }
        }
        for (const auto& [Key, Count] : Census.Counts) std::printf("%7d %s\n", Count, Key.c_str());
    } else if (Command == "scan" && argc >= 5) {
        // Every package of the IoStore containers; each export of the class (by the name of its class, without loading
        // the others) is created through its package's linker.
        std::vector<FPackageId> Packages;
        const FileSystem& Files = Provider.getFiles();
        for (std::size_t Archive = 0; Archive < Files.getArchiveCount(); Archive++) {
            const IoStoreContainer* Container = Files.getArchive(Archive).ioStore.get();
            if (Container == nullptr) continue;
            const IoStoreToc& Toc = Container->getToc();
            for (std::size_t i = 0; i < Toc.getChunkCount(); i++) {
                const FIoChunkId& Id = Toc.getChunkId(i);
                if (Id.getType() == EIoChunkType::ExportBundleData && Id.getIndex() == 0) Packages.push_back(FPackageId{Id.getId()});
            }
        }
        bPrintProblems = std::getenv("UE5LOAD_PROBLEMS") != nullptr;
        bSampleAnimations = std::getenv("UE5LOAD_SAMPLE") != nullptr;
        const bool bPrintPaths = std::getenv("UE5LOAD_PRINT") != nullptr;
        // Links the bytecode of each loaded class's functions, so that the scan reports the natives the scripts call.
        const bool bLinkScripts = std::getenv("UE5LOAD_LINK_SCRIPTS") != nullptr;
        const FName Class(argv[4]);
        const bool bAll = std::string_view(argv[4]) == "all";
        const long Limit = argc >= 6 ? std::strtol(argv[5], nullptr, 10) : -1;
        long Loaded = 0, Missing = 0;
        const auto Start = std::chrono::steady_clock::now();
        for (const FPackageId& Id : Packages) {
            if (Limit >= 0 && Loaded + Missing >= Limit) break;
            Result<const Package*> Loaded_ = Provider.loadPackage(Id);
            if (!Loaded_) continue;
            const Package& Pkg = **Loaded_;
            for (std::size_t i = 0; i < Pkg.getExportCount(); i++) {
                if (!bAll && Pkg.resolve(Pkg.getExportMapEntry(i).classIndex).getName() != Class) continue;
                rt::UObject* Object = rt::FLinkerLoad::FindOrCreate(Pkg)->CreateExport(static_cast<rt::int32>(i));
                Object ? Loaded++ : Missing++;
                if (bSampleAnimations) SampleOnSkeleton(rt::Cast<rt::UAnimSequence>(Object));
                if (bPrintPaths && Object) std::printf("%s\n", Object->GetPathName().ToUtf8().c_str());
                if (rt::UClass* Loaded = bLinkScripts ? rt::Cast<rt::UClass>(Object) : nullptr)
                    for (rt::UField* Field = Loaded->Children; Field != nullptr; Field = Field->Next)
                        if (rt::UFunction* Function = rt::Cast<rt::UFunction>(Field)) Function->EnsureScriptLinked();
            }
        }
        const double Seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
        std::printf("%s: %ld loaded, %ld not loaded, %.2f s\n", argv[4], Loaded, Missing, Seconds);
        for (const auto& [Code, Count] : Problems) std::printf("  %6d %s  e.g. %s\n", Count, Code.c_str(), ProblemExamples[Code].c_str());
        if (bSampleAnimations) std::printf("sampled %ld sequences, %ld with bad poses\n", Sampled, BadPoses);
    } else {
        std::fprintf(stderr, "unknown command\n");
        return 2;
    }
    return 0;
}
