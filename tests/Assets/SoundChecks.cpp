#include "AssetSuite.h"

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

}

void addSoundChecks(Checks& checks) {
    checks.values.emplace("sound.duration", checkDuration);
    checks.values.emplace("sound.num_channels", checkChannels);
    checks.values.emplace("sound.sample_rate", checkSampleRate);
    checks.values.emplace("sound.total_samples", checkTotalSamples);
}

}
