#include "CppUE5Parse/VFS/Pak.h"

#include <array>
#include <algorithm>
#include <cstring>
#include <optional>
#include <string>

#include "CppUE5Parse/Core/Check.h"
#include "CppUE5Parse/Crypto/Blake3.h"
#include "CppUE5Parse/GameTypes/ArenaBreakoutInfinite/ArenaBreakoutInfiniteAes.h"
#include "CppUE5Parse/GameTypes/InZOI/InZOIPakInfo.h"
#include "CppUE5Parse/GameTypes/Century/CenturyDecrypt.h"
#include "CppUE5Parse/GameTypes/GameForPeace/GameForPeaceIni.h"
#include "CppUE5Parse/GameTypes/NevernessToEverness/NevernessToEvernessIni.h"
#include "CppUE5Parse/GameTypes/ProSpi/ProSpiDecrypt.h"
#include "CppUE5Parse/GameTypes/Snowbreak/SnowbreakLua.h"
#include "CppUE5Parse/GameTypes/Strinova/StrinovaLua.h"
#include "CppUE5Parse/VFS/BlockReader.h"

namespace ue5parse {

namespace {

constexpr std::uint8_t flagEncrypted = 0x01;
constexpr std::uint64_t entryHeaderSize = 8 * 3 + 4 + 20;
constexpr std::uint64_t entryFlagsSize = 1 + 4;
constexpr std::uint64_t entryTimestampSize = 8;
constexpr std::uint64_t maxOffset = 1ull << 62;

constexpr std::uint8_t legacyZlib = 0x01;
constexpr std::uint8_t legacyGzip = 0x02;
constexpr std::uint8_t legacyCustom = 0x04;
//! Before pak version 8, an entry stores compression flags; UE4 maps zlib, gzip and custom to methods 1 to 3.
constexpr std::string_view legacyMethods[] = {"Zlib", "Gzip", "Oodle"};

struct FooterLayout {
    std::size_t size;
    EPakFileVersion first;
    EPakFileVersion last;
    std::size_t methodCount;
    bool byteMethodIndex = false;
};

//! 4.22 wrote pak version 8 with four method names and a uint8 method index per entry; 4.23 and 4.24 kept version 8 with five names and a uint32.
constexpr FooterLayout footerLayouts[] = {
    {FPakInfo::serializedSize + 1, EPakFileVersion::FrozenIndex, EPakFileVersion::FrozenIndex, FPakInfo::maxMethods},
    {FPakInfo::serializedSize, EPakFileVersion::FNameBasedCompressionMethod, EPakFileVersion::FNameBasedCompressionMethod, FPakInfo::maxMethods},
    {FPakInfo::serializedSize, EPakFileVersion::PathHashIndex, EPakFileVersion::Utf8PakDirectory, FPakInfo::maxMethods},
    {FPakInfo::serializedSize - FPakInfo::methodNameLength, EPakFileVersion::FNameBasedCompressionMethod, EPakFileVersion::FNameBasedCompressionMethod,
     FPakInfo::maxMethods - 1, true},
    {61, EPakFileVersion::EncryptionKeyGuid, EPakFileVersion::EncryptionKeyGuid, 0},
    {45, EPakFileVersion::Initial, EPakFileVersion::DeleteRecords, 0},
};

//! The custom data value of a file that the game encrypts again inside the pak data.
constexpr std::uint8_t transformedFile = 1;

constexpr bool hasFileTransform(EGame game) {
    return game == EGame::GAME_Snowbreak || game == EGame::GAME_Strinova || game == EGame::GAME_NevernessToEverness || game == EGame::GAME_GameForPeace;
}

//! A custom game is any EGame value that is not a plain engine version.
constexpr bool isCustomGame(EGame game) {
    return (static_cast<std::uint32_t>(game) & 0xFFFF) != 0;
}

//! CUE4Parse's pak footer sizes; each one names how many method names follow the fixed fields.
constexpr std::size_t sizeGameForPeace = 45;
constexpr std::size_t sizeBase = 61;
constexpr std::size_t sizeFridayThe13th = sizeBase + 4;
constexpr std::size_t size8v1 = sizeBase + 32;
constexpr std::size_t size8v2 = size8v1 + 32;
constexpr std::size_t size8v3 = size8v2 + 32;
constexpr std::size_t size8 = size8v3 + 32;
constexpr std::size_t sizeRacingMaster = size8 + 4;
constexpr std::size_t size8a = size8 + 32;
constexpr std::size_t size9 = size8a + 1;
constexpr std::size_t sizeBlackMythWukong = size9 + 1;
constexpr std::size_t sizeHotta = size8a + 4;
constexpr std::size_t sizeArkSurvivalAscended = size8a + 8;
constexpr std::size_t sizeFarlight = size8a + 9;
constexpr std::size_t sizeDreamStar = size8a + 10;
constexpr std::size_t sizeRennsport = size8a + 16;
constexpr std::size_t sizeQQ = size8a + 26;
constexpr std::size_t sizeDeadByDaylight = size8a + 32;
constexpr std::size_t sizeDuneAwakening = 261;
constexpr std::size_t sizeKartRiderDrift = 397;

constexpr std::size_t maxGameFooterSize = sizeKartRiderDrift;

constexpr std::size_t defaultFooterSizes[] = {size8a, size8, sizeBase, size9, size8v1, size8v2, size8v3};
constexpr std::size_t hottaFooterSizes[] = {sizeHotta};
constexpr std::size_t fridayThe13thFooterSizes[] = {sizeFridayThe13th};
constexpr std::size_t deadByDaylightFooterSizes[] = {sizeDeadByDaylight};
constexpr std::size_t farlightFooterSizes[] = {sizeFarlight};
constexpr std::size_t dreamStarFooterSizes[] = {sizeDreamStar, sizeQQ};
constexpr std::size_t gameForPeaceFooterSizes[] = {sizeGameForPeace};
constexpr std::size_t blackMythWukongFooterSizes[] = {sizeBlackMythWukong};
constexpr std::size_t rennsportFooterSizes[] = {sizeRennsport};
constexpr std::size_t racingMasterFooterSizes[] = {sizeRacingMaster};
constexpr std::size_t arkSurvivalAscendedFooterSizes[] = {sizeArkSurvivalAscended};
constexpr std::size_t kartRiderDriftFooterSizes[] = {size8a, size8, sizeBase, size9, size8v1, size8v2, size8v3, sizeKartRiderDrift};
constexpr std::size_t duneAwakeningFooterSizes[] = {sizeDuneAwakening};

std::span<const std::size_t> getGameFooterSizes(EGame game) {
    switch (game) {
    case EGame::GAME_TowerOfFantasy:
    case EGame::GAME_MeetYourMaker:
    case EGame::GAME_TorchlightInfinite:
    case EGame::GAME_EtheriaRestart:
        return hottaFooterSizes;
    case EGame::GAME_FridayThe13th:
        return fridayThe13thFooterSizes;
    case EGame::GAME_DeadByDaylight:
    case EGame::GAME_DeadByDaylight_Old:
        return deadByDaylightFooterSizes;
    case EGame::GAME_Farlight84:
        return farlightFooterSizes;
    case EGame::GAME_QQ:
    case EGame::GAME_DreamStar:
        return dreamStarFooterSizes;
    case EGame::GAME_GameForPeace:
    case EGame::GAME_DragonQuestXI:
        return gameForPeaceFooterSizes;
    case EGame::GAME_BlackMythWukong:
        return blackMythWukongFooterSizes;
    case EGame::GAME_Rennsport:
        return rennsportFooterSizes;
    case EGame::GAME_RacingMaster:
        return racingMasterFooterSizes;
    case EGame::GAME_ARKSurvivalAscended:
    case EGame::GAME_PromiseMascotAgency:
        return arkSurvivalAscendedFooterSizes;
    case EGame::GAME_KartRiderDrift:
        return kartRiderDriftFooterSizes;
    case EGame::GAME_DuneAwakening:
        return duneAwakeningFooterSizes;
    default:
        return defaultFooterSizes;
    }
}

std::size_t getGameFooterMethodCount(std::size_t size) {
    switch (size) {
    case size8a:
    case sizeHotta:
    case sizeDeadByDaylight:
    case sizeRennsport:
        return 5;
    case size8v1:
        return 1;
    case size8v2:
        return 2;
    case size8v3:
        return 3;
    default:
        return 4;
    }
}

constexpr std::uint32_t magicOutlastTrials = 0xA590ED1E;
constexpr std::uint32_t magicTorchlightInfinite = 0x6B2A56B8;
constexpr std::uint32_t magicWildAssault = 0xA4CCD123;
constexpr std::uint32_t magicUndawn = 0x5A6F12EC;
constexpr std::uint32_t magicFridayThe13th = 0x65617441;
constexpr std::uint32_t magicDreamStar = 0x1B6A32F1;
constexpr std::uint32_t magicGameForPeace = 0xff67ff70;
constexpr std::uint32_t magicKartRiderDrift = 0x81c4b35b;
constexpr std::uint32_t magicRacingMaster = 0x9a51da3f;
constexpr std::uint32_t magicCrystalOfAtlan = 0x22ce976a;
constexpr std::uint32_t magicPromiseMascotAgency = 0x11adde11;
constexpr std::uint32_t magicArenaBreakoutInfinite = 0x53647586;
constexpr std::uint32_t magicAssaultFireFuture = 0x4F6FAE86;
constexpr std::uint32_t magicDuneAwakening = 0xA590ED1E;

//! The magic a game writes in place of the engine one, or the engine magic.
std::uint32_t getGamePakMagic(EGame game) {
    switch (game) {
    case EGame::GAME_OutlastTrials:
        return magicOutlastTrials;
    case EGame::GAME_TorchlightInfinite:
    case EGame::GAME_EtheriaRestart:
        return magicTorchlightInfinite;
    case EGame::GAME_WildAssault:
        return magicWildAssault;
    case EGame::GAME_Undawn:
        return magicUndawn;
    case EGame::GAME_FridayThe13th:
        return magicFridayThe13th;
    case EGame::GAME_DreamStar:
        return magicDreamStar;
    case EGame::GAME_AssaultFireFuture:
        return magicAssaultFireFuture;
    case EGame::GAME_KartRiderDrift:
        return magicKartRiderDrift;
    default:
        return FPakInfo::magic;
    }
}

//! The engine methods before pak version 8 as CUE4Parse lists them, with LZ4 and Zstd for the games that map flags to them.
constexpr std::string_view gameLegacyMethods[] = {"Zlib", "Gzip", "Oodle", "LZ4", "Zstd"};

struct GameFooterFields {
    std::int32_t version = 0;
    std::int64_t indexOffset = 0;
    std::int64_t indexSize = 0;
    bool frozenIndex = false;
    bool fixedMethods = false;
    std::uint32_t hottaVersion = 0;
};

//! Reads one footer candidate the way CUE4Parse's FPakInfo constructor does.
std::optional<FPakInfo> readGameFooter(std::span<const std::byte> footer, EGame game, std::size_t size) {
    ByteReader reader{footer};
    FPakInfo info;
    info.game = game;
    info.footerSize = static_cast<std::uint32_t>(size);
    GameFooterFields fields;
    std::uint32_t magic = 0;
    if (game == EGame::GAME_TowerOfFantasy && size == sizeHotta) {
        fields.hottaVersion = reader.read<std::uint32_t>();
        if (fields.hottaVersion > 255)
            fields.hottaVersion = 0;
    }
    if (game == EGame::GAME_TorchlightInfinite || game == EGame::GAME_EtheriaRestart)
        reader.skip(3);
    switch (game) {
    case EGame::GAME_GameForPeace:
        info.encryptedIndex = reader.read<std::uint8_t>() != 0x6c;
        if (reader.read<std::uint32_t>() != magicGameForPeace)
            return std::nullopt;
        fields.version = reader.read<std::int32_t>();
        if (fields.version >= static_cast<std::int32_t>(EPakFileVersion::PathHashIndex))
            fields.version = static_cast<std::int32_t>(EPakFileVersion::FNameBasedCompressionMethod);
        reader.skip(20);
        fields.indexSize = static_cast<std::int64_t>(reader.read<std::uint64_t>() ^ 0x8924b0e3298b7069);
        fields.indexOffset = static_cast<std::int64_t>(reader.read<std::uint64_t>() ^ 0xd74af37faa6b020d);
        fields.fixedMethods = true;
        break;
    case EGame::GAME_ArenaBreakoutInfinite:
        info.encryptionKeyGuid = reader.read<FGuid>();
        if (reader.read<std::uint32_t>() != magicArenaBreakoutInfinite)
            return std::nullopt;
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        fields.indexSize = reader.read<std::int64_t>();
        fields.indexOffset = reader.read<std::int64_t>();
        reader.skip(20);
        fields.version = reader.read<std::int32_t>();
        break;
    case EGame::GAME_DragonQuestXI:
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        if (reader.read<std::uint32_t>() != FPakInfo::magic)
            return std::nullopt;
        fields.version = reader.read<std::int32_t>();
        fields.indexOffset = reader.read<std::int64_t>();
        fields.indexSize = reader.read<std::int64_t>();
        reader.skip(20);
        break;
    case EGame::GAME_RacingMaster: {
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        info.encryptionKeyGuid = reader.read<FGuid>();
        std::span<const std::byte> custom = reader.readBytes(4);
        if (!reader.hasFailed()) {
            std::memcpy(info.customEncryptionData.data(), custom.data(), custom.size());
            info.customEncryptionDataSize = 4;
        }
        if (reader.read<std::uint32_t>() != magicRacingMaster)
            return std::nullopt;
        fields.indexSize = reader.read<std::int64_t>();
        reader.skip(20);
        fields.version = reader.read<std::int32_t>();
        fields.indexOffset = reader.read<std::int64_t>();
        break;
    }
    case EGame::GAME_PromiseMascotAgency:
        info.encryptionKeyGuid = reader.read<FGuid>();
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        if (reader.read<std::uint32_t>() != magicPromiseMascotAgency)
            return std::nullopt;
        reader.skip(20);
        fields.version = 11 + (reader.read<std::int32_t>() ^ 0x0A4FFC11);
        reader.skip(8);
        fields.indexSize = reader.read<std::int64_t>() ^ 0x0BBEFB6F91D3B57B;
        fields.indexOffset = reader.read<std::int64_t>();
        break;
    case EGame::GAME_CrystalOfAtlan:
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        fields.version = reader.read<std::int32_t>();
        fields.indexSize = reader.read<std::int64_t>();
        reader.skip(20);
        fields.indexOffset = reader.read<std::int64_t>();
        if (reader.read<std::uint32_t>() != magicCrystalOfAtlan)
            return std::nullopt;
        info.encryptionKeyGuid = reader.read<FGuid>();
        break;
    case EGame::GAME_DuneAwakening:
        if (reader.read<std::uint32_t>() != magicDuneAwakening)
            return std::nullopt;
        fields.indexOffset = reader.read<std::int64_t>();
        fields.indexSize = reader.read<std::int64_t>();
        reader.skip(20);
        info.encryptionKeyGuid = reader.read<FGuid>();
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        if (reader.read<std::uint32_t>() != FPakInfo::magic)
            return std::nullopt;
        fields.version = reader.read<std::int32_t>();
        reader.skip(36);
        break;
    default:
        info.encryptionKeyGuid = reader.read<FGuid>();
        info.encryptedIndex = reader.read<std::uint8_t>() != 0;
        magic = reader.read<std::uint32_t>();
        if (magic != FPakInfo::magic && magic != getGamePakMagic(game))
            return std::nullopt;
        fields.version = fields.hottaVersion >= 2 ? reader.read<std::int32_t>() ^ 2 : reader.read<std::int32_t>();
        if (game == EGame::GAME_StateOfDecay2)
            fields.version &= 0xFFFF;
        if (game == EGame::GAME_KartRiderDrift)
            fields.version &= 0x0F;
        //! Friday the 13th writes its own version 5 and up for an engine version 4 footer with an extra magic.
        if (game == EGame::GAME_FridayThe13th && fields.version >= static_cast<std::int32_t>(EPakFileVersion::RelativeChunkOffsets)) {
            fields.version = static_cast<std::int32_t>(EPakFileVersion::IndexEncryption);
            reader.skip(4);
        }
        if (game == EGame::GAME_TorchlightInfinite || game == EGame::GAME_EtheriaRestart)
            reader.skip(1);
        if (game == EGame::GAME_BlackMythWukong)
            reader.skip(2);
        fields.indexOffset = reader.read<std::int64_t>();
        if (game == EGame::GAME_Farlight84)
            reader.skip(8);
        if (game == EGame::GAME_Snowbreak)
            fields.indexOffset ^= 0x1C1D1E1F;
        if (game == EGame::GAME_KartRiderDrift)
            fields.indexOffset ^= 0x3009EB;
        if (game == EGame::GAME_NevernessToEverness || game == EGame::GAME_NevernessToEverness_CBT2)
            fields.indexOffset -= 1;
        fields.indexSize = reader.read<std::int64_t>();
        reader.skip(20);
        if (game == EGame::GAME_DreamStar || game == EGame::GAME_AssaultFireFuture)
            std::swap(fields.indexOffset, fields.indexSize);
        if (game == EGame::GAME_MeetYourMaker && size == sizeHotta && fields.version >= static_cast<std::int32_t>(EPakFileVersion::Fnv64BugFix))
            reader.skip(4);
        if (game == EGame::GAME_WildAssault) {
            info.encryptionKeyGuid = {};
            fields.indexOffset = static_cast<std::int64_t>((static_cast<std::uint64_t>(fields.indexOffset) ^ 0xD5B9B05CE8143A3C) - 0xAA);
            fields.indexSize = static_cast<std::int64_t>((static_cast<std::uint64_t>(fields.indexSize) ^ 0x6DB425B4BC084B4B) - 0xA8);
        }
        if (game == EGame::GAME_DeadByDaylight || game == EGame::GAME_DeadByDaylight_Old) {
            std::span<const std::byte> custom = reader.readBytes(28);
            if (!reader.hasFailed()) {
                std::memcpy(info.customEncryptionData.data(), custom.data(), custom.size());
                info.customEncryptionDataSize = 28;
            }
            reader.skip(4);
        }
        if (fields.version == static_cast<std::int32_t>(EPakFileVersion::FrozenIndex))
            fields.frozenIndex = reader.read<std::uint8_t>() != 0;
        break;
    }

    info.version = static_cast<EPakFileVersion>(fields.version);
    if (fields.fixedMethods || info.version < EPakFileVersion::FNameBasedCompressionMethod) {
        for (std::string_view method : gameLegacyMethods)
            std::memcpy(info.methodNames[info.methodCount++], method.data(), method.size());
    } else {
        std::size_t nameLength = game == EGame::GAME_KartRiderDrift ? 48 : FPakInfo::methodNameLength;
        std::size_t methodCount = getGameFooterMethodCount(size);
        std::span<const std::byte> names = reader.readBytes(nameLength * methodCount);
        if (reader.hasFailed())
            return std::nullopt;
        if (fields.hottaVersion >= 3)
            info.methodIndexBase = 1;
        for (std::size_t i = 0; i < methodCount; i++) {
            const char* name = reinterpret_cast<const char*>(names.data() + i * nameLength);
            if (name[0] == 0)
                continue;
            std::memcpy(info.methodNames[info.methodCount], name, FPakInfo::methodNameLength - 1);
            info.methodNames[info.methodCount][FPakInfo::methodNameLength - 1] = 0;
            info.methodCount++;
        }
    }
    info.byteMethodIndex = info.version == EPakFileVersion::FNameBasedCompressionMethod && size != size8a;
    if (info.version < EPakFileVersion::IndexEncryption)
        info.encryptedIndex = false;
    if (info.version < EPakFileVersion::EncryptionKeyGuid)
        info.encryptionKeyGuid = {};
    if (reader.hasFailed() || fields.frozenIndex)
        return std::nullopt;
    if (fields.indexOffset < 0 || fields.indexSize < 0 || static_cast<std::uint64_t>(fields.indexOffset) >= maxOffset
        || static_cast<std::uint64_t>(fields.indexSize) >= maxOffset)
        return std::nullopt;
    info.indexOffset = static_cast<std::uint64_t>(fields.indexOffset);
    info.indexSize = static_cast<std::uint64_t>(fields.indexSize);
    return info;
}

Result<FPakInfo> parseGamePakInfo(std::span<const std::byte> tail, EGame game) {
    for (std::size_t size : getGameFooterSizes(game)) {
        if (size > tail.size())
            continue;
        std::optional<FPakInfo> info = readGameFooter(tail.last(size), game, size);
        if (info)
            return *info;
    }
    return fail(ErrorCode::BadMagic, "parsePakInfo.magic");
}

std::uint32_t getLegacyMethod(std::uint32_t flags, EGame game) {
    if (flags == 0)
        return 0;
    //! State of Decay 2 writes 259 for LZ4; CUE4Parse maps 259 to LZ4 for every game that it reads.
    if (isCustomGame(game) && flags == 259)
        return 4;
    if (flags & legacyZlib)
        return 1;
    if (flags & legacyGzip)
        return 2;
    //! Sea of Thieves uses the custom flag for LZ4.
    if (flags & legacyCustom)
        return game == EGame::GAME_SeaOfThieves ? 4 : 3;
    if (game == EGame::GAME_PlayerUnknownsBattlegrounds || game == EGame::GAME_Ashen)
        return 3;
    //! Dead Island 2 stores unknown flags for Zstd; CUE4Parse writes 6, the CompressionMethod value of Zstd when the case was added.
    if (game == EGame::GAME_DeadIsland2)
        return 5;
    return FPakInfo::maxMethods + 1;
}

//! Gears of War 4 stores CUE4Parse's CompressionMethod value, and its Oodle is LZ4.
std::uint32_t getGearsOfWar4Method(std::uint8_t method) {
    switch (method) {
    case 0:
        return 0;
    case 1:
        return 1;
    case 2:
        return 2;
    case 4:
    case 5:
        return 4;
    case 7:
        return 5;
    default:
        return FPakInfo::maxMethods + 1;
    }
}

//! Applies Tower of Fantasy's method base; a None name in the footer is the uncompressed method.
std::uint32_t getEntryMethod(std::uint32_t stored, const FPakInfo& info) {
    std::uint32_t method = stored + info.methodIndexBase;
    if (method < info.methodCount && info.getMethodName(method) == "None")
        return 0;
    return method;
}

std::int32_t getEncodedHeaderPadding(EGame game) {
    switch (game) {
    case EGame::GAME_TorchlightInfinite:
    case EGame::GAME_EtheriaRestart:
    case EGame::GAME_BlackMythWukong:
        return 1;
    case EGame::GAME_InfinityNikki:
        return 20;
    case EGame::GAME_VisionsofMana:
        return -3;
    default:
        return 0;
    }
}

//! Wuthering Waves after pak version 11 and Infinity Nikki move the bits of the encoded entry bitfield.
std::uint32_t remapEncodedBitfield(std::uint32_t value, const FPakInfo& info) {
    if (info.game == EGame::GAME_WutheringWaves && info.version > EPakFileVersion::Fnv64BugFix)
        return ((value >> 16) & 0x3F) | ((value & 0xFFFF) << 6) | ((value & (1u << 28)) >> 6) | ((value & 0x0FC00000) << 1) | ((value & 0xC0000000) >> 1)
               | ((value & 0x20000000) << 2);
    if (info.game == EGame::GAME_InfinityNikki) {
        std::uint32_t blocks = (value >> 6) & 0xFFFF;
        std::uint32_t offset32 = (value >> 31) & 1;
        std::uint32_t size32 = (value >> 22) & 1;
        std::uint32_t uncompressedSize32 = (value >> 30) & 1;
        std::uint32_t blockSize = value & 0x3F;
        std::uint32_t encrypted = (value >> 29) & 1;
        std::uint32_t method = (value >> 23) & 0x3F;
        return blockSize | (blocks << 6) | (encrypted << 22) | (method << 23) | (size32 << 29) | (uncompressedSize32 << 30) | (offset32 << 31);
    }
    return value;
}

std::optional<std::int32_t> findVersion(std::span<const std::byte> tail, const FooterLayout& layout) {
    if (tail.size() < layout.size)
        return std::nullopt;
    std::size_t magicOffset = layout.first >= EPakFileVersion::EncryptionKeyGuid ? 17 : 1;
    std::span<const std::byte> footer = tail.last(layout.size);
    std::uint32_t magic;
    std::int32_t version;
    std::memcpy(&magic, footer.data() + magicOffset, 4);
    std::memcpy(&version, footer.data() + magicOffset + 4, 4);
    if (magic != FPakInfo::magic || version < static_cast<std::int32_t>(layout.first) || version > static_cast<std::int32_t>(layout.last))
        return std::nullopt;
    return version;
}

bool hasValidBlocks(const FPakEntry& entry) {
    if (entry.offset >= maxOffset || entry.size >= maxOffset || entry.uncompressedSize >= maxOffset)
        return false;
    if (entry.compressionMethodIndex == 0 || entry.uncompressedSize == 0)
        return true;
    if (entry.compressionBlockSize == 0)
        return false;
    return entry.blockCount == (entry.uncompressedSize + entry.compressionBlockSize - 1) / entry.compressionBlockSize;
}

}

std::string_view FPakInfo::getMethodName(std::size_t index) const {
    return {methodNames[index], strnlen(methodNames[index], methodNameLength)};
}

Result<FPakInfo> parsePakInfo(std::span<const std::byte> tail, EGame game) {
    if (isCustomGame(game))
        return parseGamePakInfo(tail, game);
    const FooterLayout* layout = nullptr;
    std::int32_t version = 0;
    for (const FooterLayout& candidate : footerLayouts) {
        std::optional<std::int32_t> found = findVersion(tail, candidate);
        if (!found)
            continue;
        layout = &candidate;
        version = *found;
        break;
    }
    if (layout == nullptr)
        return fail(ErrorCode::BadMagic, "parsePakInfo.magic");

    FPakInfo info;
    info.version = static_cast<EPakFileVersion>(version);
    info.byteMethodIndex = layout->byteMethodIndex;
    info.game = game;
    info.footerSize = static_cast<std::uint32_t>(layout->size);
    ByteReader reader{tail.last(layout->size)};
    if (info.version >= EPakFileVersion::EncryptionKeyGuid)
        info.encryptionKeyGuid = reader.read<FGuid>();
    info.encryptedIndex = reader.read<std::uint8_t>() != 0 && info.version >= EPakFileVersion::IndexEncryption;
    reader.skip(8);
    std::int64_t indexOffset = reader.read<std::int64_t>();
    std::int64_t indexSize = reader.read<std::int64_t>();
    reader.skip(20);
    bool frozenIndex = info.version == EPakFileVersion::FrozenIndex && reader.read<std::uint8_t>() != 0;
    std::span<const std::byte> names = reader.readBytes(FPakInfo::methodNameLength * layout->methodCount);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (frozenIndex)
        return fail(ErrorCode::UnsupportedVersion, "parsePakInfo.frozenIndex");
    if (indexOffset < 0 || indexSize < 0 || static_cast<std::uint64_t>(indexOffset) >= maxOffset || static_cast<std::uint64_t>(indexSize) >= maxOffset)
        return fail(ErrorCode::InvalidData, "parsePakInfo.index");
    info.indexOffset = static_cast<std::uint64_t>(indexOffset);
    info.indexSize = static_cast<std::uint64_t>(indexSize);

    if (info.version < EPakFileVersion::FNameBasedCompressionMethod)
        for (std::string_view method : legacyMethods)
            std::memcpy(info.methodNames[info.methodCount++], method.data(), method.size());
    for (std::size_t i = 0; i < layout->methodCount; i++) {
        const char* name = reinterpret_cast<const char*>(names.data() + i * FPakInfo::methodNameLength);
        if (name[0] == 0)
            continue;
        std::memcpy(info.methodNames[info.methodCount], name, FPakInfo::methodNameLength - 1);
        info.methodNames[info.methodCount][FPakInfo::methodNameLength - 1] = 0;
        info.methodCount++;
    }
    return info;
}

Result<FPakInfo> readPakInfo(const FileSource& file, EGame game, std::span<const std::byte> publicKeyPem) {
    std::uint64_t size = file.getSize();
    std::byte tail[maxGameFooterSize];
    std::size_t tailLimit = FPakInfo::serializedSize + 1;
    if (isCustomGame(game))
        tailLimit = game == EGame::GAME_KartRiderDrift || game == EGame::GAME_InZOI ? sizeKartRiderDrift
                    : game == EGame::GAME_DuneAwakening                            ? sizeDuneAwakening
                                                                                   : sizeDeadByDaylight;
    std::size_t tailSize = static_cast<std::size_t>(std::min<std::uint64_t>(size, tailLimit));
    Result<void> read = file.read(size - tailSize, {tail, tailSize});
    if (!read)
        return std::unexpected(read.error());
    if (game == EGame::GAME_InZOI && tailSize >= inZOIEncryptedFooterSize) {
        Result<void> decrypted = decryptInZOIPakInfo({tail, tailSize}, publicKeyPem);
        if (!decrypted)
            return std::unexpected(decrypted.error());
    }
    Result<FPakInfo> info = parsePakInfo({tail, tailSize}, game);
    if (!info)
        return info;
    //! One Piece Ambition stores the real index offset at the stored one, XORed with the inverse of the eight bytes before it.
    if (game == EGame::GAME_OnePieceAmbition) {
        std::int64_t stored[2];
        Result<void> readOffset = file.read(info->indexOffset, std::as_writable_bytes(std::span(stored)));
        if (!readOffset)
            return std::unexpected(readOffset.error());
        std::int64_t indexOffset = stored[1] ^ ~stored[0];
        std::int64_t indexSize = static_cast<std::int64_t>(size - info->footerSize) - indexOffset - 17;
        if (indexOffset < 0 || indexSize < 0)
            return fail(ErrorCode::InvalidData, "readPakInfo.index");
        info->indexOffset = static_cast<std::uint64_t>(indexOffset);
        info->indexSize = static_cast<std::uint64_t>(indexSize);
    }
    if (info->indexOffset + info->indexSize > size)
        return fail(ErrorCode::InvalidData, "readPakInfo.index");
    if (info->encryptedIndex && info->indexSize % aesBlock != 0)
        return fail(ErrorCode::InvalidData, "readPakInfo.indexSize");
    return info;
}

std::uint64_t FPakEntry::getHeaderSize(const FPakInfo& info) const {
    EPakFileVersion version = info.version;
    if (encoded) {
        std::uint64_t size = entryHeaderSize + entryFlagsSize + static_cast<std::uint64_t>(static_cast<std::int64_t>(getEncodedHeaderPadding(info.game)));
        if (compressionMethodIndex == 0)
            return size;
        return size + 4 + std::uint64_t(blockCount) * sizeof(FPakCompressedBlock);
    }
    if (info.game == EGame::GAME_StateOfDecay2 && compressionMethodIndex == 0)
        return 0;
    if (info.game == EGame::GAME_GearsOfWar4) {
        std::uint64_t size = 8 + 4 + 4 + 1;
        if (version < EPakFileVersion::NoTimestamps)
            size += entryTimestampSize;
        if (version < EPakFileVersion::CompressionEncryption)
            return size;
        if (compressionMethodIndex != 0)
            size += 4 + std::uint64_t(blockCount) * sizeof(FPakCompressedBlock);
        return size + 4;
    }
    if (info.game == EGame::GAME_GameForPeace)
        return 20 + 8 + 8 + 4 + 8 + 21 + (compressionMethodIndex != 0 ? 4 + std::uint64_t(blockCount) * sizeof(FPakCompressedBlock) : 0) + 4 + 1;
    std::uint64_t size = info.byteMethodIndex ? entryHeaderSize - 3 : entryHeaderSize;
    if (info.game == EGame::GAME_ConanExiles && version >= EPakFileVersion::CompressionEncryption)
        size += 4;
    if (version <= EPakFileVersion::Initial)
        size += entryTimestampSize;
    if (version < EPakFileVersion::CompressionEncryption)
        return size;
    size += entryFlagsSize;
    //! A Conan Exiles gtp or gts entry keeps its unused block table in the header.
    if (compressionMethodIndex == 0 && (info.game != EGame::GAME_ConanExiles || blockCount == 0))
        return size;
    return size + 4 + std::uint64_t(blockCount) * sizeof(FPakCompressedBlock);
}

Result<std::uint32_t> decodePakEntry(std::span<const std::byte> encoded, std::size_t offset, const FPakInfo& info, PakEntries& out) {
    if (offset > encoded.size())
        return fail(ErrorCode::InvalidData, "decodePakEntry.offset", offset);
    ByteReader reader{encoded.subspan(offset)};
    //! Bits: 31/30/29 offset/raw size/size are 32-bit, 28-23 method, 22 encrypted, 21-6 blocks, 5-0 block size/2048 (0x3f: uint32 follows).
    std::uint32_t value = remapEncodedBitfield(reader.read<std::uint32_t>(), info);
    FPakEntry entry;
    entry.encoded = true;
    bool wutheringWaves = info.game == EGame::GAME_WutheringWaves && info.version > EPakFileVersion::Fnv64BugFix;
    if (wutheringWaves)
        entry.customData = reader.read<std::uint8_t>();
    std::uint32_t blockSize = (value & 0x3f) == 0x3f ? reader.read<std::uint32_t>() : (value & 0x3f) << 11;

    std::uint32_t method = getEntryMethod((value >> 23) & 0x3f, info);
    entry.compressionMethodIndex = static_cast<std::uint8_t>(method < 0xff ? method : 0xff);
    entry.offset = (value & (1u << 31)) != 0 ? reader.read<std::uint32_t>() : reader.read<std::uint64_t>();
    if (info.game == EGame::GAME_Snowbreak)
        entry.offset ^= 0x1F1E1D1C;
    if (info.game == EGame::GAME_QQ || info.game == EGame::GAME_DreamStar)
        entry.offset += 8;
    entry.uncompressedSize = (value & (1u << 30)) != 0 ? reader.read<std::uint32_t>() : reader.read<std::uint64_t>();
    if (wutheringWaves)
        std::swap(entry.offset, entry.uncompressedSize);
    if (entry.compressionMethodIndex != 0)
        entry.size = (value & (1u << 29)) != 0 ? reader.read<std::uint32_t>() : reader.read<std::uint64_t>();
    else
        entry.size = entry.uncompressedSize;
    entry.encrypted = (value & (1u << 22)) != 0;
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (entry.compressionMethodIndex >= info.methodCount)
        return fail(ErrorCode::UnknownCompression, "decodePakEntry.method");

    std::uint32_t blockCount = entry.compressionMethodIndex != 0 ? (value >> 6) & 0xffff : 0;
    entry.blockCount = blockCount;
    if (blockCount == 1)
        blockSize = entry.uncompressedSize <= ~0u ? static_cast<std::uint32_t>(entry.uncompressedSize) : 0;
    entry.compressionBlockSize = blockCount > 0 ? blockSize : 0;
    if (!hasValidBlocks(entry))
        return fail(ErrorCode::InvalidData, "decodePakEntry.blocks", offset);

    entry.firstBlock = static_cast<std::uint32_t>(out.blocks.size());
    std::uint64_t start = entry.offset + entry.getHeaderSize(info);
    if (blockCount == 1 && !entry.encrypted) {
        out.blocks.push_back({start, start + entry.size});
    } else {
        std::uint64_t alignment = entry.encrypted ? aesBlock : 1;
        for (std::uint32_t i = 0; i < blockCount; i++) {
            std::uint32_t length = reader.read<std::uint32_t>();
            out.blocks.push_back({start, start + length});
            start += alignUp(length, alignment);
        }
        if (reader.hasFailed()) {
            out.blocks.resize(entry.firstBlock);
            return std::unexpected(reader.getError());
        }
    }
    out.entries.push_back(entry);
    return static_cast<std::uint32_t>(out.entries.size() - 1);
}

Result<std::uint32_t> readGearsOfWar4PakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out) {
    FPakEntry entry;
    entry.offset = reader.read<std::uint64_t>();
    entry.size = static_cast<std::uint32_t>(reader.read<std::int32_t>());
    entry.uncompressedSize = static_cast<std::uint32_t>(reader.read<std::int32_t>());
    std::uint32_t method = getGearsOfWar4Method(reader.read<std::uint8_t>());
    if (info.version < EPakFileVersion::NoTimestamps)
        reader.skip(entryTimestampSize);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (method >= info.methodCount)
        return fail(ErrorCode::UnknownCompression, "readPakEntry.method");
    entry.compressionMethodIndex = static_cast<std::uint8_t>(method);
    std::size_t firstBlock = out.blocks.size();
    if (info.version >= EPakFileVersion::CompressionEncryption) {
        if (method != 0) {
            std::uint32_t count = reader.readCount(sizeof(FPakCompressedBlock));
            for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
                FPakCompressedBlock block = reader.read<FPakCompressedBlock>();
                out.blocks.push_back(block);
            }
            entry.blockCount = count;
        }
        entry.compressionBlockSize = reader.read<std::uint32_t>();
    }
    entry.firstBlock = static_cast<std::uint32_t>(firstBlock);
    if (reader.hasFailed() || !hasValidBlocks(entry)) {
        out.blocks.resize(firstBlock);
        return reader.hasFailed() ? std::unexpected(reader.getError()) : fail(ErrorCode::InvalidData, "readPakEntry.blocks");
    }
    out.entries.push_back(entry);
    return static_cast<std::uint32_t>(out.entries.size() - 1);
}

