// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <cstdint>

#include <ncore.h>
#include <ncore/core/collection.h>

namespace nc {

/**
 * @brief Simple command-line argument parser (Esoterica-inspired).
 *
 * Supports:
 *   - bool flags:     -verbose / --verbose
 *   - int values:     -width 1920
 *   - float values:   -scale 1.5
 *   - string values:  -map data/test.map
 *
 * Names are case-insensitive. Both -name and --name are accepted.
 */
class NCAPI CommandLineParser {
public:
    CommandLineParser() = default;

    //---------------------------------------------------------------------
    // Registration
    //---------------------------------------------------------------------

    void add_required_bool( StringView name, StringView description = {} );
    void add_optional_bool( StringView name, StringView description = {}, bool default_value = false );

    void add_required_int( StringView name, StringView description = {}, int64_t default_value = 0 );
    void add_optional_int( StringView name, StringView description = {}, int64_t default_value = 0 );

    void add_required_float( StringView name, StringView description = {}, float default_value = 0.0f );
    void add_optional_float( StringView name, StringView description = {}, float default_value = 0.0f );

    void add_required_string( StringView name, StringView description = {}, StringView default_value = {} );
    void add_optional_string( StringView name, StringView description = {}, StringView default_value = {} );

    //---------------------------------------------------------------------
    // Parse
    //---------------------------------------------------------------------

    /**
     * @brief Parse argc/argv. Returns true on success.
     * On failure, get_error_message() has details.
     */
    bool parse( int argc, char* argv[] );

    const String& get_error_message() const { return m_error_msg; }

    void clear();
    void print_help( StringView app_name = {} ) const;

    //---------------------------------------------------------------------
    // Accessors
    //---------------------------------------------------------------------

    bool get_bool( StringView name ) const;
    int64_t get_int( StringView name ) const;
    float get_float( StringView name ) const;
    const String& get_string( StringView name ) const;

    /** @brief True if the argument appeared on the command line. */
    bool was_provided( StringView name ) const;

private:
    template<typename T>
    struct Arg {
        String name; // stored lowercase
        String description;
        bool is_required = false;
        bool was_parsed  = false;
        T default_value{};
        T value{};
    };

    static String to_lower( StringView s );

    template<typename T>
    Arg<T>* find_arg( DynamicArray<Arg<T>>& args, StringView name );

    template<typename T>
    const Arg<T>* find_arg( const DynamicArray<Arg<T>>& args, StringView name ) const;

    bool is_unique_name( StringView name ) const;

    template<typename T>
    void add_arg(
        DynamicArray<Arg<T>>& args, StringView name, StringView description, const T& default_value, bool is_required
    );

    template<typename T>
    bool check_required( const DynamicArray<Arg<T>>& args, DynamicArray<String>& missing ) const;

    bool read_next_value( int argc, char* argv[], int& i, String& out_value );

private:
    DynamicArray<Arg<bool>> m_bool_args;
    DynamicArray<Arg<int64_t>> m_int_args;
    DynamicArray<Arg<float>> m_float_args;
    DynamicArray<Arg<String>> m_string_args;

    String m_error_msg;
};

} // namespace nc
