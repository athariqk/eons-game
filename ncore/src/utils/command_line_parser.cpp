// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <iostream>

#include <ncore/utils/command_line_parser.h>

namespace nc {

//-------------------------------------------------------------------------
// Helpers
//-------------------------------------------------------------------------

String CommandLineParser::to_lower( StringView s )
{
    String out( s );
    std::transform( out.begin(), out.end(), out.begin(), []( unsigned char c ) {
        return static_cast<char>( std::tolower( c ) );
    } );
    return out;
}

template<typename T>
CommandLineParser::Arg<T>* CommandLineParser::find_arg( DynamicArray<Arg<T>>& args, StringView name )
{
    const String key = to_lower( name );
    for (auto& arg : args) {
        if (arg.name == key) {
            return &arg;
        }
    }
    return nullptr;
}

template<typename T>
const CommandLineParser::Arg<T>* CommandLineParser::find_arg( const DynamicArray<Arg<T>>& args, StringView name ) const
{
    const String key = to_lower( name );
    for (const auto& arg : args) {
        if (arg.name == key) {
            return &arg;
        }
    }
    return nullptr;
}

bool CommandLineParser::is_unique_name( StringView name ) const
{
    return find_arg( m_bool_args, name ) == nullptr && find_arg( m_int_args, name ) == nullptr &&
           find_arg( m_float_args, name ) == nullptr && find_arg( m_string_args, name ) == nullptr;
}

template<typename T>
void CommandLineParser::add_arg(
    DynamicArray<Arg<T>>& args, StringView name, StringView description, const T& default_value, bool is_required
)
{
    Arg<T> arg;
    arg.name          = to_lower( name );
    arg.description   = String( description );
    arg.is_required   = is_required;
    arg.was_parsed    = false;
    arg.default_value = default_value;
    arg.value         = default_value;
    args.push_back( std::move( arg ) );
}

//-------------------------------------------------------------------------
// Registration
//-------------------------------------------------------------------------

void CommandLineParser::add_required_bool( StringView name, StringView description )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_bool_args, name, description, false, true );
}

void CommandLineParser::add_optional_bool( StringView name, StringView description, bool default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_bool_args, name, description, default_value, false );
}

void CommandLineParser::add_required_int( StringView name, StringView description, int64_t default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_int_args, name, description, default_value, true );
}

void CommandLineParser::add_optional_int( StringView name, StringView description, int64_t default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_int_args, name, description, default_value, false );
}

void CommandLineParser::add_required_float( StringView name, StringView description, float default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_float_args, name, description, default_value, true );
}

void CommandLineParser::add_optional_float( StringView name, StringView description, float default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_float_args, name, description, default_value, false );
}

void CommandLineParser::add_required_string( StringView name, StringView description, StringView default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_string_args, name, description, String( default_value ), true );
}

void CommandLineParser::add_optional_string( StringView name, StringView description, StringView default_value )
{
    if (!is_unique_name( name )) {
        return;
    }
    add_arg( m_string_args, name, description, String( default_value ), false );
}

//-------------------------------------------------------------------------
// Parse helpers
//-------------------------------------------------------------------------

void CommandLineParser::clear()
{
    m_bool_args.clear();
    m_int_args.clear();
    m_float_args.clear();
    m_string_args.clear();
    m_error_msg.clear();
}

bool CommandLineParser::read_next_value( int argc, char* argv[], int& i, String& out_value )
{
    if (i + 1 >= argc) {
        m_error_msg = "Missing value for argument: ";
        m_error_msg += argv[i];
        return false;
    }

    ++i;
    out_value = argv[i];
    return true;
}

template<typename T>
bool CommandLineParser::check_required( const DynamicArray<Arg<T>>& args, DynamicArray<String>& missing ) const
{
    bool ok = true;
    for (const auto& arg : args) {
        if (arg.is_required && !arg.was_parsed) {
            missing.push_back( arg.name );
            ok = false;
        }
    }
    return ok;
}