Result<std::uint32_t> readGameForPeacePakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out) {
    FPakEntry entry;
    reader.skip(20);
    entry.offset = reader.read<std::uint64_t>();
    entry.uncompressedSize = reader.read<std::uint64_t>();
    std::uint32_t method = reader.read<std::uint32_t>();
    entry.size = reader.read<std::uint64_t>();
    reader.skip(21);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (method >= info.methodCount)
        return fail(ErrorCode::UnknownCompression, "readPakEntry.method");
    entry.compressionMethodIndex = static_cast<std::uint8_t>(method);
    std::size_t firstBlock = out.blocks.size();
    if (method != 0) {
        std::uint32_t count = reader.readCount(sizeof(FPakCompressedBlock));
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++)
            out.blocks.push_back(reader.read<FPakCompressedBlock>());
        entry.blockCount = count;
    }
    entry.compressionBlockSize = reader.read<std::uint32_t>();
    entry.encrypted = (reader.read<std::uint8_t>() & flagEncrypted) != 0;
    entry.firstBlock = static_cast<std::uint32_t>(firstBlock);
    if (reader.hasFailed() || !hasValidBlocks(entry)) {
        out.blocks.resize(firstBlock);
        return reader.hasFailed() ? std::unexpected(reader.getError()) : fail(ErrorCode::InvalidData, "readPakEntry.blocks");
    }
    out.entries.push_back(entry);
    return static_cast<std::uint32_t>(out.entries.size() - 1);
}

