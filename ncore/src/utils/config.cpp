#include <algorithm>

#include <inicpp.h>

#include <ncore/utils/config.h>

namespace nc {

void ConfFile::load( const String& p_path )
{
    ini::IniFile inifile( p_path.c_str() );
    for (auto& it : inifile) {
        auto& section = it.second;
        for (auto& field_it : section) {
            auto& field                  = field_it.second;
            auto qualified_name          = it.first + "." + field_it.first;
            data[qualified_name.c_str()] = field.as<std::string>();
        }
    }
    inifile.clear();

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
        auto it             = data.find( qualified_name.c_str() );
        if (it == data.end()) {
            continue;
        }
        auto* type = field.get_type();
        if (!type)
            continue;
        switch (type->kind) {
            case rtti::TypeKind::BOOL: {
                ini::Convert<bool> c;
                c.decode( it->second.c_str(), *field.get_ptr<bool>( result ) );
                break;
            }
            case rtti::TypeKind::INT32: {
                ini::Convert<int> c;
                c.decode( it->second.c_str(), *field.get_ptr<int>( result ) );
                break;
            }
            case rtti::TypeKind::FLOAT: {
                ini::Convert<float> c;
                c.decode( it->second.c_str(), *field.get_ptr<float>( result ) );
                break;
            }
            case rtti::TypeKind::STRING: {
                *field.get_ptr<String>( result ) = it->second;
                break;
            }
            default:
                break;
        }
    }
}

String ConfFile::get( const String& key, const String& default_value ) const
{
    auto it = data.find( key );
    if (it != data.end()) {
        return it->second;
    }
    return default_value;
}

} // namespace nc
