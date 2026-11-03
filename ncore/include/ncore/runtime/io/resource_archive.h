// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: resource_archive.h

#pragma once

#include <ncore/core/errors.h>

#include "resource.h"

namespace nc {

class ResourceLoader;

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
    ResourceArchive() = default;

    /**
     * @brief Bind a loader for Ref<T> fields.
     *
     * With a loader, ref fields are written as an asset path (preferred) or a
     * loader RID, and resolved back through it on read. Without one (offline
     * asset compilers) only paths are written; anything else persists as null.
     */
    explicit ResourceArchive( ResourceLoader* p_loader ) : loader( p_loader ) {}

    /**
     * @brief Serialize a resource to a binary file at output_path.
     *
	 * Binary format: [4-byte magic] [4-byte version] [4-byte payload size] [payload...]
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
     *
     * @param p_loader Resolves Ref<T> fields read from the payload. Pass the
     *        scene's ResourceLoader in runtime code; null leaves refs empty.
     * @return The loaded resource, or nullptr on failure.
     */
    static Ref<IResource> deserialize( const String& input_path, ResourceLoader* p_loader = nullptr );

private:
    ResourceLoader* loader = nullptr;
    String base_directory  = "assets";
};

} // namespace nc