//-------------------------------------------------------------------------
// Parse
//-------------------------------------------------------------------------

bool CommandLineParser::parse( int argc, char* argv[] )
{
    m_error_msg.clear();

    for (auto& a : m_bool_args) {
        a.was_parsed = false;
        a.value      = a.default_value;
    }
    for (auto& a : m_int_args) {
        a.was_parsed = false;
        a.value      = a.default_value;
    }
    for (auto& a : m_float_args) {
        a.was_parsed = false;
        a.value      = a.default_value;
    }
    for (auto& a : m_string_args) {
        a.was_parsed = false;
        a.value      = a.default_value;
    }

    for (int i = 1; i < argc; ++i) {
        StringView token = argv[i];

        if (token.size() < 2 || token[0] != '-') {
            continue; // ignore positionals for now
        }

        const StringView raw_name = ( token[1] == '-' ) ? token.substr( 2 ) : token.substr( 1 );
        if (raw_name.empty()) {
            continue;
        }

        const String name = to_lower( raw_name );

        if (auto* arg = find_arg( m_bool_args, name )) {
            arg->value      = true;
            arg->was_parsed = true;
            continue;
        }

        if (auto* arg = find_arg( m_int_args, name )) {
            String value_str;
            if (!read_next_value( argc, argv, i, value_str )) {
                return false;
            }

            errno        = 0;
            char* end    = nullptr;
            const long v = std::strtol( value_str.c_str(), &end, 0 );
            if (errno != 0 || end == value_str.c_str() || *end != '\0') {
                m_error_msg = "Failed to parse int argument '";
                m_error_msg += name;
                m_error_msg += "' from '";
                m_error_msg += value_str;
                m_error_msg += "'";
                return false;
            }

            arg->value      = static_cast<int64_t>( v );
            arg->was_parsed = true;
            continue;
        }

        if (auto* arg = find_arg( m_float_args, name )) {
            String value_str;
            if (!read_next_value( argc, argv, i, value_str )) {
                return false;
            }

            errno         = 0;
            char* end     = nullptr;
            const float v = std::strtof( value_str.c_str(), &end );
            if (errno != 0 || end == value_str.c_str() || *end != '\0') {
                m_error_msg = "Failed to parse float argument '";
                m_error_msg += name;
                m_error_msg += "' from '";
                m_error_msg += value_str;
                m_error_msg += "'";
                return false;
            }

            arg->value      = v;
            arg->was_parsed = true;
            continue;
        }

        if (auto* arg = find_arg( m_string_args, name )) {
            String value_str;
            if (!read_next_value( argc, argv, i, value_str )) {
                return false;
            }

            arg->value      = std::move( value_str );
            arg->was_parsed = true;
            continue;
        }

        m_error_msg = "Unknown argument: ";
        m_error_msg += token;
        return false;
    }

    DynamicArray<String> missing;
    check_required( m_bool_args, missing );
    check_required( m_int_args, missing );
    check_required( m_float_args, missing );
    check_required( m_string_args, missing );

    if (!missing.empty()) {
        m_error_msg = "Missing required argument(s): ";
        for (size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) {
                m_error_msg += ", ";
            }
            m_error_msg += missing[i];
        }
        return false;
    }

    return true;
}

//-------------------------------------------------------------------------
// Accessors
//-------------------------------------------------------------------------

bool CommandLineParser::get_bool( StringView name ) const
{
    const auto* arg = find_arg( m_bool_args, name );
    if (!arg) {
        return false;
    }
    return arg->was_parsed ? arg->value : arg->default_value;
}

int64_t CommandLineParser::get_int( StringView name ) const
{
    const auto* arg = find_arg( m_int_args, name );
    if (!arg) {
        return 0;
    }
    return arg->was_parsed ? arg->value : arg->default_value;
}

