// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: object.h - essential base classes for all object-oriented NCORE types.

#pragma once

#include <ncore/core/typedefs.h>

#include "../types/typedb.h"

namespace nc {

/**
 * @brief Object is the base class for every object-oriented NCORE types.
 * It provides first-class dynamic type features.
 *
 * NOTE: always declare NCLASS macro in derived classes to properly
 * register them in the reflection system.
 */
class NCAPI Object {
public:
    virtual ~Object() = default;

    Object()                           = default;
    Object( const Object& )            = default;
    Object& operator=( const Object& ) = default;

    virtual const StringView get_class_name() const = 0;
    virtual types::TypeId get_type_id() const       = 0;

    virtual const types::RecordInfo& get_class_info() const = 0;

    bool is_a( types::TypeId type_id ) const;

    template<typename T>
    bool is_a() const
    {
        return is_a( TypeDB::get_type_id<T>() );
    }
};

/**
 * @brief RefCounted is a typical implementation of an
 * intrusive reference counting mechanism, in this case
 * for Object classes.
 *
 * Useful read: https://baptiste-wicht.com/posts/2011/11/boost-intrusive_ptr.html,
 *				https://chadaustin.me/2023/11/reference-counting-things/
 */
class NCAPI RefCounted : public Object {
public:
    RefCounted() = default;

    RefCounted( const RefCounted& other )            = delete;
    RefCounted& operator=( const RefCounted& other ) = delete;

    void increment_ref();

    /**
     * @brief Decrements the reference count atomically.
     *
     * @return True if decrementing to zero.
     */
    bool decrement_ref();

    uint32_t get_ref_count() const;

    const StringView get_class_name() const override;
    types::TypeId get_type_id() const override;
    const types::RecordInfo& get_class_info() const override;

private:
    mutable std::atomic<uint32_t> ref_count{ 1 };
};

} // namespace nc
