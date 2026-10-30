// ue5parse_shootergame_check <Paks folder> <file.usmap>: loads VALORANT's wall penetration Blueprints through the
// runtime and checks that their default objects are built on the C++ class of tests/ShooterGame, a module outside the
// library that registers itself (UE5_AES_KEY holds the key).

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>

#include "ShooterGame/AresWallPenetration.h"
#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/CoreUObject/UObject/UObjectGlobals.h"

using namespace ue5parse;
namespace rt = ue5parse::runtime;

namespace {

int Failures = 0;

void Check(bool bCondition, const char* What) {
    std::printf("%s  %s\n", bCondition ? "ok  " : "FAIL", What);
    if (!bCondition) Failures++;
}

// The C++ member a reflected property is bound to: the property is at the member's offset.
template <class TMember>
void CheckBound(const rt::UClass* Class, const char* Name, std::size_t MemberOffset) {
    const rt::FProperty* Property = rt::FindFProperty<rt::FProperty>(Class, FName(Name));
    char What[256];
    std::snprintf(What, sizeof(What), "%s is the C++ member (property offset %d, member offset %zu)", Name,
                  Property ? Property->GetOffset_ForInternal() : -1, MemberOffset);
    Check(Property != nullptr && static_cast<std::size_t>(Property->GetOffset_ForInternal()) == MemberOffset && Property->GetSize() == sizeof(TMember), What);
}

}

int main(int argc, char** argv) {
    if (argc < 3) {
        std::fprintf(stderr, "usage: ue5parse_shootergame_check <Paks folder> <file.usmap>\n");
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
        if (!Parsed) return 1;
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

    using shootergame::UAresWallPenetration;
    rt::UClass* Native = UAresWallPenetration::StaticClass();
    Check(Native != nullptr && Native->GetFName() == FName("AresWallPenetration"), "AresWallPenetration is built from the mappings");
    if (Native == nullptr) return 1;
    Check(Native->GetOuter() != nullptr && Native->GetOuter()->GetFName() == FName("/Script/ShooterGame"), "its package is /Script/ShooterGame");
    CheckBound<float>(Native, "EnergyReductionMultiplier", offsetof(UAresWallPenetration, EnergyReductionMultiplier));
    CheckBound<bool>(Native, "bParticipatesInAirDropoff", offsetof(UAresWallPenetration, bParticipatesInAirDropoff));
    CheckBound<bool>(Native, "bShowsRicochet", offsetof(UAresWallPenetration, bShowsRicochet));
    CheckBound<rt::FString>(Native, "DebugWallPenetrationTypeName", offsetof(UAresWallPenetration, DebugWallPenetrationTypeName));
    Check(Native->FindFunctionByName(FName("GetEnergyAfter")) != nullptr, "GetEnergyAfter is a native function of the class");
    rt::UFunction* ScaleEnergy = Native->FindFunctionByName(FName("ScaleEnergy"));
    Check(ScaleEnergy != nullptr && ScaleEnergy->NumParms == 3 && ScaleEnergy->ReturnValueOffset != 0xffff,
          "ScaleEnergy has its parameters and return value as properties");

    const char* const Blueprints[] = {
        "/Game/Equippables/Guns/_Core/WallPenetration/WallPen_High.WallPen_High_C",
        "/Game/Equippables/Guns/_Core/WallPenetration/WallPen_Moderate.WallPen_Moderate_C",
        "/Game/Equippables/Guns/_Core/WallPenetration/WallPen_Low.WallPen_Low_C",
        "/Game/Equippables/Guns/_Core/WallPenetration/WallPen_VeryLow.WallPen_VeryLow_C",
    };
    for (const char* Path : Blueprints) {
        rt::UClass* Class = rt::StaticLoadClass(Native, nullptr, rt::FString(Path));
        char What[512];
        std::snprintf(What, sizeof(What), "%s loads as a subclass of AresWallPenetration", Path);
        Check(Class != nullptr && Class->IsChildOf(Native), What);
        if (Class == nullptr) continue;
        UAresWallPenetration* Defaults = rt::Cast<UAresWallPenetration>(Class->GetDefaultObject());
        Check(Defaults != nullptr, "  its default object is the C++ class");
        if (Defaults == nullptr) continue;
        std::printf("        EnergyReductionMultiplier %g, bParticipatesInAirDropoff %d, bShowsRicochet %d, DebugWallPenetrationTypeName \"%s\"\n",
                    Defaults->EnergyReductionMultiplier, Defaults->bParticipatesInAirDropoff, Defaults->bShowsRicochet,
                    Defaults->DebugWallPenetrationTypeName.ToUtf8().c_str());
        // A native event through ProcessEvent: the thunk reads the parameters from the frame, runs _Implementation,
        // and gives back the out parameter and the return value. An event no Blueprint implements does nothing.
        rt::FName Reason;
        float Scaled = Defaults->ScaleEnergy(100.0f, Reason);
        Check(Scaled == 100.0f * Defaults->EnergyReductionMultiplier && Reason == rt::FName("Native"), "  ScaleEnergy runs its native body through ProcessEvent");
        Defaults->ReceivePenetrated(1.0f, rt::FVector(1, 2, 3));
    }
    // In the same folder, but a CustomWallPenetrationResponseComponent: not a class of this base.
    Check(rt::StaticLoadClass(Native, nullptr, rt::FString("/Game/Equippables/Guns/_Core/WallPenetration/"
                                                           "WallPenetrationNonParticipant.WallPenetrationNonParticipant_C")) == nullptr,
          "WallPenetrationNonParticipant (a component Blueprint) does not load as AresWallPenetration");
    std::printf("%d failed\n", Failures);
    return Failures == 0 ? 0 : 1;
}
