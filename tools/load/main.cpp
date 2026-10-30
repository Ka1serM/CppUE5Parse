// ue5load <Paks folder> <file.usmap> object <object path>...
// ue5load <Paks folder> <file.usmap> scan <class>|all [<limit>]
//
// Loads objects through the runtime on real game data (UE5_AES_KEY holds the key):
//   object   loads each object and prints its class, its path and a summary of its native data
//   scan     loads every export of a class (or all) and counts the load problems by code; a limit stops early

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
#include "ue5parse/runtime/Engine/Animation/AnimMontage.h"
#include "ue5parse/runtime/Engine/Animation/AnimBlueprintGeneratedClass.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstance.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"
#include "ue5parse/runtime/Engine/Animation/BlendSpace.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Engine/SkeletalMesh.h"
#include "ue5parse/runtime/Engine/Engine/StaticMesh.h"
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
        std::printf("  graph nodes %d node data %d machines %d root %s target %s\n", AnimClass->GetAnimNodeProperties().Num(),
                    AnimClass->AnimNodeData.Num(), AnimClass->BakedStateMachines.Num(),
                    AnimClass->GetRootAnimClass()->GetPathName().ToUtf8().c_str(),
                    rt::GetPathNameSafe(AnimClass->TargetSkeleton).ToUtf8().c_str());
        if (std::getenv("UE5LOAD_NODES"))
            for (const auto& Machine : AnimClass->BakedStateMachines)
                std::printf("  machine %s: %d states, %d transitions\n", Machine.MachineName.toString().c_str(), Machine.States.Num(),
                            Machine.Transitions.Num());
        if (std::getenv("UE5LOAD_NODES"))
            for (const rt::FStructProperty* Property : AnimClass->GetAnimNodeProperties())
                std::printf("  node %s: %s (%d folded fields)\n", Property->GetName().ToUtf8().c_str(),
                            Property->Struct->GetName().ToUtf8().c_str(), AnimClass->GetAnimNodePropertyCount(Property->Struct));
        if (std::getenv("UE5LOAD_NODES")) {
            const auto Nodes = AnimClass->GetAnimNodeProperties();
            rt::int32 HandlerCount = 0, CopyCount = 0, FunctionCount = 0;
            for (rt::int32 Index = 0; Index < Nodes.Num(); ++Index)
                if (const auto* Handler = AnimClass->GetExposedValueHandler(Index)) {
                    ++HandlerCount;
                    CopyCount += Handler->CopyRecords.Num();
                    if (!Handler->BoundFunction.isNone()) ++FunctionCount;
                }
            std::printf("  exposed handlers %d copy records %d bound functions %d\n", HandlerCount, CopyCount, FunctionCount);
            if (const auto* Library = AnimClass->GetPropertyAccessLibrary()) {
                rt::int32 Copies = 0;
                for (const auto& Batch : Library->CopyBatchArray) Copies += Batch.Copies.Num();
                std::printf("  property access paths %d source %d destination %d batches %d copies %d\n",
                    Library->PathSegments.Num(), Library->SrcPaths.Num(), Library->DestPaths.Num(),
                    Library->CopyBatchArray.Num(), Copies);
                for (rt::int32 Index = 0; Index < std::min<rt::int32>(Library->PathSegments.Num(), 5); ++Index) {
                    const auto& Segment = Library->PathSegments[Index];
                    std::printf("  path segment %d %s[%d] flags %u\n", Index, Segment.Name.toString().c_str(),
                        Segment.ArrayIndex, Segment.Flags);
                }
                if (Library->CopyBatchArray.Num() && Library->CopyBatchArray[0].Copies.Num()) {
                    const auto& Copy = Library->CopyBatchArray[0].Copies[0];
                    std::printf("  property copy source %d destinations %d..%d type %u\n", Copy.AccessIndex,
                        Copy.DestAccessStartIndex, Copy.DestAccessEndIndex, Copy.Type);
                    const rt::FProperty* SourceProperty = nullptr;
                    void* Source = nullptr;
                    const rt::FProperty* DestinationProperty = nullptr;
                    void* Destination = nullptr;
                    auto* DefaultObject = const_cast<rt::UAnimBlueprintGeneratedClass*>(AnimClass)->GetDefaultObject();
                    const bool SourceFound = Library->SrcPaths.IsValidIndex(Copy.AccessIndex) &&
                        Library->ResolvePath(DefaultObject, Library->SrcPaths[Copy.AccessIndex], SourceProperty, Source);
                    const bool DestinationFound = Library->DestPaths.IsValidIndex(Copy.DestAccessStartIndex) &&
                        Library->ResolvePath(DefaultObject, Library->DestPaths[Copy.DestAccessStartIndex], DestinationProperty, Destination);
                    std::printf("  first property copy resolves source %s destination %s\n",
                        SourceFound ? SourceProperty->GetName().ToUtf8().c_str() : "unavailable",
                        DestinationFound ? DestinationProperty->GetName().ToUtf8().c_str() : "unavailable");
                    auto* SampleInstance = rt::NewObject<rt::UAnimInstance>(nullptr, AnimClass);
                    const rt::FProperty* SampleSourceProperty = nullptr;
                    void* SampleSource = nullptr;
                    const rt::FProperty* SampleDestinationProperty = nullptr;
                    void* SampleDestination = nullptr;
                    const bool SamplePaths = Library->ResolvePath(SampleInstance, Library->SrcPaths[Copy.AccessIndex],
                            SampleSourceProperty, SampleSource) &&
                        Library->ResolvePath(SampleInstance, Library->DestPaths[Copy.DestAccessStartIndex],
                            SampleDestinationProperty, SampleDestination);
                    if (SamplePaths) {
                        const auto* SourceNumber = rt::CastField<rt::FNumericProperty>(SampleSourceProperty);
                        const auto* DestinationNumber = rt::CastField<rt::FNumericProperty>(SampleDestinationProperty);
                        if (SourceNumber && DestinationNumber) {
                            SourceNumber->SetFloatingPointPropertyValue(SampleSource, 0.375);
                            const bool Copied = Library->ProcessCopy(SampleInstance, 0);
                            std::printf("  first property copy executed %d value %.3f\n", Copied,
                                DestinationNumber->GetFloatingPointPropertyValue(SampleDestination));
                        }
                    }
                    rt::int32 ExecutedCopies = 0;
                    for (rt::int32 Index = 0; Index < Library->CopyBatchArray[0].Copies.Num(); ++Index)
                        if (Library->ProcessCopy(SampleInstance, Index)) ++ExecutedCopies;
                    std::printf("  property copies executed %d of %d\n", ExecutedCopies,
                        Library->CopyBatchArray[0].Copies.Num());
                }
            }
            for (rt::int32 Index = 0; Index < Nodes.Num(); ++Index)
                if (const auto* Handler = AnimClass->GetExposedValueHandler(Index); Handler && Handler->CopyRecords.Num()) {
                    std::printf("  first copy record node %d index %d operation %d\n", Index,
                        Handler->CopyRecords[0].CopyIndex, static_cast<int>(Handler->CopyRecords[0].PostCopyOperation.GetValue()));
                    break;
                }
            if (const auto* Library = AnimClass->GetPropertyAccessLibrary())
                for (rt::int32 Index = 0; Index < Nodes.Num(); ++Index) {
                    const auto* Handler = AnimClass->GetExposedValueHandler(Index);
                    if (!Handler || !Handler->CopyRecords.Num() || !Handler->BoundFunction.isNone()) continue;
                    auto* SampleInstance = rt::NewObject<rt::UAnimInstance>(nullptr, AnimClass);
                    std::printf("  exposed handler node %d executed %d\n", Index, Handler->Execute(SampleInstance, *Library));
                    break;
                }
            if (Nodes.Num() > 0)
                std::printf("  first node links Base %d Additive %d\n", AnimClass->GetAnimNodeLinkID(0, rt::FName("Base")),
                    AnimClass->GetAnimNodeLinkID(0, rt::FName("Additive")));
            for (rt::int32 Index = 0; Index < Nodes.Num() && Index < AnimClass->AnimNodeData.Num(); ++Index) {
                if (Nodes[Index]->Struct->GetFName() == rt::FName("AnimNode_BlendListByBool")) {
                    const auto Links = AnimClass->GetAnimNodeLinkIDs(Index, rt::FName("BlendPose"));
                    std::printf("  first blend list has %d pose links\n", Links.Num());
                }
                if (Nodes[Index]->Struct->GetFName() != rt::FName("AnimNode_SequencePlayer")) continue;
                const rt::FProperty* ValueProperty = nullptr;
                const void* Value = AnimClass->GetAnimNodeValue(AnimClass->AnimNodeData[Index], Nodes[Index]->Struct,
                    rt::FName("PlayRate"), nullptr, ValueProperty);
                if (const auto* Number = rt::CastField<rt::FNumericProperty>(ValueProperty); Number && Value)
                    std::printf("  first sequence player folded PlayRate %.3f\n", Number->GetFloatingPointPropertyValue(Value));
                const rt::FProperty* SequenceProperty = nullptr;
                const void* SequenceValue = AnimClass->GetAnimNodeFieldValue(Index, rt::FName("Sequence"), nullptr, SequenceProperty);
                if (const auto* ObjectProperty = rt::CastField<rt::FObjectPropertyBase>(SequenceProperty); ObjectProperty && SequenceValue)
                    std::printf("  first sequence player Sequence %s\n",
                        rt::GetPathNameSafe(ObjectProperty->GetObjectPropertyValue(SequenceValue)).ToUtf8().c_str());
                break;
            }
        }
    } else if (const rt::USkeletalMesh* Mesh = rt::Cast<rt::USkeletalMesh>(Object)) {
        std::printf("  bones %d materials %d skeleton %s\n", Mesh->GetRefSkeleton().GetRawBoneNum(), Mesh->GetMaterials().Num(),
                    rt::GetPathNameSafe(Mesh->GetSkeleton()).ToUtf8().c_str());
        if (const rt::FSkeletalMeshRenderData* Data = Mesh->GetResourceForRendering()) {
            for (rt::int32 i = 0; i < Data->LODRenderData.Num(); i++) {
                const rt::FSkeletalMeshLODRenderData& Lod = Data->LODRenderData[i];
                std::printf("  lod %d: sections %d vertices %u\n", i, Lod.RenderSections.Num(), Lod.GetNumVertices());
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

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: ue5load <Paks folder> <file.usmap> object <path>... | scan <class> [<limit>]\n");
        return 2;
    }
    MountOptions Options;
    std::error_code Error;
    for (const auto& Entry : std::filesystem::directory_iterator(argv[1], Error)) {
        if (!Entry.is_regular_file()) continue;
        Result<std::unique_ptr<FileSource>> File = openLocalFile(Entry.path());
        if (File) Options.archives.push_back(std::move(*File));
    }
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
    Options.engine = EngineVersion::UE5_3;
    Options.quirks = valorantQuirks;
    Result<MountedGame> Game = mount(std::move(Options));
    if (!Game) {
        std::fprintf(stderr, "mount failed: %s\n", getErrorName(Game.error().code));
        return 1;
    }
    FileProvider Provider(std::move(*Game));
    rt::InitializeModule_CoreUObject();
    rt::InitializeRuntime(Provider);
    rt::SetLoadProblemHandler(&OnProblem);

    std::string_view Command = argv[3];
    if (Command == "object") {
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
    rt::ShutdownRuntime();
    return 0;
}
