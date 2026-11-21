#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MetasoundOutputFormatInterfaces.generated.h"

namespace ue5parse::runtime {

//! EMetasoundSourceAudioFormat until 5.3, which had only Mono and Stereo.
UENUM()
enum class EMetaSoundOutputAudioFormat : uint8 { Mono, Stereo, Quad, FiveDotOne, SevenDotOne, ChannelAgnostic, COUNT };

}