float CommandLineParser::get_float( StringView name ) const
{
    const auto* arg = find_arg( m_float_args, name );
    if (!arg) {
        return 0.0f;
    }
    return arg->was_parsed ? arg->value : arg->default_value;
}

const String& CommandLineParser::get_string( StringView name ) const
{
    static const String s_empty;
    const auto* arg = find_arg( m_string_args, name );
    if (!arg) {
        return s_empty;
    }
    return arg->was_parsed ? arg->value : arg->default_value;
}

bool CommandLineParser::was_provided( StringView name ) const
{
    if (const auto* a = find_arg( m_bool_args, name )) {
        return a->was_parsed;
    }
    if (const auto* a = find_arg( m_int_args, name )) {
        return a->was_parsed;
    }
    if (const auto* a = find_arg( m_float_args, name )) {
        return a->was_parsed;
    }
    if (const auto* a = find_arg( m_string_args, name )) {
        return a->was_parsed;
    }
    return false;
}

//-------------------------------------------------------------------------
// Help
//-------------------------------------------------------------------------

void CommandLineParser::print_help( StringView app_name ) const
{
    if (!app_name.empty()) {
        std::cout << "Usage: " << app_name << " [options]\n\n";
    }

    std::cout << "Options:\n";

    auto print_list = []( const auto& args ) {
        for (const auto& arg : args) {
            std::cout << "  -" << arg.name;
            if (arg.is_required) {
                std::cout << " (required)";
            } else {
                std::cout << " (optional)";
            }
            if (!arg.description.empty()) {
                std::cout << "  " << arg.description;
            }
            std::cout << '\n';
        }
    };

    print_list( m_bool_args );
    print_list( m_int_args );
    print_list( m_float_args );
    print_list( m_string_args );
}

// Explicit instantiations
template CommandLineParser::Arg<bool>* CommandLineParser::find_arg( DynamicArray<Arg<bool>>&, StringView );
template const CommandLineParser::Arg<bool>* CommandLineParser::find_arg( const DynamicArray<Arg<bool>>&, StringView ) const;
template CommandLineParser::Arg<int64_t>* CommandLineParser::find_arg( DynamicArray<Arg<int64_t>>&, StringView );
template const CommandLineParser::Arg<int64_t>* CommandLineParser::find_arg( const DynamicArray<Arg<int64_t>>&, StringView ) const;
template CommandLineParser::Arg<float>* CommandLineParser::find_arg( DynamicArray<Arg<float>>&, StringView );
template const CommandLineParser::Arg<float>* CommandLineParser::find_arg( const DynamicArray<Arg<float>>&, StringView ) const;
template CommandLineParser::Arg<String>* CommandLineParser::find_arg( DynamicArray<Arg<String>>&, StringView );
template const CommandLineParser::Arg<String>* CommandLineParser::find_arg( const DynamicArray<Arg<String>>&, StringView ) const;

template void CommandLineParser::add_arg( DynamicArray<Arg<bool>>&, StringView, StringView, const bool&, bool );
template void CommandLineParser::add_arg( DynamicArray<Arg<int64_t>>&, StringView, StringView, const int64_t&, bool );
template void CommandLineParser::add_arg( DynamicArray<Arg<float>>&, StringView, StringView, const float&, bool );
template void CommandLineParser::add_arg( DynamicArray<Arg<String>>&, StringView, StringView, const String&, bool );

template bool CommandLineParser::check_required( const DynamicArray<Arg<bool>>&, DynamicArray<String>& ) const;
template bool CommandLineParser::check_required( const DynamicArray<Arg<int64_t>>&, DynamicArray<String>& ) const;
template bool CommandLineParser::check_required( const DynamicArray<Arg<floatfloat>>&, DynamicArray<String>& ) const;
template bool CommandLineParser::check_required( const DynamicArray<Arg<String>>&, DynamicArray<String>& ) const;

} // namespace nc
