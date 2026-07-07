#include <ncore/modules/resource/resource.h>
#include <ncore/utils/four_cc.h>

namespace nc {

ResourceFormatID::ResourceFormatID( const std::string& ascii_id )
{
    id = FourCC::from_string( ascii_id );
}

bool ResourceFormatID::is_valid()
{
    return id != 0 && FourCC::is_valid_fourcc( id );
}

std::string ResourceFormatID::to_string()
{
    return is_valid() ? FourCC::to_string( id ) : std::string();
}

} // namespace nc
