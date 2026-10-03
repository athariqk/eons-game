#include "truetype_font_compiler.h"

#include <fstream>

#include <ncore/resources/resource_archive.h>
#include <ncore/utils/log.h>

namespace nc {

StringView TrueTypeFontCompiler::get_format_name() const
{
    return "Font";
}

bool TrueTypeFontCompiler::is_handling_extension( const String& ext )
{
    return ext == ".ttf" || ext == ".otf";
}

Error TrueTypeFontCompiler::compile( StringView p_input, StringView p_output, AssetImportContext ctx )
{
    // Fonts are stored as an opaque TTF/OTF blob; the consumer (e.g. ImGui)
    // parses the table structure itself, so no glyph preprocessing happens here.
    std::ifstream in( std::string( p_input ), std::ios::binary | std::ios::ate );
    if (!in) {
        NC_LOG_ERROR_C( log::IO, "Failed opening font file '{}'", p_input );
        return Error::FAIL;
    }

    auto end_pos = in.tellg();
    auto size    = static_cast<size_t>( static_cast<std::streamoff>( end_pos ) );
    if (size == 0) {
        NC_LOG_ERROR_C( log::IO, "Font file '{}' is empty", p_input );
        return Error::FAIL;
    }

    BytesBuffer bytes( size );
    in.seekg( 0, std::ios::beg );
    if (!in.read( reinterpret_cast<char*>( bytes.data() ), static_cast<std::streamsize>( size ) )) {
        NC_LOG_ERROR_C( log::IO, "Failed reading font file '{}'", p_input );
        return Error::FAIL;
    }

    auto result = Ref<Font>::create();
    result->set_data( bytes.data(), size );

    ResourceArchive archive;
    return archive.serialize( result, p_output.data() );
}

} // namespace nc
