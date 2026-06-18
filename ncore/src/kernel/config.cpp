#include <ncore/kernel/config.h>

#include <inicpp.h>

namespace ncore {

void ConfFile::load(const std::string &path) {
    ini::IniFile inifile(path);
    for (auto &it: inifile) {
        auto &section = it.second;
        for (auto &field_it: section) {
            auto &field = field_it.second;
            data[field_it.first] = field.as<std::string>();
        }
    }
}

void ConfFile::save() {}

void ConfFile::read_into(const rfl::ClassInfo &type_info, void *result) {
    for (auto &field: type_info.fields()) {
        auto it = data.find(field.name.data());
        if (it == data.end()) {
            continue;
        }
        auto field_ptr = field.get_void_ptr(result);
        if (field.type == rfl::get_type<int>()) {
            ini::Convert<int> c;
            c.decode(it->second, *static_cast<int *>(field_ptr));
        } else if (field.type == rfl::get_type<float>()) {
            ini::Convert<float> c;
            c.decode(it->second, *static_cast<float *>(field_ptr));
        } else if (field.type == rfl::get_type<std::string>()) {
            *static_cast<std::string *>(field_ptr) = it->second;
        }
    }
}

std::string ConfFile::get(const std::string &key, const std::string &default_value) const {
    auto it = data.find(key);
    if (it != data.end()) {
        return it->second;
    }
    return default_value;
}

} // namespace ncore
