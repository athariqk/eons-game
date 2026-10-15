#pragma once

#include <ncore.h>
#include <ncore/core/collection.h>
#include <ncore/core/color.h>
#include <ncore/core/object.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>

namespace nc {

/**
 * @brief Represents a simple keyval config file.
 * Can only parse INI files lol
 */
class NCAPI ConfFile : public Object {
    NCLASS( ConfFile, Object )

public:
    ConfFile() = default;
    ConfFile( String p_path )
    {
        load( p_path );
    }

    void operator()( const String& p_path )
    {
        load( p_path );
    }
    String operator[]( const String& key ) const
    {
        return get( key );
    }

    void load( const String& path );
    String get( const String& key, const String& default_value = "" ) const;
    bool has( const String& key ) const;
    [[noreturn]] void save();

    float get_float( const String& key, float default_value = 0.0f ) const;
    int get_int( const String& key, int default_value = 0 ) const;
    bool get_bool( const String& key, bool default_value = false ) const;
    Vec3f get_vec3( const String& key, Vec3f default_value = Vec3f() ) const;
    /**
     * @brief Parses "R,G,B[,A]" with 0-255 channels into RGBA color.
     */
    Color get_color( const String& key, Color default_value = Color() ) const;

    template<typename T>
    T read()
    {
        T result{};
        auto* type_info = rtti::TypeRegistry::find_record<T>();
        if (type_info) {
            read_into( *type_info, &result );
        }
        return result;
    }

private:
    /**
     * @brief Decodes data to a type-safe struct.
     */
    void read_into( const rtti::RecordInfo& type_info, void* result );

    String path;
    OrderedHashMap<String, String> data; // ordered by field name.
};

} // namespace nc