Result<std::uint32_t> readPakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out) {
    if (info.game == EGame::GAME_GearsOfWar4)
        return readGearsOfWar4PakEntry(reader, info, out);
    if (info.game == EGame::GAME_GameForPeace)
        return readGameForPeacePakEntry(reader, info, out);
    FPakEntry entry;
    entry.offset = reader.read<std::uint64_t>();
    entry.size = reader.read<std::uint64_t>();
    entry.uncompressedSize = reader.read<std::uint64_t>();
    std::uint32_t method = info.byteMethodIndex ? reader.read<std::uint8_t>() : reader.read<std::uint32_t>();
    if (info.version <= EPakFileVersion::Initial)
        reader.skip(entryTimestampSize);
    reader.skip(20);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (info.version < EPakFileVersion::FNameBasedCompressionMethod)
        method = getLegacyMethod(method, info.game);
    else
        method = getEntryMethod(method, info);
    if (method >= info.methodCount)
        return fail(ErrorCode::UnknownCompression, "readPakEntry.method");
    entry.compressionMethodIndex = static_cast<std::uint8_t>(method);
    if (info.version < EPakFileVersion::CompressionEncryption) {
        if (method != 0)
            return fail(ErrorCode::UnsupportedVersion, "readPakEntry.compression");
        out.entries.push_back(entry);
        return static_cast<std::uint32_t>(out.entries.size() - 1);
    }

    std::uint64_t blockBase = info.version >= EPakFileVersion::RelativeChunkOffsets ? entry.offset : 0;
    std::size_t firstBlock = out.blocks.size();
    if (method != 0) {
        std::uint32_t count = reader.readCount(sizeof(FPakCompressedBlock));
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
            FPakCompressedBlock block = reader.read<FPakCompressedBlock>();
            if (block.start >= maxOffset || block.end >= maxOffset || block.end < block.start) {
                out.blocks.resize(firstBlock);
                return fail(ErrorCode::InvalidData, "readPakEntry.block");
            }
            out.blocks.push_back({blockBase + block.start, blockBase + block.end});
        }
        entry.blockCount = count;
    }
    std::uint8_t flags = reader.read<std::uint8_t>();
    entry.compressionBlockSize = reader.read<std::uint32_t>();
    if (info.game == EGame::GAME_ConanExiles)
        reader.skip(4);
    if (info.game == EGame::GAME_TEKKEN7)
        flags &= ~flagEncrypted;
    if (reader.hasFailed()) {
        out.blocks.resize(firstBlock);
        return std::unexpected(reader.getError());
    }
    entry.encrypted = (flags & flagEncrypted) != 0;
    entry.firstBlock = static_cast<std::uint32_t>(firstBlock);
    if (!hasValidBlocks(entry)) {
        out.blocks.resize(firstBlock);
        return fail(ErrorCode::InvalidData, "readPakEntry.blocks");
    }
    out.entries.push_back(entry);
    return static_cast<std::uint32_t>(out.entries.size() - 1);
}

