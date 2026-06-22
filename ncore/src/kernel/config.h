#pragma once

#include <string>
#include <unordered_map>

#include <ncore/kernel/types.h>

namespace ncore {

/**
 * @brief Represents a simple keyval config file.
 * Can only parse INI files lol
 */
class ConfFile {
public:
    ConfFile() = default;
    ConfFile(std::string path) { load(path); }

    void operator()(const std::string &path) { load(path); }
    std::string operator[](const std::string &key) const { return get(key); }

    void load(const std::string &path);
    std::string get(const std::string &key, const std::string &default_value = "") const;
    void save();

    template<typename T>
    T read() {
        T result{};
        auto *type_info = rfl::Registry::find_record<T>();
        if (type_info) {
            read_into(*type_info, &result);
        }
        return result;
    }

private:
    /**
	* @brief Decodes data to a type-safe struct.
	*/
    void read_into(const rfl::RecordInfo &type_info, void *result);

    std::string path;
    std::unordered_map<std::string, std::string> data;
};

} // namespace ncore
