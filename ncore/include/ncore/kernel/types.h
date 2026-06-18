#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <ncore/utils/assert.h>
#include <ncore/utils/macro.h>

// Core reflection types.
// Inspired by Arvid Gerstmann's metareflect
// https://github.com/Leandros/metareflect

namespace ncore::rfl {

//------------------------------------------------------------------------------

struct TypeId {
    size_t value;

    bool operator==(TypeId o) const { return value == o.value; }
    bool operator!=(TypeId o) const { return value != o.value; }
    bool valid() const { return value != 0; }
    static constexpr TypeId null() { return {0}; }
};

//------------------------------------------------------------------------------

enum class PropertyFlags : uint16_t {
    None = 0,
    Serializable = 1 << 0,
    Editable = 1 << 1,
    ReadOnly = 1 << 2,
    Hidden = 1 << 3,
};

constexpr PropertyFlags operator|(PropertyFlags a, PropertyFlags b) noexcept {
    return static_cast<PropertyFlags>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}
constexpr PropertyFlags operator&(PropertyFlags a, PropertyFlags b) noexcept {
    return static_cast<PropertyFlags>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}
constexpr PropertyFlags operator~(PropertyFlags a) noexcept {
    return static_cast<PropertyFlags>(~static_cast<uint16_t>(a));
}
constexpr PropertyFlags &operator|=(PropertyFlags &a, PropertyFlags b) noexcept { return a = a | b; }
constexpr PropertyFlags &operator&=(PropertyFlags &a, PropertyFlags b) noexcept { return a = a & b; }

constexpr bool has_flag(PropertyFlags f, PropertyFlags check) noexcept { return (f & check) != PropertyFlags::None; }
constexpr bool has_any_flag(PropertyFlags f, PropertyFlags mask) noexcept {
    return (static_cast<uint16_t>(f) & static_cast<uint16_t>(mask)) != 0;
}
constexpr PropertyFlags set_flag(PropertyFlags f, PropertyFlags bit) noexcept { return f | bit; }
constexpr PropertyFlags clear_flag(PropertyFlags f, PropertyFlags bit) noexcept { return f & ~bit; }

//------------------------------------------------------------------------------

enum class FieldCategory : uint8_t {
    Scalar,
    String,
    Container,
    Aggregate,
    Pointer,
};

//------------------------------------------------------------------------------

struct Qualifier {
    unsigned array_length : 30;
    unsigned is_pointer : 1;
    unsigned is_array : 1;
};

//------------------------------------------------------------------------------

struct TypeInfo {
    const char *name;
    TypeId id;
    size_t size;

    TypeInfo() : name(nullptr), id(TypeId::null()), size(0) {}
    TypeInfo(const char *n, TypeId i, size_t sz) : name(n), id(i), size(sz) {}

    virtual ~TypeInfo() = default;
    virtual bool is_class() const noexcept { return false; }
};

//------------------------------------------------------------------------------

struct ContainerOps {
    size_t (*size)(const void *);
    void *(*at)(void *, size_t);
    void (*insert_default)(void *);
    void (*erase_at)(void *, size_t);
    void (*clear)(void *);
    TypeId value_type_id;
};

//------------------------------------------------------------------------------

struct FieldInfo {
    std::string_view name;
    TypeInfo *type;
    size_t width;
    size_t offset;
    PropertyFlags flags;
    FieldCategory category;
    Qualifier qualifier;

    static constexpr unsigned FLAGS_SERIALIZED = 0x1;
    static constexpr unsigned FLAGS_CSTRING = 0x2;

    template<typename T>
    T get_as(void *instance) const noexcept {
        T ret{};
        memcpy(&ret, static_cast<uint8_t *>(instance) + offset, sizeof(T));
        return ret;
    }

    template<typename T>
    T get_as(const void *instance) const noexcept {
        T ret{};
        memcpy(&ret, static_cast<const uint8_t *>(instance) + offset, sizeof(T));
        return ret;
    }

    template<typename T>
    T *get_ptr(void *instance) const noexcept {
        return static_cast<T *>(static_cast<uint8_t *>(instance) + offset);
    }

    template<typename T>
    const T *get_ptr(const void *instance) const noexcept {
        return static_cast<const T *>(static_cast<const uint8_t *>(instance) + offset);
    }

