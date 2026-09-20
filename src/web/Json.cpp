#include "web/Json.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace web {
namespace {

void skipWs(const std::string& text, std::size_t& pos) {
    while (pos < text.size()) {
        char c = text[pos];
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            ++pos;
        } else {
            break;
        }
    }
}

bool parseValue(const std::string& text, std::size_t& pos, Json& out, int depth);

bool parseString(const std::string& text, std::size_t& pos, std::string& out) {
    if (pos >= text.size() || text[pos] != '"') return false;
    ++pos;
    out.clear();
    while (pos < text.size()) {
        char c = text[pos];
        if (c == '"') {
            ++pos;
            return true;
        }
        if (c == '\\') {
            ++pos;
            if (pos >= text.size()) return false;
            char e = text[pos];
            switch (e) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    // \uXXXX: decode the BMP plane; surrogate pairs map to '?'.
                    if (pos + 4 >= text.size()) return false;
                    unsigned code = 0;
                    for (int i = 1; i <= 4; ++i) {
                        char h = text[pos + i];
                        code <<= 4;
                        if (h >= '0' && h <= '9') {
                            code |= static_cast<unsigned>(h - '0');
                        } else if (h >= 'a' && h <= 'f') {
                            code |= static_cast<unsigned>(h - 'a' + 10);
                        } else if (h >= 'A' && h <= 'F') {
                            code |= static_cast<unsigned>(h - 'A' + 10);
                        } else {
                            return false;
                        }
                    }
                    if (code < 0x80) {
                        out.push_back(static_cast<char>(code));
                    } else if (code < 0x800) {
                        out.push_back(static_cast<char>(0xC0 | (code >> 6)));
                        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
                    } else {
                        // Surrogates and 3-byte sequences: emit '?' placeholder.
                        out.push_back('?');
                    }
                    pos += 4;
                    break;
                }
                default:
                    return false;
            }
            ++pos;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            return false;  // raw control character inside a string
        } else {
            out.push_back(c);
            ++pos;
        }
    }
    return false;  // unterminated string
}

bool parseNumber(const std::string& text, std::size_t& pos, double& out) {
    std::size_t start = pos;
    if (pos < text.size() && text[pos] == '-') ++pos;
    bool digits = false;
    while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
        ++pos;
        digits = true;
    }
    if (pos < text.size() && text[pos] == '.') {
        ++pos;
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
            ++pos;
            digits = true;
        }
    }
    if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E')) {
        ++pos;
        if (pos < text.size() && (text[pos] == '+' || text[pos] == '-')) ++pos;
        while (pos < text.size() && text[pos] >= '0' && text[pos] <= '9') ++pos;
    }
    if (!digits) return false;
    out = std::strtod(text.substr(start, pos - start).c_str(), nullptr);
    return true;
}

bool parseValue(const std::string& text, std::size_t& pos, Json& out, int depth) {
    if (depth > 32) return false;
    skipWs(text, pos);
    if (pos >= text.size()) return false;
    char c = text[pos];
    if (c == '{') {
        out = Json::makeObject();
        ++pos;
        skipWs(text, pos);
        if (pos < text.size() && text[pos] == '}') {
            ++pos;
            return true;
        }
        while (true) {
            skipWs(text, pos);
            std::string key;
            if (!parseString(text, pos, key)) return false;
            skipWs(text, pos);
            if (pos >= text.size() || text[pos] != ':') return false;
            ++pos;
            Json value;
            if (!parseValue(text, pos, value, depth + 1)) return false;
            out.members.put(key, value);
            skipWs(text, pos);
            if (pos < text.size() && text[pos] == ',') {
                ++pos;
                continue;
            }
            if (pos < text.size() && text[pos] == '}') {
                ++pos;
                return true;
            }
            return false;
        }
    }
    if (c == '[') {
        out = Json::makeArray();
        ++pos;
        skipWs(text, pos);
        if (pos < text.size() && text[pos] == ']') {
            ++pos;
            return true;
        }
        while (true) {
            Json value;
            if (!parseValue(text, pos, value, depth + 1)) return false;
            out.items.pushBack(value);
            skipWs(text, pos);
            if (pos < text.size() && text[pos] == ',') {
                ++pos;
                continue;
            }
            if (pos < text.size() && text[pos] == ']') {
                ++pos;
                return true;
            }
            return false;
        }
    }
    if (c == '"') return parseString(text, pos, out.str) ? (out.type = Json::Type::String, true) : false;
    if (c == 't') {
        if (text.compare(pos, 4, "true") == 0) {
            pos += 4;
            out.type = Json::Type::Bool;
            out.boolean = true;
            return true;
        }
        return false;
    }
    if (c == 'f') {
        if (text.compare(pos, 5, "false") == 0) {
            pos += 5;
            out.type = Json::Type::Bool;
            out.boolean = false;
            return true;
        }
        return false;
    }
    if (c == 'n') {
        if (text.compare(pos, 4, "null") == 0) {
            pos += 4;
            out.type = Json::Type::Null;
            return true;
        }
        return false;
    }
    if (c == '-' || (c >= '0' && c <= '9')) {
        out.type = Json::Type::Number;
        return parseNumber(text, pos, out.number);
    }
    return false;
}

}  // namespace