bool needsEntryPath(EGame game) {
    return game == EGame::GAME_MarvelRivals || game == EGame::GAME_WutheringWaves || game == EGame::GAME_OperationApocalypse || game == EGame::GAME_MindsEye
           || game == EGame::GAME_ArenaBreakoutInfinite || game == EGame::GAME_Snowbreak || game == EGame::GAME_Strinova
           || game == EGame::GAME_NevernessToEverness || game == EGame::GAME_GameForPeace;
}

void applyEntryPath(const FPakInfo& info, std::string_view path, FPakEntry& entry) {
    switch (info.game) {
    case EGame::GAME_MarvelRivals: {
        //! Marvel Rivals hashes 0x44332211 and the lower-case path with BLAKE3 to get the encrypted size.
        std::string seeded = "\x11\x22\x33\x44";
        for (char c : path)
            seeded.push_back(c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c);
        std::array<std::uint8_t, 32> hash = hashBlake3(std::as_bytes(std::span(seeded)));
        std::uint64_t first = 0;
        std::memcpy(&first, hash.data(), sizeof(first));
        entry.encryptedLimit = static_cast<std::uint32_t>((63 * (first % 0x3D) + 319) & ~std::uint64_t{0x3F});
        return;
    }
    case EGame::GAME_OperationApocalypse:
    case EGame::GAME_MindsEye:
        entry.encryptedLimit = 0x1000;
        return;
    case EGame::GAME_ArenaBreakoutInfinite:
        entry.customData = static_cast<std::uint8_t>(getArenaBreakoutInfiniteFile(path));
        return;
    case EGame::GAME_Snowbreak:
    case EGame::GAME_Strinova:
        entry.customData = path.ends_with(".lua") ? transformedFile : 0;
        return;
    case EGame::GAME_NevernessToEverness:
        entry.customData = path.ends_with(".ini") ? transformedFile : 0;
        return;
    case EGame::GAME_GameForPeace:
        //! Game for Peace decrypts ini files only when they are compressed.
        entry.customData = path.ends_with(".ini") && entry.compressionMethodIndex != 0 ? transformedFile : 0;
        return;
    case EGame::GAME_WutheringWaves:
        if (entry.customData == 1)
            entry.encryptedLimit = 0x200000;
        else if (entry.customData == 2)
            entry.encryptedLimit = 0x800;
        else if (entry.customData == 4)
            entry.encryptedLimit = 0;
        return;
    default:
        return;
    }
}

