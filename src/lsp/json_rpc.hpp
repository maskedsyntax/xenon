#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>
#include <stdexcept>

namespace xenon::lsp {

// Minimal JSON value type for LSP communication
struct JsonValue;
using JsonNull = std::monostate;
using JsonBool = bool;
using JsonInt = int64_t;
using JsonFloat = double;
using JsonString = std::string;
using JsonArray = std::vector<JsonValue>;
using JsonObject = std::unordered_map<std::string, JsonValue>;

struct JsonValue {
    std::variant<JsonNull, JsonBool, JsonInt, JsonFloat, JsonString,
                 std::shared_ptr<JsonArray>, std::shared_ptr<JsonObject>> value;

    JsonValue() : value(JsonNull{}) {}
    JsonValue(bool b) : value(b) {}
    JsonValue(int64_t i) : value(i) {}
    JsonValue(int i) : value(static_cast<int64_t>(i)) {}
    JsonValue(double d) : value(d) {}
    JsonValue(const std::string& s) : value(s) {}
    JsonValue(std::string&& s) : value(std::move(s)) {}
    JsonValue(const char* s) : value(std::string(s)) {}
    JsonValue(JsonArray arr) : value(std::make_shared<JsonArray>(std::move(arr))) {}
    JsonValue(JsonObject obj) : value(std::make_shared<JsonObject>(std::move(obj))) {}

    bool isNull() const { return std::holds_alternative<JsonNull>(value); }
    bool isBool() const { return std::holds_alternative<JsonBool>(value); }
    bool isInt() const { return std::holds_alternative<JsonInt>(value); }
    bool isFloat() const { return std::holds_alternative<JsonFloat>(value); }
    bool isString() const { return std::holds_alternative<JsonString>(value); }
    bool isArray() const { return std::holds_alternative<std::shared_ptr<JsonArray>>(value); }
    bool isObject() const { return std::holds_alternative<std::shared_ptr<JsonObject>>(value); }

    bool asBool() const { return std::get<JsonBool>(value); }
    int64_t asInt() const { return std::get<JsonInt>(value); }
    double asFloat() const { return std::get<JsonFloat>(value); }
    const std::string& asString() const { return std::get<JsonString>(value); }
    const JsonArray& asArray() const { return *std::get<std::shared_ptr<JsonArray>>(value); }
    const JsonObject& asObject() const { return *std::get<std::shared_ptr<JsonObject>>(value); }
    JsonObject& asObject() { return *std::get<std::shared_ptr<JsonObject>>(value); }
    JsonArray& asArray() { return *std::get<std::shared_ptr<JsonArray>>(value); }

    // Access object member, returns null JsonValue if missing
    const JsonValue& operator[](const std::string& key) const {
        static const JsonValue null_val;
        if (!isObject()) return null_val;
        auto& obj = asObject();
        auto it = obj.find(key);
        if (it == obj.end()) return null_val;
        return it->second;
    }

    JsonValue& operator[](const std::string& key) {
        if (!isObject()) {
            value = std::make_shared<JsonObject>();
        }
        return (*std::get<std::shared_ptr<JsonObject>>(value))[key];
    }

    bool has(const std::string& key) const {
        if (!isObject()) return false;
        return asObject().count(key) > 0;
    }
};

// JSON serializer / deserializer
std::string jsonSerialize(const JsonValue& v);
JsonValue jsonParse(const std::string& text);

// Build a JSON-RPC request message (with id)
std::string buildRequest(int id, const std::string& method, const JsonValue& params);

// Build a JSON-RPC notification message (no id)
std::string buildNotification(const std::string& method, const JsonValue& params);

// Wrap with Content-Length header for LSP transport
std::string lspEncode(const std::string& json);

} // namespace xenon::lsp