Json Json::makeString(const std::string& value) {
    Json v;
    v.type = Type::String;
    v.str = value;
    return v;
}

Json Json::makeNumber(double value) {
    Json v;
    v.type = Type::Number;
    v.number = value;
    return v;
}

Json Json::makeBool(bool value) {
    Json v;
    v.type = Type::Bool;
    v.boolean = value;
    return v;
}

Json Json::makeArray() {
    Json v;
    v.type = Type::Array;
    return v;
}

Json Json::makeObject() {
    Json v;
    v.type = Type::Object;
    return v;
}

std::string Json::escape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (unsigned char c : text) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
        }
    }
    return out;
}

std::string Json::numberLiteral(int value) { return std::to_string(value); }

std::string Json::dump() const {
    switch (type) {
        case Type::Null: return "null";
        case Type::Bool: return boolean ? "true" : "false";
        case Type::Number: {
            if (std::floor(number) == number && std::fabs(number) < 1e15) {
                return std::to_string(static_cast<long long>(number));
            }
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.6g", number);
            return buf;
        }
        case Type::String: return "\"" + escape(str) + "\"";
        case Type::Array: {
            std::string out = "[";
            for (std::size_t i = 0; i < items.size(); ++i) {
                if (i > 0) out += ",";
                out += items[i].dump();
            }
            out += "]";
            return out;
        }
        case Type::Object: {
            std::string out = "{";
            bool first = true;
            for (const std::string& key : members.keys()) {
                if (!first) out += ",";
                first = false;
                out += "\"" + escape(key) + "\":" + members.find(key)->dump();
            }
            out += "}";
            return out;
        }
    }
    return "null";
}

bool Json::parse(const std::string& text, Json& out) {
    std::size_t pos = 0;
    Json value;
    if (!parseValue(text, pos, value, 0)) return false;
    skipWs(text, pos);
    if (pos != text.size()) return false;
    out = std::move(value);
    return true;
}

const Json* Json::get(const std::string& key) const {
    if (type != Type::Object) return nullptr;
    return members.find(key);
}

std::string Json::getString(const std::string& key, const std::string& fallback) const {
    const Json* v = get(key);
    return (v != nullptr && v->type == Type::String) ? v->str : fallback;
}

int Json::getInt(const std::string& key, int fallback) const {
    const Json* v = get(key);
    return (v != nullptr && v->type == Type::Number) ? static_cast<int>(v->number) : fallback;
}

double Json::getNumber(const std::string& key, double fallback) const {
    const Json* v = get(key);
    return (v != nullptr && v->type == Type::Number) ? v->number : fallback;
}

bool Json::getBool(const std::string& key, bool fallback) const {
    const Json* v = get(key);
    return (v != nullptr && v->type == Type::Bool) ? v->boolean : fallback;
}

}  // namespace web