namespace {

//! Crystal of Atlan writes the entry count first, then the mount point, three unknown bytes, the seed and the encoded entries before the directory index.
Result<PakPrimaryIndex> parseCrystalOfAtlanPrimaryIndex(std::span<const std::byte> index) {
    ByteReader reader{index};
    PakPrimaryIndex primary;
    std::int32_t entryCount = reader.read<std::int32_t>();
    primary.mountPoint = reader.readStringBytes(primary.mountPointWide);
    reader.skip(3 + 8);
    primary.encodedEntries = reader.readBytes(reader.readCount(1));
    reader.skip(8);
    bool hasDirectoryIndex = reader.read<std::uint32_t>() != 0;
    std::int64_t directoryOffset = reader.read<std::int64_t>();
    std::int64_t directorySize = reader.read<std::int64_t>();
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (entryCount < 0)
        return fail(ErrorCode::InvalidData, "parsePakPrimaryIndex.entryCount");
    if (!hasDirectoryIndex)
        return fail(ErrorCode::UnsupportedVersion, "parsePakPrimaryIndex.directoryIndex");
    if (directoryOffset < 0 || directorySize < 0 || static_cast<std::uint64_t>(directoryOffset) >= maxOffset
        || static_cast<std::uint64_t>(directorySize) >= maxOffset)
        return fail(ErrorCode::InvalidData, "parsePakPrimaryIndex.directoryIndex");
    primary.entryCount = static_cast<std::uint32_t>(entryCount);
    primary.directoryIndexOffset = static_cast<std::uint64_t>(directoryOffset);
    primary.directoryIndexSize = static_cast<std::uint64_t>(directorySize);
    return primary;
}

}

