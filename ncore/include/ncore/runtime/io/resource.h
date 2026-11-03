// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: resource.h - Base class for resources

#pragma once

#include <ncore/core/collection.h>
#include <ncore/object/object_info.h>
#include <ncore/object/reference.h>
#include <ncore/structs/rid.h>
#include <ncore/types/typedb.h>

namespace nc {

class Scene;

// ------------------------------------------------------------------------------
// ResourceFormatID
// ------------------------------------------------------------------------------

/**
 * @brief ResourceFormatID uniquely identifies a resource format (e.g. image, etc).
 */
struct REFLECT NCAPI ResourceFormatID {
    REFLECT uint32_t id = 0;

    ResourceFormatID() = default;
    ResourceFormatID( StringView ascii_id );
    ResourceFormatID( const char* ascii_id );

    bool is_valid() const;
    String to_string() const;
};

// ------------------------------------------------------------------------------
// IResource
// ------------------------------------------------------------------------------

/**
 * @brief IResource is the base class for any CPU-side and
 * reference-counted objects. Resources are data containers
 * owned by the world that loaded (or constructed) them.
 *
 * A resource owns its live representation — a GPU buffer/texture, a decoded
 * buffer, a rasterized atlas — and creates/releases it through the services it
 * resolves from a ResourceContext. Consumers pull that representation with
 * prepare() at the point of use.
 */
class NCAPI IResource : public RefCounted {
    NCLASS( IResource, RefCounted )

public:
    virtual ResourceFormatID get_format_id() const;

    virtual uint32_t get_version() const;

    virtual size_t get_size_bytes() const;

    /**
     * @brief Handle of this resource's prepared representation.
     *
     * RID::INVALID when nothing has been prepared yet, or when the resource has
     * no prepared representation at all (e.g. AudioClip).
     */
    RID get_rid() const;

    /**
     * @brief Bumped every time the prepared representation is (re)created.
     *
     * Consumers cache this value to detect that a dependency changed underneath
     * them (hot reload, device reset) and rebuild their own state.
     */
    uint32_t get_revision() const;

    /**
     * @brief Materialize the live representation.
     *
     * Idempotent and main-thread only. Pulls dependencies first, so call order
     * does not matter. Also flushes pending CPU->backend updates (e.g. a dirty
     * material parameter block). Returns get_rid().
     */
    virtual RID prepare( Scene& p_scene );

    /**
     * @brief Rebuild derived (non-REFLECT) state after binary deserialization.
     *
     * Called by ResourceArchive::deserialize() once all REFLECT fields have
     * been read. Default no-op; Shader overrides it to rebuild lookups that
     * its constructors normally populate.
     */
    virtual void on_deserialized() {}

    String filepath = "generated";

protected:
    /**
     * @brief Drop the prepared representation through the owning service.
     *
     * Called from concrete destructors and before a rebuild. Safe to call after
     * the owning service shut down: implementations compare the service
     * revision they prepared against and skip the release when it moved on.
     */
    virtual void release() {}

    /**
     * @brief Record a freshly created prepared representation.
     *
     * Bumps get_revision() so consumers can detect that the representation was
     * rebuilt (hot reload, device reset, dependency change).
     */
    void set_prepared( RID p_rid, uint32_t p_service_revision );

    RID prepared_rid          = RID::INVALID;
    uint32_t revision         = 0;
    uint32_t service_revision = 0;
};

} // namespace nc
