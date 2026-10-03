#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/types.h>

namespace nc {

/**
 * @brief Represents a simple keyval config file.
 * Can only parse INI files lol
 */
class ConfFile {
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
    [[noreturn]] void save();

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