    void *get_void_ptr(void *instance) const noexcept { return static_cast<uint8_t *>(instance) + offset; }

    const void *get_void_ptr(const void *instance) const noexcept {
        return static_cast<const uint8_t *>(instance) + offset;
    }

    bool is(PropertyFlags f) const noexcept { return has_flag(flags, f); }
};

//------------------------------------------------------------------------------

struct ClassVisitor;

struct ClassInfo : public TypeInfo {
    TypeId parent_id = TypeId::null();
    const FieldInfo *fields_begin = nullptr;
    const FieldInfo *fields_end = nullptr;
    ClassInfo *_next = nullptr;

    ClassInfo() = default;
    ClassInfo(const char *n, TypeId i, size_t sz) : TypeInfo(n, i, sz) {}

    bool is_class() const noexcept override { return true; }

    size_t field_count() const noexcept { return fields().size(); }

    std::span<const FieldInfo> fields() const noexcept { return {fields_begin, fields_end}; }

    const FieldInfo *find_field(std::string_view n) const noexcept {
        for (auto &f: fields())
            if (f.name == n)
                return &f;
        return nullptr;
    }

    virtual void visit(void const *instance, ClassVisitor *visitor,
                       PropertyFlags filter = static_cast<PropertyFlags>(0xFFFF), unsigned depth = 0) const noexcept;

    virtual void visit_field(void const *ptr, const FieldInfo *field, ClassVisitor *visitor, PropertyFlags filter,
                             int depth, int array_elem = -1) const noexcept;

    virtual void visit_array(void const *ptr, const FieldInfo *field, ClassVisitor *visitor, PropertyFlags filter,
                             unsigned depth) const noexcept;
};

//------------------------------------------------------------------------------

struct ClassVisitor {
    virtual ~ClassVisitor() = default;

    virtual void class_begin(const ClassInfo *c, int depth) = 0;
    virtual void class_end(const ClassInfo *c, int depth) = 0;
    virtual void class_member(const FieldInfo *f, int depth) = 0;

    virtual void array_begin(const TypeInfo *t, int depth, int length) = 0;
    virtual void array_end(const TypeInfo *t, int depth) = 0;
    virtual void array_element(const TypeInfo *t, int depth, int elem) = 0;

    virtual void primitive(const TypeInfo *t, const void *instance) = 0;
    virtual void string(const TypeInfo *t, const void *instance) = 0;
};

//------------------------------------------------------------------------------

template<typename VecT>
struct VectorClass : public ClassInfo {
    VectorClass(const char *n, TypeId i, size_t sz) : ClassInfo(n, i, sz) {}

    void visit(void const *instance, ClassVisitor *visitor, PropertyFlags filter,
               unsigned depth) const noexcept override {
        if (!instance) {
            visitor->primitive(this, nullptr);
            return;
        }

        auto *vec = static_cast<const VecT *>(instance);
        auto *elem_type = get_type<typename VecT::value_type>();

        visitor->array_begin(elem_type, static_cast<int>(depth), static_cast<int>(vec->size()));
        size_t idx = 0;
        for (auto const &e: *vec) {
            visitor->array_element(elem_type, static_cast<int>(depth + 1), static_cast<int>(idx++));
            if (elem_type->is_class())
                static_cast<const ClassInfo *>(elem_type)->visit(&e, visitor, filter, depth + 2);
            else
                visitor->primitive(elem_type, &e);
        }
        visitor->array_end(elem_type, static_cast<int>(depth));
    }
};

//------------------------------------------------------------------------------

struct StringClass : public ClassInfo {
    StringClass(const char *n, TypeId i, size_t sz) : ClassInfo(n, i, sz) {}

