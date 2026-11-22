#include "CppUE5Parse/GameTypes/PropertyTypeGuesses.h"

#include <array>
#include <utility>

namespace ue5parse {

namespace {

constexpr std::string_view nameProperty = "NameProperty";
constexpr std::string_view intProperty = "IntProperty";
constexpr std::string_view objectProperty = "ObjectProperty";
constexpr std::string_view structProperty = "StructProperty";
constexpr std::string_view strProperty = "StrProperty";

template <std::size_t Size>
bool isOneOf(std::string_view name, const std::array<std::string_view, Size>& names) {
    for (std::string_view candidate : names)
        if (candidate == name)
            return true;
    return false;
}

std::optional<GuessedMapTypes> guessDaysGoneMap(std::string_view name) {
    if (name == "ScalarMap")
        return GuessedMapTypes{nameProperty, intProperty};
    constexpr std::array<std::string_view, 5> objectMaps = {"SoundCueMap", "SoundCueAssetMap", "PlayerSoundCueMap", "FriendSoundCueMap", "ParticleMap"};
    if (isOneOf(name, objectMaps))
        return GuessedMapTypes{nameProperty, objectProperty};
    return std::nullopt;
}

std::optional<GuessedMapTypes> guessStateOfDecay2Map(std::string_view name) {
    constexpr std::array<std::string_view, 13> nameStructMaps = {
        "ThreadsByName",     "Conversations",    "Scenes",          "BuffDefs", "CastingMap", "CriticalMissionMap", "ObjectivesMap", "PhaseEventMap",
        "ArcMissionEventIndices", "RequirementsMap", "CookedComponentInstancingData", "CollectionEntryCache", "Characters"};
    constexpr std::array<std::string_view, 2> objectStructMaps = {"PrimitiveComponentToSampleResourceId", "FacilitySlotToSampleResourceId"};
    constexpr std::array<std::string_view, 2> objectObjectMaps = {"SearchableMaterials", "InteractableMaterials"};
    constexpr std::array<std::string_view, 4> enumTextMaps = {"GameModeDescriptors", "ZombieTextNameStructs", "ZombieTextNameAlternatesStructs",
                                                              "ZombieBloodPlagueQualifierStructs"};
    if (isOneOf(name, nameStructMaps))
        return GuessedMapTypes{nameProperty, structProperty};
    if (name == "Assets")
        return GuessedMapTypes{strProperty, structProperty};
    if (name == "AllHumanDefinitions")
        return GuessedMapTypes{nameProperty, objectProperty};
    if (name == "SortedDefinitionsByFace")
        return GuessedMapTypes{structProperty, structProperty};
    if (isOneOf(name, objectStructMaps))
        return GuessedMapTypes{objectProperty, structProperty};
    if (isOneOf(name, objectObjectMaps))
        return GuessedMapTypes{objectProperty, objectProperty};
    if (name == "ComponentMap")
        return GuessedMapTypes{"UInt32Property", objectProperty};
    if (isOneOf(name, enumTextMaps))
        return GuessedMapTypes{"EnumProperty", "TextProperty"};
    return std::nullopt;
}

std::string_view guessDuneAwakeningSetStruct(std::string_view name) {
    constexpr std::array<std::string_view, 10> stringEnumSets = {
        "AllowedItems",        "RequiredProductionTypes", "m_SupportedProductionTypes", "m_ActionsOnReceive", "m_DesiredInputContexts",
        "m_SupportedFuelTypes", "m_MapMarkersToCount",    "m_ValidContactGroups",       "m_Placeables",       "m_Buildables"};
    constexpr std::array<std::string_view, 3> tagSets = {"CharacterStateTags", "PrereqModuleTags_Or", "PrereqModuleTags_And"};
    if (isOneOf(name, stringEnumSets))
        return "StringEnumValue";
    if (name == "ParameterInfoSet")
        return "MaterialParameterInfo";
    if (isOneOf(name, tagSets))
        return "GameplayTag";
    return {};
}

std::string_view guessDaysGoneArrayStruct(std::string_view name, int elementSize) {
    using Entry = std::pair<std::string_view, std::string_view>;
    static constexpr std::array<Entry, 50> structs = {{
        {"ParameterIds", "Guid"},
        {"ReferenceFunctionIdCache", "Guid"},
        {"SwipeStart", "Vector2D"},
        {"InputCurvePoints", "Vector2D"},
        {"StickyScorePositions", "Vector2D"},
        {"SectorsToRender", "Vector2D"},
        {"MapPerimeterPoly", "Vector2D"},
        {"Colors", "LinearColor"},
        {"SubtitleColors", "LinearColor"},
        {"BackgroundColors", "LinearColor"},
        {"SubtitleColor", "LinearColor"},
        {"IndexedColors", "LinearColor"},
        {"PrimaryColor", "LinearColor"},
        {"SecondaryColor", "LinearColor"},
        {"RankColor", "LinearColor"},
        {"DebugPathColors", "LinearColor"},
        {"InteractButtonIconColorCorrespondingArray", "LinearColor"},
        {"InteractButtonTextureColorCorrespondingArray", "LinearColor"},
        {"initialCompRot", "Rotator"},
        {"Points_BP", "Vector"},
        {"Positions", "Vector"},
        {"Normals", "Vector"},
        {"LocalNormals", "Vector"},
        {"LocalPositions", "Vector"},
        {"TransformedNormals", "Vector"},
        {"TransformedPositions", "Vector"},
        {"GeneratedLocations", "Vector"},
        {"TrackingGameplayLocations", "Vector"},
        {"StaticNavigableGeometry", "Vector"},
        {"LocationsForConversationToAvoid", "Vector"},
        {"FacingSpeedRanges", "Vector"},
        {"s_FoamClothScale", "Vector"},
        {"s_MeshScales", "Vector"},
        {"s_MeshMassOffset", "Vector"},
        {"VectorLocationsForTunnels_Temp", "Vector"},
        {"BreadcrumbLocations", "Vector"},
        {"ObjectiveLocations", "Vector"},
        {"ValidEndLocations", "Vector"},
        {"s_DummyInitPos", "Vector"},
        {"Position_10_4C15053445A03427D18C5D96DBE7C392", "Vector"},
        {"ExtraSoundLocations", "Vector"},
        {"EndLocations", "Vector"},
        {"DebugBikeTrailRenderCoords", "Vector"},
        {"TempRagerBoxExtents", "Vector"},
        {"SpawnPoints", "Vector"},
        {"Bike Position", "Vector"},
        {"IncludeMeshes", "StringAssetReference"},
        {"ExcludeMeshes", "StringAssetReference"},
        {"IncludePhysicalMaterials", "StringAssetReference"},
        {"ExcludePhysicalMaterials", "StringAssetReference"},
    }};
    if (name == "Keys")
        return elementSize == 27 ? "RichCurveKey" : std::string_view();
    if (name == "VertexData")
        return elementSize == 8 ? "Vector2D" : "Vector";
    if (name == "Points")
        return elementSize == 12 ? "Vector" : std::string_view();
    for (const auto& [property, structName] : structs)
        if (property == name)
            return structName;
    return {};
}

}

std::optional<GuessedMapTypes> guessMapPropertyTypes(EGame game, std::string_view propertyName) {
    switch (game) {
    case EGame::GAME_DaysGone:
        return guessDaysGoneMap(propertyName);
    case EGame::GAME_StateOfDecay2:
        return guessStateOfDecay2Map(propertyName);
    case EGame::GAME_WeHappyFew:
        if (propertyName == "PointMap" || propertyName == "JunctionMap" || propertyName == "RoadMap")
            return GuessedMapTypes{intProperty, structProperty};
        if (propertyName == "States")
            return GuessedMapTypes{nameProperty, structProperty};
        return std::nullopt;
    default:
        return std::nullopt;
    }
}

std::string_view guessSetInnerType(EGame game, std::string_view propertyName) {
    if (game == EGame::GAME_StateOfDecay2) {
        if (propertyName == "AllEntityIds" || propertyName == "SceneNameSet")
            return nameProperty;
        if (propertyName == "TextVarSources")
            return strProperty;
    }
    if (game == EGame::GAME_HonorofKingsWorld && (propertyName == "LogicState" || propertyName == "CategorySet"))
        return "EnumProperty";
    return {};
}

std::string_view guessSetStructType(EGame game, std::string_view propertyName) {
    switch (game) {
    case EGame::GAME_AssaultFireFuture:
        if (propertyName == "Data")
            return "SoftObjectPath";
        if (propertyName == "MGCIdList")
            return "GPRowName";
        if (propertyName == "PrimitiveBindingGuids" || propertyName == "RefSets" || propertyName == "OwnerActors")
            return "Guid";
        break;
    case EGame::GAME_ThroneAndLiberty:
        if (propertyName == "ExcludeMeshes" || propertyName == "IncludeMeshes")
            return "SoftObjectPath";
        break;
    case EGame::GAME_MetroAwakening:
        if (propertyName == "SoundscapePaletteCollection")
            return "SoftObjectPath";
        break;
    case EGame::GAME_Avowed:
        if (propertyName.ends_with("IDs"))
            return "Guid";
        break;
    case EGame::GAME_Farlight84:
        return "SoftObjectPath";
    case EGame::GAME_HonorofKingsWorld:
        if (propertyName == "WaterPhysicalMaterials")
            return "SoftObjectPath";
        if (propertyName == "InstanceIdsNotToHide")
            return "Guid";
        break;
    case EGame::GAME_ChasingKaleidoRIDER:
        if (propertyName == "DialogueEntityInstances")
            return "Guid";
        break;
    case EGame::GAME_DuneAwakening:
        return guessDuneAwakeningSetStruct(propertyName);
    default:
        break;
    }
    if (propertyName == "AnimSequenceInstances" || propertyName == "PostProcessInstances")
        return "Guid";
    return {};
}

std::string_view guessArrayStructType(EGame game, std::string_view propertyName, int elementSize) {
    return game == EGame::GAME_DaysGone ? guessDaysGoneArrayStruct(propertyName, elementSize) : std::string_view();
}

}