std::size_t getPakMountPointOffset(const FPakInfo& info) {
    if (info.version < EPakFileVersion::PathHashIndex)
        return 0;
    if (info.game == EGame::GAME_DreamStar || info.game == EGame::GAME_DeltaForce)
        return 8 + 4;
    if (info.game == EGame::GAME_CrystalOfAtlan)
        return 4;
    return 0;
}

Result<PakPrimaryIndex> parsePakPrimaryIndex(std::span<const std::byte> index, const FPakInfo& info) {
    if (info.game == EGame::GAME_CrystalOfAtlan)
        return parseCrystalOfAtlanPrimaryIndex(index);
    ByteReader reader{index};
    PakPrimaryIndex primary;
    EGame game = info.game;
    //! Dream Star and Delta Force write the path hash seed and the entry count before the mount point.
    bool countFirst = game == EGame::GAME_DreamStar || game == EGame::GAME_DeltaForce;
    std::int32_t entryCount = 0;
    if (countFirst) {
        reader.skip(8);
        entryCount = reader.read<std::int32_t>();
    }
    primary.mountPoint = reader.readStringBytes(primary.mountPointWide);
    if (!countFirst) {
        entryCount = reader.read<std::int32_t>();
        reader.skip(8);
    }
    if (reader.read<std::uint32_t>() != 0)
        reader.skip(8 + 8 + 20);
    if (game == EGame::GAME_Rennsport)
        reader.skip(16);
    bool hasDirectoryIndex = reader.read<std::uint32_t>() != 0;
    if (game == EGame::GAME_TheDivisionResurgence)
        reader.skip(40);
    std::int64_t directoryOffset = -1;
    std::int64_t directorySize = 0;
    if (hasDirectoryIndex) {
        directoryOffset = reader.read<std::int64_t>();
        directorySize = reader.read<std::int64_t>();
        reader.skip(20);
        if (game == EGame::GAME_Rennsport)
            reader.skip(20);
    }
    std::uint32_t encodedSize = 0;
    //! Rennsport stores no size; the encoded entries run to six bytes before the end.
    if (game == EGame::GAME_Rennsport) {
        std::size_t position = index.size() - reader.getRemaining();
        encodedSize = index.size() >= position + 6 ? static_cast<std::uint32_t>(index.size() - position - 6) : 0;
    } else {
        encodedSize = reader.readCount(1);
    }
    primary.encodedEntries = reader.readBytes(encodedSize);
    std::uint32_t nonEncodedCount = reader.readCount(entryHeaderSize);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (entryCount < 0)
        return fail(ErrorCode::InvalidData, "parsePakPrimaryIndex.entryCount");
    if (!hasDirectoryIndex || directoryOffset == -1)
        return fail(ErrorCode::UnsupportedVersion, "parsePakPrimaryIndex.directoryIndex");
    if (directoryOffset < 0 || directorySize < 0 || static_cast<std::uint64_t>(directoryOffset) >= maxOffset
        || static_cast<std::uint64_t>(directorySize) >= maxOffset)
        return fail(ErrorCode::InvalidData, "parsePakPrimaryIndex.directoryIndex");
    primary.entryCount = static_cast<std::uint32_t>(entryCount);
    primary.directoryIndexOffset = static_cast<std::uint64_t>(directoryOffset);
    primary.directoryIndexSize = static_cast<std::uint64_t>(directorySize);

    primary.nonEncoded.entries.reserve(nonEncodedCount);
    for (std::uint32_t i = 0; i < nonEncodedCount; i++) {
        Result<std::uint32_t> entry = readPakEntry(reader, info, primary.nonEncoded);
        if (!entry)
            return std::unexpected(entry.error());
    }
    return primary;
}

