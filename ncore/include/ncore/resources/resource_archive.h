#pragma once

#include <ncore/core/errors.h>

#include "resource.h"

namespace nc {

/**
 * @brief On-disk header of a compiled resource file (.bin).
 *
 * Layout (little-endian, 16 bytes):
 *   [format_id : u32 FourCC] [version : u32] [size_bytes : u64] [payload...]
 *
 * The payload is the RTTI visitation of the concrete resource type
 * (ResourceFormatID dispatches to the concrete factory in deserialize()).
 */
struct REFLECT NCAPI ResourceHeader {
    REFLECT ResourceFormatID format_id;
    REFLECT uint32_t version  = 0;
    REFLECT size_t size_bytes = 0;
};

class NCAPI ResourceArchive : public Object {
    NCLASS( ResourceArchive, Object )

public:
    /**
     * @brief Serialize a resource to a binary file at output_path.
     *
     * Writes the ResourceHeader followed by the resource's concrete RTTI
     * fields (dispatched via get_class_info(), so subclasses serialize their
     * own members).
     * @return Error::OK on success, an error code on failure.
     */
    Error serialize( const Ref<IResource>& p_resource, const String& p_output_path ) const;

    /**
     * @brief Deserialize a resource from a binary file at input_path.
     * Dispatches on the header's FourCC to construct the concrete resource
     * type, then fills it via RTTI visitation and on_deserialized().
     * @return The loaded resource, or nullptr on failure.
     */
    static Ref<IResource> deserialize( const String& input_path );

private:
    String base_directory = "assets";
};

} // namespace nc
