#include <string>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundCue.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundNodeWavePlayer.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"

namespace test::assets {

namespace {

void checkDuration(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "Duration", value);
}

void checkChannels(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "NumChannels", value);
}

void checkSampleRate(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "SampleRate", value);
}

void checkTotalSamples(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "TotalSamples", value);
}

//! Each wave player that holds a hard reference must name the same wave as its soft reference.
int checkCueNode(const rt::USoundNode* node, const std::string& cuePath) {
    if (node == nullptr)
        return 0;
    int hardReferences = 0;
    if (const auto* player = rt::Cast<rt::USoundNodeWavePlayer>(node); player != nullptr && player->SoundWave != nullptr) {
        checkText(getPathName(player->SoundWave), player->SoundWaveAssetPtr.ToString().ToUtf8(), describe(cuePath, node->GetName().ToUtf8()));
        hardReferences++;
    }
    for (const rt::TObjectPtr<rt::USoundNode>& child : node->ChildNodes)
        hardReferences += checkCueNode(child, cuePath);
    return hardReferences;
}

void checkCueNodes(const Cook& cook) {
    const Json* assets = cook.expected.find("assets");
    if (assets == nullptr) {
        fail(cook.version, "the reference values have no assets");
        return;
    }
    withRuntime(cook, [&](const ue5parse::FileProvider&) {
        int hardReferences = 0;
        for (const auto& [path, assetClass] : assets->getObject())
            if (assetClass.getString() == "SoundCue")
                if (const rt::USoundCue* cue = loadObject<rt::USoundCue>(path))
                    hardReferences += checkCueNode(cue->FirstNode, path);
        CHECK((hardReferences > 0) == (cook.game >= ue5parse::EGame::GAME_UE4_16));
    });
}

}

void addSoundChecks(Checks& checks) {
    checks.cook.emplace("sound.cue_nodes", checkCueNodes);
    checks.values.emplace("sound.duration", checkDuration);
    checks.values.emplace("sound.num_channels", checkChannels);
    checks.values.emplace("sound.sample_rate", checkSampleRate);
    checks.values.emplace("sound.total_samples", checkTotalSamples);
}

}
