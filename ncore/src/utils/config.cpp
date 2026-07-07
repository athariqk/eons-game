#include <algorithm>

#include <inicpp.h>

#include <ncore/utils/config.h>

namespace nc {

void ConfFile::load( const std::string& p_path )
{
    ini::IniFile inifile( p_path );
    for (auto& it : inifile) {
        auto& section = it.second;
        for (auto& field_it : section) {
            auto& field          = field_it.second;
            auto qualified_name  = it.first + "." + field_it.first;
            data[qualified_name] = field.as<std::string>();
        }
    }

    std::string summary;
    std::for_each( data.begin(), data.end(), [&summary]( const auto& pair ) {
        summary += pair.first + "=" + pair.second + "\n";
    } );
    NC_LOG_INFO( "Config file loaded: {}\n{}", p_path, summary );
}

void ConfFile::save() {}

void ConfFile::read_into( const rtti::RecordInfo& type_info, void* result )
{
    for (auto& field : type_info.fields()) {
        auto qualified_name = std::string( type_info.name ) + "." + field.name.data();
        auto it             = data.find( qualified_name );
        if (it == data.end()) {
            continue;
        }
        auto field_ptr = field.get_void_ptr( result );
        if (field.type_id == rtti::Registry::get_type_id<bool>()) {
            ini::Convert<bool> c;
            c.decode( it->second, *static_cast<bool*>( field_ptr ) );
        } else if (field.type_id == rtti::Registry::get_type_id<int>()) {
            ini::Convert<int> c;
            c.decode( it->second, *static_cast<int*>( field_ptr ) );
        } else if (field.type_id == rtti::Registry::get_type_id<float>()) {
            ini::Convert<float> c;
            c.decode( it->second, *static_cast<float*>( field_ptr ) );
        } else if (field.type_id == rtti::Registry::get_type_id<std::string>()) {
            *static_cast<std::string*>( field_ptr ) = it->second;
        }
    }
}

std::string ConfFile::get( const std::string& key, const std::string& default_value ) const
{
    auto it = data.find( key );
    if (it != data.end()) {
        return it->second;
    }
    return default_value;
}

} // namespace nc
