#pragma once

#include <optional>
#include <string_view>

#include "CppUE5Parse/Core/Versions.h"

namespace ue5parse {

struct GuessedMapTypes {
    std::string_view keyType;
    std::string_view valueType;
};

//! Before PROPERTY_TAG_SET_MAP_SUPPORT a map tag names no key or value type, so CUE4Parse knows them per game from the property name.
std::optional<GuessedMapTypes> guessMapPropertyTypes(EGame game, std::string_view propertyName);

//! The inner property type that CUE4Parse forces for a set of these games, or empty.
std::string_view guessSetInnerType(EGame game, std::string_view propertyName);

//! The struct of a set of structs whose tag names no struct, or empty.
std::string_view guessSetStructType(EGame game, std::string_view propertyName);

//! The struct of an array of structs before INNER_ARRAY_TAG_INFO, or empty.
std::string_view guessArrayStructType(EGame game, std::string_view propertyName, int elementSize);

}
