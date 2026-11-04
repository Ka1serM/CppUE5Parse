#pragma once

#include <expected>
#include <optional>
#include <string>
#include <utility>

#include "CppUE5Parse/Core/Result.h"

namespace ue5parse::conversion {

struct ConversionError {
    std::string message;
    std::optional<Error> cause;
};

template <class T>
using ConversionResult = std::expected<T, ConversionError>;

inline std::unexpected<ConversionError> conversionFailure(std::string message) {
    return std::unexpected(ConversionError{std::move(message), std::nullopt});
}

inline std::unexpected<ConversionError> conversionFailure(const Error& cause) {
    return std::unexpected(ConversionError{{}, cause});
}

}
