#pragma once

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace test {

class Json {
public:
    using Array = std::vector<Json>;
    using Object = std::vector<std::pair<std::string, Json>>;

    Json() = default;
    explicit Json(bool value) : value(value) {}
    explicit Json(double value) : value(value) {}
    explicit Json(std::string value) : value(std::move(value)) {}
    explicit Json(Array value) : value(std::move(value)) {}
    explicit Json(Object value) : value(std::move(value)) {}

    static std::optional<Json> parse(std::string_view text) {
        Reader reader{text};
        std::optional<Json> json = reader.readValue();
        reader.skipSpace();
        if (!reader.text.empty())
            return std::nullopt;
        return json;
    }

    bool isBool() const { return std::holds_alternative<bool>(value); }
    bool isNumber() const { return std::holds_alternative<double>(value); }
    bool isString() const { return std::holds_alternative<std::string>(value); }
    bool isArray() const { return std::holds_alternative<Array>(value); }
    bool isObject() const { return std::holds_alternative<Object>(value); }

    double getNumber() const { return std::get<double>(value); }
    bool getBool() const { return std::get<bool>(value); }
    const std::string& getString() const { return std::get<std::string>(value); }
    const Array& getArray() const { return std::get<Array>(value); }
    const Object& getObject() const { return std::get<Object>(value); }

    const Json* find(std::string_view key) const {
        if (!isObject())
            return nullptr;
        const Object& object = getObject();
        auto found = std::ranges::find(object, key, &Object::value_type::first);
        return found == object.end() ? nullptr : &found->second;
    }

private:
    struct Reader {
        std::string_view text;

        void skipSpace() {
            while (!text.empty() && (text[0] == ' ' || text[0] == '\n' || text[0] == '\r' || text[0] == '\t'))
                text.remove_prefix(1);
        }

        bool take(std::string_view token) {
            skipSpace();
            if (!text.starts_with(token))
                return false;
            text.remove_prefix(token.size());
            return true;
        }

        std::optional<std::string> readString() {
            if (!take("\""))
                return std::nullopt;
            std::string result;
            while (!text.empty() && text[0] != '"') {
                if (text[0] != '\\') {
                    result += text[0];
                    text.remove_prefix(1);
                    continue;
                }
                if (text.size() < 2)
                    return std::nullopt;
                char escaped = text[1];
                text.remove_prefix(2);
                if (escaped == 'u') {
                    if (text.size() < 4)
                        return std::nullopt;
                    unsigned code = 0;
                    std::from_chars(text.data(), text.data() + 4, code, 16);
                    text.remove_prefix(4);
                    appendUtf8(result, code);
                    continue;
                }
                result += escaped == 'n' ? '\n' : escaped == 't' ? '\t' : escaped == 'r' ? '\r' : escaped == 'b' ? '\b' : escaped == 'f' ? '\f' : escaped;
            }
            if (!take("\""))
                return std::nullopt;
            return result;
        }

        static void appendUtf8(std::string& out, unsigned code) {
            if (code < 0x80) {
                out += static_cast<char>(code);
            } else if (code < 0x800) {
                out += static_cast<char>(0xC0 | (code >> 6));
                out += static_cast<char>(0x80 | (code & 0x3F));
            } else {
                out += static_cast<char>(0xE0 | (code >> 12));
                out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                out += static_cast<char>(0x80 | (code & 0x3F));
            }
        }

        std::optional<Json> readValue() {
            skipSpace();
            if (text.empty())
                return std::nullopt;
            if (text[0] == '{')
                return readObject();
            if (text[0] == '[')
                return readArray();
            if (text[0] == '"') {
                std::optional<std::string> string = readString();
                return string ? std::optional<Json>(Json(std::move(*string))) : std::nullopt;
            }
            if (take("true"))
                return Json(true);
            if (take("false"))
                return Json(false);
            if (take("null"))
                return Json();
            double number = 0.0;
            std::from_chars_result read = std::from_chars(text.data(), text.data() + text.size(), number);
            if (read.ec != std::errc())
                return std::nullopt;
            text.remove_prefix(static_cast<std::size_t>(read.ptr - text.data()));
            return Json(number);
        }

        std::optional<Json> readArray() {
            take("[");
            Array array;
            if (take("]"))
                return Json(std::move(array));
            do {
                std::optional<Json> item = readValue();
                if (!item)
                    return std::nullopt;
                array.push_back(std::move(*item));
            } while (take(","));
            return take("]") ? std::optional<Json>(Json(std::move(array))) : std::nullopt;
        }

        std::optional<Json> readObject() {
            take("{");
            Object object;
            if (take("}"))
                return Json(std::move(object));
            do {
                std::optional<std::string> key = readString();
                if (!key || !take(":"))
                    return std::nullopt;
                std::optional<Json> item = readValue();
                if (!item)
                    return std::nullopt;
                object.emplace_back(std::move(*key), std::move(*item));
            } while (take(","));
            return take("}") ? std::optional<Json>(Json(std::move(object))) : std::nullopt;
        }
    };

    std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value = nullptr;
};

}