PakFile::PakFile(FPakInfo info, PakEntries entries, std::unique_ptr<FileSource> file, std::vector<const Decompressor*> decompressors,
                 const Decryptor* decryptor)
    : info(info), entries(std::move(entries)), file(std::move(file)), decompressors(std::move(decompressors)), decryptor(decryptor) {
    UE5PARSE_CHECK(this->decompressors.size() == this->info.methodCount);
}

PakFile::~PakFile() {
    forgetDecodedBlocks();
}

namespace {

//! Dragon Quest XI XORs compressed blocks with an eight-byte key and stored files with 0xFF.
constexpr std::uint8_t dragonQuestXIKey[] = {0x21, 0x52, 0x05, 0x21, 0x41, 0x10, 0x35, 0x01};

bool hasBlockTransform(EGame game) {
    return game == EGame::GAME_DragonQuestXI;
}

void transformBlock(EGame game, std::span<std::byte> block) {
    if (game == EGame::GAME_DragonQuestXI)
        for (std::size_t i = 0; i < block.size(); i++)
            block[i] ^= static_cast<std::byte>(dragonQuestXIKey[i & 7]);
}

void transformStored(EGame game, std::span<std::byte> data) {
    if (game == EGame::GAME_DragonQuestXI)
        for (std::byte& value : data)
            value ^= std::byte{0xFF};
}

}

Result<void> PakFile::decrypt(std::uint64_t position, std::span<std::byte> raw, std::uint64_t streamStart) const {
    if (decryptor == nullptr)
        return fail(ErrorCode::MissingKey, "PakFile.decryptor", position);
    DecryptRequest request;
    //! Rennsport chains AES-CBC from the start of the file data, or from its first compressed block.
    std::array<std::byte, aesBlock> chain{};
    if (info.game == EGame::GAME_Rennsport && position >= streamStart + aesBlock) {
        Result<void> read = file->read(position - aesBlock, chain);
        if (!read)
            return read;
        request.chain = chain;
    }
    request.data = raw;
    request.keyGuid = info.encryptionKeyGuid;
    request.method = EncryptionMethod::AesEcb;
    request.part = EncryptedPart::PakEntry;
    request.offset = position;
    request.game = info.game;
    request.archiveName = file->getName();
    Result<void> decrypted = decryptor->decrypt(request);
    if (!decrypted || info.game != EGame::GAME_DeltaForce)
        return decrypted;
    if (info.customEncryptionDataSize == 0)
        return fail(ErrorCode::BadAesKey, "PakFile.deltaForceMask", position);
    for (std::size_t i = 0; i < raw.size() / 8 * 8; i++)
        raw[i] ^= info.customEncryptionData[i % 8];
    return {};
}

namespace {

//! Follows CUE4Parse: blocks are encrypted in full while they are smaller than the bytes left, then one block is encrypted up to the limit.
std::vector<std::uint32_t> getEncryptedBlockSizes(std::span<const FPakCompressedBlock> blocks, std::uint32_t limit) {
    std::vector<std::uint32_t> sizes(blocks.size(), 0);
    std::int64_t left = limit;
    for (std::size_t i = 0; i < blocks.size() && left > 0; i++) {
        std::int64_t size = static_cast<std::int64_t>(blocks[i].end - blocks[i].start);
        std::int64_t stored = static_cast<std::int64_t>(alignUp(static_cast<std::uint64_t>(size), aesBlock));
        sizes[i] = static_cast<std::uint32_t>(size < left ? stored : left / aesBlock * aesBlock);
        left = size < left ? left - stored : 0;
    }
    return sizes;
}

}

class PakFile::PakEntryBlocks final : public BlockList {
public:
    PakEntryBlocks(const PakFile& pak, const FPakEntry& entry) : pak(pak), entry(entry), blocks(pak.getBlocks(entry)) {
        if (entry.encrypted && entry.encryptedLimit != UINT32_MAX)
            encryptedSizes = getEncryptedBlockSizes(blocks, entry.encryptedLimit);
        centuryPwc = pak.info.game == EGame::GAME_CenturyAgeofAshes && pak.info.getMethodName(entry.compressionMethodIndex) == "PWC";
    }

    Result<StoredBlock> getBlock(std::size_t index) const override {
        if (index >= blocks.size())
            return fail(ErrorCode::InvalidData, "PakFile.read.blocks", index);
        const FPakCompressedBlock& stored = blocks[index];
        if (stored.end < stored.start)
            return fail(ErrorCode::InvalidData, "PakFile.read.block", stored.start);
        if (entry.encrypted && pak.decryptor == nullptr)
            return fail(ErrorCode::MissingKey, "PakFile.decryptor", stored.start);
        StoredBlock block;
        block.file = pak.file.get();
        block.position = stored.start;
        block.compressedSize = static_cast<std::size_t>(stored.end - stored.start);
        block.storedSize = static_cast<std::size_t>(entry.encrypted ? alignUp(block.compressedSize, aesBlock) : block.compressedSize);
        if (entry.encrypted && pak.info.game == EGame::GAME_eBaseballProSpirit)
            block.storedSize = static_cast<std::size_t>(alignUp(block.compressedSize + proSpiTrailerSize, aesBlock));
        std::uint64_t blockSize = entry.compressionBlockSize;
        block.uncompressedSize = static_cast<std::size_t>(std::min<std::uint64_t>(blockSize, entry.uncompressedSize - index * blockSize));
        block.decompressor = pak.decompressors[entry.compressionMethodIndex];
        block.encrypted = entry.encrypted || hasBlockTransform(pak.info.game) || centuryPwc;
        return block;
    }

    Result<void> decrypt(std::size_t index, std::span<std::byte> stored) const override {
        if (entry.encrypted) {
            std::span<std::byte> encrypted = stored;
            if (!encryptedSizes.empty())
                encrypted = stored.first(std::min<std::size_t>(encryptedSizes[index], stored.size()));
            Result<void> decrypted = encrypted.empty() ? Result<void>{} : pak.decrypt(blocks[index].start, encrypted, blocks.front().start);
            if (!decrypted)
                return decrypted;
        }
        transformBlock(pak.info.game, stored);
        if (centuryPwc)
            decryptCenturyPwc(stored.first(std::min<std::size_t>(static_cast<std::size_t>(blocks[index].end - blocks[index].start), stored.size())));
        return {};
    }

private:
    const PakFile& pak;
    const FPakEntry& entry;
    std::span<const FPakCompressedBlock> blocks;
    std::vector<std::uint32_t> encryptedSizes;
    bool centuryPwc = false;
};

