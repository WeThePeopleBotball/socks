#include "schema.hpp"

namespace Socks {

ParamSchema::ParamSchema(json::value_t type) : value(type) {}

ParamSchema::ParamSchema(std::vector<json::value_t> types)
    : value(std::move(types)) {}

ParamSchema::ParamSchema(
    const std::unordered_map<std::string, ParamSchema> &map)
    : value(
          std::make_shared<std::unordered_map<std::string, ParamSchema>>(map)) {
}

ParamSchema::ParamSchema(std::unordered_map<std::string, ParamSchema> &&map)
    : value(std::make_shared<std::unordered_map<std::string, ParamSchema>>(
          std::move(map))) {}

static const char *value_t_to_string(json::value_t type) {
    switch (type) {
    case json::value_t::null:
        return "null";
    case json::value_t::object:
        return "object";
    case json::value_t::array:
        return "array";
    case json::value_t::string:
        return "string";
    case json::value_t::boolean:
        return "boolean";
    case json::value_t::number_integer:
        return "number_integer";
    case json::value_t::number_unsigned:
        return "number_unsigned";
    case json::value_t::number_float:
        return "number_float";
    case json::value_t::binary:
        return "binary";
    case json::value_t::discarded:
        return "discarded";
    default:
        return "unknown";
    }
}

static void assert_parameters_impl(const json &obj,
                                   const ParamSchemaMap &schema,
                                   const std::string &path = "") {
    for (const auto &[key, schema_val] : schema) {
        std::string full_path = path.empty() ? key : path + "." + key;

        if (!obj.contains(key)) {
            throw std::runtime_error("Missing key: " + full_path);
        }

        const auto &json_val = obj.at(key);

        if (std::holds_alternative<json::value_t>(schema_val.value)) {
            json::value_t expected_type =
                std::get<json::value_t>(schema_val.value);
            if (json_val.type() != expected_type) {
                throw std::runtime_error(
                    "Wrong type for key '" + full_path + "' (expected " +
                    value_t_to_string(expected_type) + ", got " +
                    value_t_to_string(json_val.type()) + ")");
            }
        } else if (std::holds_alternative<std::vector<json::value_t>>(
                       schema_val.value)) {
            const auto &expected_types =
                std::get<std::vector<json::value_t>>(schema_val.value);
            bool match = false;
            for (auto type : expected_types) {
                if (json_val.type() == type) {
                    match = true;
                    break;
                }
            }
            if (!match) {
                std::string expected_list;
                for (size_t i = 0; i < expected_types.size(); ++i) {
                    if (i > 0)
                        expected_list += ", ";
                    expected_list += value_t_to_string(expected_types[i]);
                }
                throw std::runtime_error(
                    "Wrong type for key '" + full_path +
                    "' (expected one of [" + expected_list + "], got " +
                    value_t_to_string(json_val.type()) + ")");
            }
        } else {
            if (!json_val.is_object()) {
                throw std::runtime_error("Expected object at key: " +
                                         full_path);
            }
            const auto &nested = *std::get<
                std::shared_ptr<std::unordered_map<std::string, ParamSchema>>>(
                schema_val.value);
            assert_parameters_impl(json_val, nested, full_path);
        }
    }
}

void assert_parameters(const json &obj, const ParamSchemaMap &schema) {
    if (!obj.is_object()) {
        throw std::runtime_error("Top-level JSON must be an object.");
    }
    assert_parameters_impl(obj, schema);
}

} // namespace Socks