    void visit(void const *instance, ClassVisitor *visitor, PropertyFlags filter,
               unsigned depth) const noexcept override {
        (void) filter;
        if (!instance) {
            visitor->string(this, nullptr);
            return;
        }
        auto *str = static_cast<const std::string *>(instance);
        auto *cstr = str->c_str();
        visitor->string(this, &cstr);
    }
};

//------------------------------------------------------------------------------

namespace detail {

constexpr size_t fnv1a(const char *s, size_t n) noexcept {
    size_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < n; ++i)
        h = (h ^ static_cast<uint8_t>(s[i])) * 1099511628211ULL;
    return h ? h : 1;
}

template<typename T>
constexpr size_t type_hash() noexcept {
#if defined(__GNUC__) || defined(__clang__)
    constexpr std::string_view sig = __PRETTY_FUNCTION__;
#elif defined(_MSC_VER)
    constexpr std::string_view sig = __FUNCSIG__;
#else
#error "Unsupported compiler for stable type IDs"
#endif
    return fnv1a(sig.data(), sig.size());
}

} // namespace detail

template<typename T>
constexpr TypeId type_id() noexcept {
    return TypeId{detail::type_hash<T>()};
}

//------------------------------------------------------------------------------

/**
 * @brief A global registry of reflected types and classes.
 * Equivalent to Godot's ClassDB.
 */
class Registry {
public:
    static void register_class(ClassInfo &info) noexcept {
        info._next = _head;
        _head = &info;
    }

    static const ClassInfo *find(TypeId id) noexcept {
        for (auto *c = _head; c; c = c->_next)
            if (c->id == id)
                return c;
        NC_LOG_WARN("class with ID '{}' not found, probably not reflected?", id.value);
        return nullptr;
    }

    static const ClassInfo *find(std::string_view name) noexcept {
        for (auto *c = _head; c; c = c->_next)
            if (name == c->name)
                return c;
        NC_LOG_WARN("class with name '{}' not found, probably not reflected?", name);
        return nullptr;
    }

    static const ClassInfo &get(TypeId id) noexcept;
    static const ClassInfo &get(std::string_view name) noexcept;

    static const std::string_view get_class_name(TypeId id) noexcept {
        auto *c = find(id);
        return c ? c->name : "<unknown>";
    }

    template<typename T>
    static const ClassInfo *find() noexcept {
        return find(type_id<T>());
    }

    /**
     * @brief Hard exits if we can't find the class info.
     */
    template<typename T>
    static const ClassInfo &get() noexcept {
        NC_ASSERT(is_registered<T>(), "class type is not found in the registry");
        return get(type_id<T>());
    }

    template<typename T>
    static bool is_registered() noexcept {
        return find<T>() != nullptr;
    }

    template<typename T>
    static const std::string_view get_class_name() {
        return get_class_name(type_id<T>());
    }

private:
    inline static ClassInfo *_head = nullptr;
};

template<typename T>
struct ClassTag {};
template<typename T>
struct TypeTag {};

namespace detail {

template<typename T>
const TypeInfo *get_type_impl(TypeTag<T>) noexcept {
    return Registry::find(type_id<T>());
}
template<typename T>
const ClassInfo *get_class_impl(ClassTag<T>) noexcept {
    return Registry::find(type_id<T>());
}

#define NC_RFL_FUNDAMENTAL(T)                                                                                          \
    template<>                                                                                                         \
    inline const TypeInfo *get_type_impl(TypeTag<T>) noexcept {                                                        \
        struct _##T##_ti : TypeInfo {                                                                                  \
            _##T##_ti() : TypeInfo(#T, type_id<T>(), sizeof(T)) {}                                                     \
        };                                                                                                             \
        static _##T##_ti ti;                                                                                           \
        return &ti;                                                                                                    \
    }

NC_RFL_FUNDAMENTAL(bool)
NC_RFL_FUNDAMENTAL(int32_t)
NC_RFL_FUNDAMENTAL(uint32_t)
NC_RFL_FUNDAMENTAL(int64_t)
NC_RFL_FUNDAMENTAL(uint64_t)
NC_RFL_FUNDAMENTAL(float)
NC_RFL_FUNDAMENTAL(double)
NC_RFL_FUNDAMENTAL(uint8_t)

#undef NC_RFL_FUNDAMENTAL

// std::vector<T>
template<typename T>
const ClassInfo *get_class_impl(ClassTag<std::vector<T>>) noexcept {
    static VectorClass<std::vector<T>> v("std::vector<T>", type_id<std::vector<T>>(), sizeof(std::vector<T>));
    return &v;
}

template<typename T>
const TypeInfo *get_type_impl(TypeTag<std::vector<T>>) noexcept {
    return get_class_impl(ClassTag<std::vector<T>>{});
}

