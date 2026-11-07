// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include "typedb.h"

namespace nc::types {

template<typename VecT>
struct VectorClass : public RecordInfo {
    VectorClass( const char* n, TypeId i, size_t sz, size_t align ) : RecordInfo( n, i, sz, align )
    {
        this->kind = TypeKind::VECTOR;
    }

    void resize( void* instance, size_t length ) const noexcept override
    {
        static_cast<VecT*>( instance )->resize( length );
    }

    size_t container_size( const void* instance ) const noexcept override
    {
        return static_cast<const VecT*>( instance )->size();
    }

    void* container_data( void* instance ) const noexcept override
    {
        return static_cast<VecT*>( instance )->data();
    }

    const TypeInfo* element_type() const noexcept override
    {
        return TypeDB::find<typename VecT::value_type>();
    }

    void visit( void* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        if (!instance) {
            visitor->primitive( this, nullptr );
            return;
        }

        auto* vec       = static_cast<VecT*>( instance );
        auto* elem_type = TypeDB::find<typename VecT::value_type>();

        if (!elem_type) {
            // Element type was never registered (e.g. TypeDB::initialize()
            // not run in this process). Bail out before the element loop so the
            // archive size check fails loudly instead of dereferencing null.
            NC_LOG_ERROR( "VectorClass: element type not registered for vector '{}'", this->name );
            visitor->primitive( this, instance );
            return;
        }

        visitor->array_begin( elem_type, this, vec, static_cast<int>( depth ), static_cast<int>( vec->size() ) );

        // Records and strings are visited element-by-element; primitive runs (byte
        // buffers, float/Vec arrays) go through the bulk hook so archives can
        // read/write them in a single operation.
        if (elem_type->is_record() || elem_type->is_string()) {
            size_t idx = 0;
            for (auto& e : *vec) {
                visitor->array_element( elem_type, static_cast<int>( depth + 1 ), static_cast<int>( idx++ ) );
                if (elem_type->is_record())
                    static_cast<const RecordInfo*>( elem_type )->visit( &e, visitor, filter, depth + 2 );
                else
                    visitor->string( elem_type, &e );
            }
        } else if (!vec->empty()) {
            visitor->primitive_array( elem_type, vec->data(), vec->size() );
        }

        visitor->array_end( elem_type, static_cast<int>( depth ) );
    }
};

struct NCAPI StringClass : public RecordInfo {
    StringClass( const char* n, TypeId i, size_t sz, size_t align ) : RecordInfo( n, i, sz, align )
    {
        this->kind = TypeKind::STRING;
    }

    void to_string( String& out, const void* instance ) const override;

    void construct( void* instance, const void* data = nullptr ) const override;
    void destruct( void* instance ) const override;
    void clone( const void* src, void* dst ) const override;
    void replace( const void* src, void* dst ) const override;

    void visit( void* instance, RecordVisitor* visitor, PropertyFlags filter, unsigned depth ) const noexcept override
    {
        ( void ) filter;
        visitor->string( this, instance );
    }
};

} // namespace nc::types
