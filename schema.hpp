#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

/**
 * @file schema.hpp
 * @brief Type-safe recursive schema validation for Socks JSON messages.
 *
 * This module provides utilities to define expected JSON structures and
 * automatically validate incoming requests. Supports nested objects,
 * multiple allowed types per key, and detailed error reporting.
 */

namespace Socks {

using json = nlohmann::json;

/**
 * @brief Represents a single schema entry for a JSON key.
 *
 * Each schema entry can be:
 * - A single JSON type (e.g., json::value_t::string)
 * - A list of allowed JSON types (e.g., [json::value_t::number_integer,
 * json::value_t::number_float])
 * - A nested object with further key validations (recursive ParamSchemaMap)
 */
struct ParamSchema {
    /// @brief The type rule: single type, multiple types, or nested map.
    std::variant<json::value_t, std::vector<json::value_t>,
                 std::shared_ptr<std::unordered_map<std::string, ParamSchema>>>
        value;

    /**
     * @brief Construct a ParamSchema for a single expected type.
     * @param type The expected JSON type (e.g., json::value_t::string).
     */
    ParamSchema(json::value_t type);

    /**
     * @brief Construct a ParamSchema allowing multiple possible types.
     * @param types A list of allowed JSON types.
     */
    ParamSchema(std::vector<json::value_t> types);

    /**
     * @brief Construct a nested ParamSchema from a key-to-schema map.
     * @param map The nested validation schema.
     */
    ParamSchema(const std::unordered_map<std::string, ParamSchema> &map);

    /**
     * @brief Construct a nested ParamSchema from an rvalue key-to-schema map.
     * @param map The nested validation schema (moved).
     */
    ParamSchema(std::unordered_map<std::string, ParamSchema> &&map);
};

/**
 * @brief Defines the schema for an entire JSON object.
 *
 * Maps field names to ParamSchema rules.
 */
using ParamSchemaMap = std::unordered_map<std::string, ParamSchema>;

/**
 * @brief Validates a JSON object against a specified schema.
 *
 * Recursively checks that:
 * - Required keys exist
 * - Keys match allowed types
 * - Nested objects conform to nested schemas
 *
 * Throws a detailed std::runtime_error on first mismatch.
 *
 * @param obj The JSON object to validate.
 * @param schema The expected structure and type rules.
 * @throws std::runtime_error on schema mismatch (missing key, wrong type, wrong
 * nesting).
 */
void assert_parameters(const json &obj, const ParamSchemaMap &schema);

/**
 * @brief Helper function to create a ParamSchema that accepts multiple JSON
 * types.
 *
 * This function simplifies creating schemas where a field can be of several
 * types, for example, accepting both integer and float numbers.
 *
 * Example usage:
 * @code
 * assert_parameters(req, {
 *   {"n", types({json::value_t::number_integer, json::value_t::number_float})}
 * });
 * @endcode
 *
 * @param allowed A list of allowed JSON types.
 * @return ParamSchema that matches any of the provided types.
 */
inline ParamSchema types(std::initializer_list<json::value_t> allowed) {
    return ParamSchema(std::vector<json::value_t>(allowed));
}

} // namespace Socks