// std::string
template<>
inline const ClassInfo *get_class_impl(ClassTag<std::string>) noexcept {
    static StringClass s("std::string", type_id<std::string>(), sizeof(std::string));
    return &s;
}

template<>
inline const TypeInfo *get_type_impl(TypeTag<std::string>) noexcept {
    return get_class_impl(ClassTag<std::string>{});
}

template<typename F>
const TypeInfo *type_for() noexcept {
    return get_type_impl(TypeTag<std::remove_cvref_t<F>>{});
}

template<typename F>
constexpr FieldCategory category_of() noexcept {
    using raw = std::remove_cvref_t<F>;
    if constexpr (std::is_pointer_v<raw>) {
        if constexpr (std::is_convertible_v<raw, std::string_view>)
            return FieldCategory::String;
        return FieldCategory::Pointer;
    } else if constexpr (std::is_convertible_v<raw, std::string_view>) {
        return FieldCategory::String;
    } else if constexpr (requires { typename raw::value_type; }) {
        return FieldCategory::Container;
    } else {
        return FieldCategory::Scalar;
    }
}

} // namespace detail

template<typename T>
const TypeInfo *get_type() noexcept {
    return detail::get_type_impl(TypeTag<T>{});
}

template<typename T>
const ClassInfo *get_class() noexcept {
    return detail::get_class_impl(ClassTag<T>{});
}

} // namespace ncore::rfl

namespace std {
template<>
struct hash<ncore::rfl::TypeId> {
    size_t operator()(ncore::rfl::TypeId id) const noexcept { return id.value; }
};
} // namespace std

//------------------------------------------------------------------------------

#define NC_FIELD_IMPL(T, m, flg, q)                                                                                    \
    ::ncore::rfl::FieldInfo{                                                                                           \
        #m,                                                                                                            \
        const_cast<::ncore::rfl::TypeInfo *>(::ncore::rfl::detail::type_for<decltype(((T *) 0)->m)>()),                \
        sizeof(((T *) 0)->m),                                                                                          \
        offsetof(T, m),                                                                                                \
        flg,                                                                                                           \
        ::ncore::rfl::detail::category_of<decltype(((T *) 0)->m)>(),                                                   \
        q,                                                                                                             \
    },

#define NC_F(T, m)                                                                                                     \
    NC_FIELD_IMPL(T, m, (::ncore::rfl::PropertyFlags::Serializable | ::ncore::rfl::PropertyFlags::Editable),           \
                  ::ncore::rfl::Qualifier{})

#define NC_FR(T, m)                                                                                                    \
    NC_FIELD_IMPL(T, m,                                                                                                \
                  (::ncore::rfl::PropertyFlags::Serializable | ::ncore::rfl::PropertyFlags::Editable |                 \
                   ::ncore::rfl::PropertyFlags::ReadOnly),                                                             \
                  ::ncore::rfl::Qualifier{})

#define NC_FH(T, m) NC_FIELD_IMPL(T, m, ::ncore::rfl::PropertyFlags::Serializable, ::ncore::rfl::Qualifier{})

//------------------------------------------------------------------------------

#define NC_BIND(T, ...)                                                                                                \
    static ::ncore::rfl::ClassInfo &_nc_info_##T() {                                                                   \
        static ::ncore::rfl::FieldInfo _nc_flds_##T[] = {__VA_ARGS__};                                                 \
        static ::ncore::rfl::ClassInfo _nc_ci_##T;                                                                     \
        static bool _once = false;                                                                                     \
        if (!_once) {                                                                                                  \
            _nc_ci_##T = ::ncore::rfl::ClassInfo(#T, ::ncore::rfl::type_id<T>(), sizeof(T));                           \
            _nc_ci_##T.fields_begin = _nc_flds_##T;                                                                    \
            _nc_ci_##T.fields_end = _nc_flds_##T + (sizeof(_nc_flds_##T) / sizeof(::ncore::rfl::FieldInfo));           \
            ::ncore::rfl::Registry::register_class(_nc_ci_##T);                                                        \
            _once = true;                                                                                              \
        }                                                                                                              \
        return _nc_ci_##T;                                                                                             \
    }                                                                                                                  \
    inline static const int _nc_trig_##T = (_nc_info_##T(), 0);
