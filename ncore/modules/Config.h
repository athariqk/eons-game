#pragma once

#include <inicpp.h>

namespace ncore {

/**
 * @brief A simple configuration mapper that can only parse INI file.
 */
class ConfigMap {
public:
    ConfigMap(std::string path) { inifile.load(path); }

    template<typename T>
    T load() {
        T val{};
        val.deserialize(inifile);
        return val;
    }

    template<typename T>
    void save(const T &val) {
        val.serialize(inifile);
    }

    void write_to(const std::string &path) { inifile.save(path); }

private:
    ini::IniFile inifile;
};

namespace cfg {

/**
 * @brief Properties related to hardware window settings
 */
struct Window {
    int SizeWidth = 800;
    int SizeHeight = 800;
    bool Fullscreen = false;
    NC_DEF_CFG_MAP(Window, SizeWidth, SizeHeight, Fullscreen)
};

struct Render {
    float PixelsPerMeter = 32.0f;
    NC_DEF_CFG_MAP(Render, PixelsPerMeter)
};

/**
 * @brief Properties related to the engine's runtime
 */
struct Runtime {
    std::string DefaultWorld;
    NC_DEF_CFG_MAP(Runtime, DefaultWorld)
};

struct Log {
    int Level = 0; // 0 = trace
    std::string FilePath = "logs/engine.log";
    std::string Overrides; // "<category:string>:<level:int>", e.g. "engine:0,phys:3"
    NC_DEF_CFG_MAP(Log, Level, FilePath, Overrides)
};

} // namespace cfg

} // namespace ncore