std::uint64_t PakFile::getFileSize(std::size_t index) const {
    const FPakEntry& entry = entries.entries[index];
    if (info.game == EGame::GAME_NevernessToEverness && entry.customData == transformedFile) {
        Result<std::vector<std::byte>> file = readTransformed(index, {});
        return file ? file->size() : entry.uncompressedSize;
    }
    if (info.game != EGame::GAME_ArenaBreakoutInfinite || entry.customData != static_cast<std::uint8_t>(ArenaBreakoutInfiniteFile::Ini) || entry.uncompressedSize < 8)
        return entry.uncompressedSize;
    std::array<std::byte, 8> start{};
    if (!readStored(index, 0, start, {}))
        return entry.uncompressedSize;
    return getArenaBreakoutInfiniteIniSize(start, entry.uncompressedSize);
}

Result<void> PakFile::read(std::size_t entryIndex, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) const {
    if (info.game == EGame::GAME_ArenaBreakoutInfinite && entryIndex < entries.entries.size()
        && entries.entries[entryIndex].customData != static_cast<std::uint8_t>(ArenaBreakoutInfiniteFile::Other))
        return readArenaBreakoutInfinite(entryIndex, offset, output, stop);
    if (hasFileTransform(info.game) && entryIndex < entries.entries.size() && entries.entries[entryIndex].customData == transformedFile) {
        Result<std::vector<std::byte>> file = readTransformed(entryIndex, stop);
        if (!file)
            return std::unexpected(file.error());
        if (offset > file->size() || output.size() > file->size() - offset)
            return fail(ErrorCode::ReadPastEnd, "PakFile.read.range", offset);
        std::memcpy(output.data(), file->data() + offset, output.size());
        return {};
    }
    return readStored(entryIndex, offset, output, stop);
}

//! CUE4Parse decrypts these files as a whole after it reads them from the pak.
Result<std::vector<std::byte>> PakFile::readTransformed(std::size_t entryIndex, std::stop_token stop) const {
    std::vector<std::byte> file(static_cast<std::size_t>(entries.entries[entryIndex].uncompressedSize));
    Result<void> read = readStored(entryIndex, 0, file, stop);
    if (!read)
        return std::unexpected(read.error());
    switch (info.game) {
    case EGame::GAME_Snowbreak:
        decryptSnowbreakLua(file);
        return file;
    case EGame::GAME_Strinova:
        decryptStrinovaLua(file);
        return file;
    case EGame::GAME_GameForPeace:
        decryptGameForPeaceIni(file);
        return file;
    case EGame::GAME_NevernessToEverness:
        return decryptNevernessToEvernessIni(file);
    default:
        return file;
    }
}

//! Arena Breakout Infinite encrypts the package summary, ini files and Lua bytecode again inside the pak data.
Result<void> PakFile::readArenaBreakoutInfinite(std::size_t entryIndex, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) const {
    const FPakEntry& entry = entries.entries[entryIndex];
    auto kind = static_cast<ArenaBreakoutInfiniteFile>(entry.customData);
    std::uint64_t storedSize = entry.uncompressedSize;
    if (kind == ArenaBreakoutInfiniteFile::Package) {
        std::array<std::byte, 8> start{};
        if (storedSize < start.size())
            return readStored(entryIndex, offset, output, stop);
        Result<void> read = readStored(entryIndex, 0, start, stop);
        if (!read)
            return read;
        std::uint64_t summarySize = getArenaBreakoutInfiniteSummarySize(start);
        if (offset >= summarySize)
            return readStored(entryIndex, offset, output, stop);
        if (offset > storedSize || output.size() > storedSize - offset)
            return fail(ErrorCode::ReadPastEnd, "PakFile.read.range", offset);
        std::vector<std::byte> head(static_cast<std::size_t>(std::min(storedSize, std::max(summarySize, offset + output.size()))));
        read = readStored(entryIndex, 0, head, stop);
        if (!read)
            return read;
        Result<void> decrypted = decryptArenaBreakoutInfinitePackage(head);
        if (!decrypted)
            return decrypted;
        std::memcpy(output.data(), head.data() + offset, output.size());
        return {};
    }
    std::vector<std::byte> file(static_cast<std::size_t>(storedSize));
    Result<void> read = readStored(entryIndex, 0, file, stop);
    if (!read)
        return read;
    if (kind == ArenaBreakoutInfiniteFile::Ini && file.size() >= 8) {
        Result<std::vector<std::byte>> decrypted = decryptArenaBreakoutInfiniteIni(file);
        if (!decrypted)
            return std::unexpected(decrypted.error());
        file = std::move(*decrypted);
    } else if (kind == ArenaBreakoutInfiniteFile::Lua && file.size() >= 12) {
        Result<void> decrypted = decryptArenaBreakoutInfiniteLua(file);
        if (!decrypted)
            return decrypted;
    }
    if (offset > file.size() || output.size() > file.size() - offset)
        return fail(ErrorCode::ReadPastEnd, "PakFile.read.range", offset);
    std::memcpy(output.data(), file.data() + offset, output.size());
    return {};
}

//! eBaseball Pro Spirit encrypts an uncompressed file as one buffer with its trailer, so it decrypts the full file.
Result<void> PakFile::readProSpiStored(const FPakEntry& entry, std::uint64_t dataStart, std::uint64_t offset, std::span<std::byte> output) const {
    std::uint64_t size = alignUp(entry.uncompressedSize + proSpiTrailerSize, aesBlock);
    std::vector<std::byte>& raw = threadBuffer(0);
    if (raw.size() < size)
        raw.resize(size);
    std::span<std::byte> stored{raw.data(), static_cast<std::size_t>(size)};
    Result<void> read = file->read(dataStart, stored);
    if (!read)
        return read;
    Result<void> decrypted = decrypt(dataStart, stored, dataStart);
    if (!decrypted)
        return decrypted;
    std::memcpy(output.data(), raw.data() + offset, output.size());
    return {};
}

Result<void> PakFile::readStored(std::size_t entryIndex, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) const {
    if (entryIndex >= entries.entries.size())
        return fail(ErrorCode::MissingFile, "PakFile.read.index");
    const FPakEntry& entry = entries.entries[entryIndex];
    if (offset > entry.uncompressedSize || output.size() > entry.uncompressedSize - offset)
        return fail(ErrorCode::ReadPastEnd, "PakFile.read.range", offset);
    if (output.empty())
        return {};
    if (stop.stop_requested())
        return fail(ErrorCode::Cancelled, "PakFile.read");

    std::uint64_t dataStart = entry.offset + entry.getHeaderSize(info);
    if (entry.compressionMethodIndex == 0) {
        if (!entry.encrypted) {
            Result<void> read = file->read(dataStart + offset, output);
            if (read)
                transformStored(info.game, output);
            return read;
        }
        if (info.game == EGame::GAME_eBaseballProSpirit)
            return readProSpiStored(entry, dataStart, offset, output);
        std::uint64_t first = offset / aesBlock * aesBlock;
        std::uint64_t end = alignUp(offset + output.size(), aesBlock);
        std::vector<std::byte>& raw = threadBuffer(0);
        if (raw.size() < end - first)
            raw.resize(end - first);
        std::span<std::byte> stored{raw.data(), static_cast<std::size_t>(end - first)};
        Result<void> read = file->read(dataStart + first, stored);
        if (!read)
            return read;
        std::span<std::byte> encrypted = stored;
        if (first >= entry.encryptedLimit)
            encrypted = {};
        else if (end > entry.encryptedLimit)
            encrypted = stored.first(static_cast<std::size_t>(entry.encryptedLimit - first));
        Result<void> decrypted = encrypted.empty() ? Result<void>{} : decrypt(dataStart + first, encrypted, dataStart);
        if (!decrypted)
            return decrypted;
        std::memcpy(output.data(), raw.data() + (offset - first), output.size());
        transformStored(info.game, output);
        return {};
    }

    PakEntryBlocks blocks(*this, entry);
    return readBlocks(blocks, entry.compressionBlockSize, offset, output, stop);
}

}
