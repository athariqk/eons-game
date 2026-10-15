#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

#include <inicpp.h>

#include <ncore/utils/config.h>

namespace nc {

namespace {

// Parses up to `max_count` comma/space separated floats. Returns the number parsed.
int parse_floats_( const String& value, float* out, int max_count )
{
    std::string normalized;
    normalized.reserve( value.size() );
    for (char ch : value)
        normalized.push_back( ch == ',' ? ' ' : ch );

    std::istringstream stream( normalized );
    int count = 0;
    for (; count < max_count; ++count) {
        if (!( stream >> out[count] ))
            break;
    }
    return count;
}

} // namespace

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
        summary += "\n    " + pair.first + " = " + pair.second;
    } );
    NC_LOG_INFO( "Config file \"{}\" loaded: {}", p_path, summary );
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

bool ConfFile::has( const String& key ) const
{
    return data.find( key ) != data.end();
}

float ConfFile::get_float( const String& key, float default_value ) const
{
    auto it = data.find( key );
    if (it == data.end() || it->second.empty())
        return default_value;
    return std::strtof( it->second.c_str(), nullptr );
}

int ConfFile::get_int( const String& key, int default_value ) const
{
    auto it = data.find( key );
    if (it == data.end() || it->second.empty())
        return default_value;
    return static_cast<int>( std::strtol( it->second.c_str(), nullptr, 10 ) );
}

bool ConfFile::get_bool( const String& key, bool default_value ) const
{
    auto it = data.find( key );
    if (it == data.end())
        return default_value;

    String value = it->second;
    std::transform( value.begin(), value.end(), value.begin(), []( unsigned char c ) {
        return static_cast<char>( std::tolower( c ) );
    } );

    if (value == "1" || value == "true" || value == "yes" || value == "on")
        return true;
    if (value == "0" || value == "false" || value == "no" || value == "off")
        return false;
    return default_value;
}

Vec3f ConfFile::get_vec3( const String& key, Vec3f default_value ) const
{
    auto it = data.find( key );
    if (it == data.end())
        return default_value;

    float out[3] = { default_value.x, default_value.y, default_value.z };
    if (parse_floats_( it->second, out, 3 ) < 3)
        return default_value;
    return Vec3f( out[0], out[1], out[2] );
}

Color ConfFile::get_color( const String& key, Color default_value ) const
{
    auto it = data.find( key );
    if (it == data.end())
        return default_value;

    float rgba[4] = { 0, 0, 0, 255 };
    if (parse_floats_( it->second, rgba, 4 ) < 3)
        return default_value;
    return Color(
        static_cast<uint8_t>( rgba[0] ), static_cast<uint8_t>( rgba[1] ), static_cast<uint8_t>( rgba[2] ),
        static_cast<uint8_t>( rgba[3] )
    );
}

} // namespace nc
