#include "Engine/Source/Runtime/Core/Hash/ShaderHash.h"

#include <format>

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FShaderCodeHash& Value) {
    if (Ar.GetGame() >= EGame::GAME_UE5_8)
        return Ar << std::get<FShaderHash>(Value = FShaderHash()).Hash;
    return Ar << std::get<FSHAHash>(Value = FSHAHash());
}

std::string ToString(const FShaderCodeHash& Value) {
    if (const FShaderHash* Hash = std::get_if<FShaderHash>(&Value))
        return std::format("{:016X}", Hash->Hash);
    return std::get<FSHAHash>(Value).ToString();
}

std::size_t FShaderCodeHashHasher::operator()(const FShaderCodeHash& Value) const {
    if (const FShaderHash* Hash = std::get_if<FShaderHash>(&Value))
        return static_cast<std::size_t>(Hash->Hash);
    return FSHAHashHasher()(std::get<FSHAHash>(Value));
}

}
