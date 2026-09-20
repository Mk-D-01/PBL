#pragma once

#include <string>

#include "data_structures/DynamicArray.hpp"
#include "data_structures/HashMap.hpp"

namespace web {

// Json: minimal hand-rolled JSON value + parser + writer helpers.
// Supports the whole JSON grammar (null, bool, number, string, array, object)
// with a bounded-depth recursive-descent parser — no external dependencies,
// keeping the "custom data structures only" story of the project intact.
class Json {
public:
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolean = false;
    double number = 0.0;
    std::string str;
    ds::DynamicArray<Json> items;                    // Array payload
    ds::HashMap<std::string, Json> members;          // Object payload

    // ---- construction helpers ----
    static Json makeString(const std::string& value);
    static Json makeNumber(double value);
    static Json makeBool(bool value);
    static Json makeArray();
    static Json makeObject();

    // ---- serialization ----
    // Serializes this value into compact JSON text.
    std::string dump() const;
    // Returns the value as a JSON string literal (quoted + escaped).
    static std::string escape(const std::string& text);
    // Formats an integer as a JSON number literal.
    static std::string numberLiteral(int value);

    // ---- parsing ----
    // Parses JSON text; returns false on malformed input (value is then Null).
    static bool parse(const std::string& text, Json& out);

    // ---- object convenience accessors ----
    const Json* get(const std::string& key) const;    // nullptr when absent
    std::string getString(const std::string& key, const std::string& fallback = "") const;
    int getInt(const std::string& key, int fallback = 0) const;
    double getNumber(const std::string& key, double fallback = 0.0) const;
    bool getBool(const std::string& key, bool fallback = false) const;
};

}  // namespace web
