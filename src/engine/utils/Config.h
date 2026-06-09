#pragma once

#include <inicpp.h>
#include <ranges>
#include <string>
#include <string_view>

#include <ErrorCodes.h>

namespace Aeon {

/**
 * @brief A simple configuration mapper that can only parse INI file.
 */
class ConfigMap {
public:
    ConfigMap(std::string path) { m_iniFile.load(path); }

    template<typename T>
    T Load() {
        T val{};
        val.deserialize(m_iniFile);
        LOG_INFO("Loaded configurations for {}", val.print());
        return val;
    }

    template<typename T>
    void Save(const T &val) {
        val.serialize(m_iniFile);
    }

    void WriteTo(const std::string &path) { m_iniFile.save(path); }

private:
    ini::IniFile m_iniFile;
};

} // namespace Aeon

/*  ---------------- SERIALIZER/DESERIALIZER MACRO -------------------- */

#define _AEON_INIFILE_WRITE(SECTION, FIELD) ini[#SECTION][#FIELD] = FIELD;
#define _AEON_INIFILE_READ(SECTION, FIELD) FIELD = ini[#SECTION][#FIELD].as<decltype(FIELD)>();
#define _AEON_PRINT_FIELD(SECTION, FIELD) ss << "\n\t" << #FIELD " = " << FIELD;

#define DEFINE_CONFIG_MAP_FIELDS(TYPE_NAME, ...)                                                                       \
    void serialize(ini::IniFile &ini) const { AEON_FOR_EACH(_AEON_INIFILE_WRITE, TYPE_NAME, __VA_ARGS__) }             \
    void deserialize(ini::IniFile &ini){AEON_FOR_EACH(_AEON_INIFILE_READ, TYPE_NAME, __VA_ARGS__)} std::string print() \
        const {                                                                                                        \
        std::ostringstream ss;                                                                                         \
        ss << #TYPE_NAME;                                                                                      \
        AEON_FOR_EACH(_AEON_PRINT_FIELD, TYPE_NAME, __VA_ARGS__)                                                       \
        return ss.str();                                                                                               \
    }
