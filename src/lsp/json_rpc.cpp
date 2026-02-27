#include "lsp/json_rpc.hpp"
#include <sstream>
#include <stdexcept>
#include <iomanip>
#include <cmath>

namespace xenon::lsp {

// ---- Serializer ----

static void serializeString(std::ostringstream& oss, const std::string& s) {
    oss << '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (c < 0x20) {
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(c) << std::dec;
                } else {
                    oss << static_cast<char>(c);
                }
        }
    }
    oss << '"';
}

static void serialize(std::ostringstream& oss, const JsonValue& v) {
    if (v.isNull()) {
        oss << "null";
    } else if (v.isBool()) {
        oss << (v.asBool() ? "true" : "false");
    } else if (v.isInt()) {
        oss << v.asInt();
    } else if (v.isFloat()) {
        oss << std::setprecision(15) << v.asFloat();
    } else if (v.isString()) {
        serializeString(oss, v.asString());
    } else if (v.isArray()) {
        oss << '[';
        bool first = true;
        for (const auto& elem : v.asArray()) {
            if (!first) oss << ',';
            serialize(oss, elem);
            first = false;
        }
        oss << ']';
    } else if (v.isObject()) {
        oss << '{';
        bool first = true;
        for (const auto& [key, val] : v.asObject()) {
            if (!first) oss << ',';
            serializeString(oss, key);
            oss << ':';
            serialize(oss, val);
            first = false;
        }
        oss << '}';
    }
}

std::string jsonSerialize(const JsonValue& v) {
    std::ostringstream oss;
    serialize(oss, v);
    return oss.str();
}

// ---- Parser ----

struct Parser {
    const std::string& src;
    size_t pos = 0;

    void skipWs() {
        while (pos < src.size() && std::isspace(static_cast<unsigned char>(src[pos])))
            ++pos;
    }

    char peek() const {
        if (pos >= src.size()) return '\0';
        return src[pos];
    }

    char consume() {
        if (pos >= src.size()) throw std::runtime_error("JSON: unexpected end");
        return src[pos++];
    }

    void expect(char c) {
        if (consume() != c) throw std::runtime_error(std::string("JSON: expected '") + c + "'");
    }

    std::string parseString() {
        expect('"');
        std::string result;
        while (true) {
            char c = consume();
            if (c == '"') break;
            if (c == '\\') {
                char esc = consume();
                switch (esc) {
                    case '"':  result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/':  result += '/'; break;
                    case 'b':  result += '\b'; break;
                    case 'f':  result += '\f'; break;
                    case 'n':  result += '\n'; break;
                    case 'r':  result += '\r'; break;
                    case 't':  result += '\t'; break;
                    case 'u': {
                        // Parse \uXXXX
                        std::string hex;
                        for (int i = 0; i < 4; ++i) hex += consume();
                        unsigned long cp = std::stoul(hex, nullptr, 16);
                        // Encode as UTF-8
                        if (cp < 0x80) {
                            result += static_cast<char>(cp);
                        } else if (cp < 0x800) {
                            result += static_cast<char>(0xC0 | (cp >> 6));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        } else {
                            result += static_cast<char>(0xE0 | (cp >> 12));
                            result += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                            result += static_cast<char>(0x80 | (cp & 0x3F));
                        }
                        break;
                    }
                    default: result += esc;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    JsonValue parseNumber() {
        size_t start = pos;
        bool is_float = false;
        if (peek() == '-') ++pos;
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        if (pos < src.size() && src[pos] == '.') { is_float = true; ++pos; }
        while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        if (pos < src.size() && (src[pos] == 'e' || src[pos] == 'E')) {
            is_float = true; ++pos;
            if (pos < src.size() && (src[pos] == '+' || src[pos] == '-')) ++pos;
            while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) ++pos;
        }
        std::string num_str = src.substr(start, pos - start);
        if (is_float) {
            return JsonValue(std::stod(num_str));
        }
        return JsonValue(static_cast<int64_t>(std::stoll(num_str)));
    }

    JsonValue parseValue() {
        skipWs();
        char c = peek();
        if (c == '"') {
            return JsonValue(parseString());
        } else if (c == '{') {
            return parseObject();
        } else if (c == '[') {
            return parseArray();
        } else if (c == 't') {
            pos += 4; return JsonValue(true);
        } else if (c == 'f') {
            pos += 5; return JsonValue(false);
        } else if (c == 'n') {
            pos += 4; return JsonValue();
        } else if (c == '-' || std::isdigit(static_cast<unsigned char>(c))) {
            return parseNumber();
        }
        throw std::runtime_error(std::string("JSON: unexpected char '") + c + "'");
    }

    JsonValue parseObject() {
        expect('{');
        JsonObject obj;
        skipWs();
        if (peek() == '}') { ++pos; return JsonValue(std::move(obj)); }
        while (true) {
            skipWs();
            std::string key = parseString();
            skipWs();
            expect(':');
            skipWs();
            obj[key] = parseValue();
            skipWs();
            if (peek() == '}') { ++pos; break; }
            expect(',');
        }
        return JsonValue(std::move(obj));
    }

    JsonValue parseArray() {
        expect('[');
        JsonArray arr;
        skipWs();
        if (peek() == ']') { ++pos; return JsonValue(std::move(arr)); }
        while (true) {
            skipWs();
            arr.push_back(parseValue());
            skipWs();
            if (peek() == ']') { ++pos; break; }
            expect(',');
        }
        return JsonValue(std::move(arr));
    }
};

JsonValue jsonParse(const std::string& text) {
    Parser p{text};
    return p.parseValue();
}

// ---- RPC helpers ----

std::string buildRequest(int id, const std::string& method, const JsonValue& params) {
    JsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["id"] = static_cast<int64_t>(id);
    msg["method"] = method;
    msg["params"] = params;
    return jsonSerialize(JsonValue(std::move(msg)));
}

std::string buildNotification(const std::string& method, const JsonValue& params) {
    JsonObject msg;
    msg["jsonrpc"] = "2.0";
    msg["method"] = method;
    msg["params"] = params;
    return jsonSerialize(JsonValue(std::move(msg)));
}

std::string lspEncode(const std::string& json) {
    return "Content-Length: " + std::to_string(json.size()) + "\r\n\r\n" + json;
}

} // namespace xenon::lsp
