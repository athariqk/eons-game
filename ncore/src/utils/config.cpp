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

    String summary;
    std::for_each( data.begin(), data.end(), [&summary]( const std::pair<String, String>& pair ) {
        summary += "\n" + pair.first + " = " + pair.second;
    } );
    NC_LOG_INFO( "Config file \"{}\" loaded, summary: {}", p_path, summary );
}

void ConfFile::save()
{
    throw std::exception( "ConfFile::save() is not implemented." );
}

void ConfFile::read_into( const rtti::RecordInfo& type_info, void* result )
{
    for (auto& field : type_info.fields()) {
        std::string short_name( type_info.name );
        if (auto pos = short_name.rfind( "::" ); pos != std::string::npos)
            short_name = short_name.substr( pos + 2 );
        auto qualified_name = short_name + "." + field.name.data();
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
            case rtti::TypeKind::ENUM: {
                auto enum_t = static_cast<const rtti::EnumInfo*>( type );
                // cast raw enum value to big enough storage such as longs
                // so we can avoid possible underflowing
                if (enum_t->is_unsigned) {
                    ini::Convert<unsigned long> c;
                    c.decode( it->second.c_str(), *field.get_ptr<unsigned long>( result ) );
                } else {
                    ini::Convert<long> c;
                    c.decode( it->second.c_str(), *field.get_ptr<long>( result ) );
                }
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
