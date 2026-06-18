#include <kernel/types.h>

#include <utils/assert.h>

namespace ncore::rfl {

// ======================================================================
// ClassRegistry
// ======================================================================

const ClassInfo &Registry::get(TypeId id) noexcept {
    auto *p = find(id);
    NC_ASSERT(p, "Type not registered");
    return *p;
}

const ClassInfo &Registry::get(std::string_view name) noexcept {
    auto *p = find(name);
    NC_ASSERT(p, "Type not registered");
    return *p;
}

// ======================================================================
// ClassInfo::visit
// ======================================================================

void ClassInfo::visit(void const *instance, ClassVisitor *visitor, PropertyFlags filter,
                      unsigned depth) const noexcept {
    if (!instance) {
        visitor->primitive(this, nullptr);
        return;
    }

    visitor->class_begin(this, static_cast<int>(depth));
    for (auto &f : fields()) {
        auto *ptr = f.get_void_ptr(const_cast<void *>(instance));
        if (f.qualifier.is_array)
            visit_array(ptr, &f, visitor, filter, depth + 1);
        else
            visit_field(ptr, &f, visitor, filter, static_cast<int>(depth + 1));
    }
    visitor->class_end(this, static_cast<int>(depth));
}

// ======================================================================
// ClassInfo::visit_field
// ======================================================================

void ClassInfo::visit_field(const void *ptr, const FieldInfo *field, ClassVisitor *visitor, PropertyFlags filter,
                            int depth, int array_elem) const noexcept {
    if (!has_any_flag(field->flags, filter))
        return;

    auto &q = field->qualifier;

    if (q.is_array)
        visitor->array_element(field->type, depth, array_elem);
    else
        visitor->class_member(field, depth);

    if (field->type->is_class()) {
        auto *c = static_cast<const ClassInfo *>(field->type);
        if (q.is_pointer) {
            auto *p = *static_cast<void const *const *>(ptr);
            if (p)
                c->visit(p, visitor, filter, static_cast<unsigned>(depth));
        } else {
            c->visit(ptr, visitor, filter, static_cast<unsigned>(depth));
        }
    } else {
        if (field->category == FieldCategory::String)
            visitor->string(field->type, ptr);
        else
            visitor->primitive(field->type, ptr);
    }
}

// ======================================================================
// ClassInfo::visit_array
// ======================================================================

void ClassInfo::visit_array(void const *ptr, FieldInfo const *field, ClassVisitor *visitor, PropertyFlags filter,
                            unsigned depth) const noexcept {
    if (!has_any_flag(field->flags, filter))
        return;

    auto &q = field->qualifier;
    visitor->class_member(field, static_cast<int>(depth));
    visitor->array_begin(field->type, static_cast<int>(depth), static_cast<int>(q.array_length));

    auto *cursor = static_cast<const uint8_t *>(ptr);
    for (unsigned i = 0; i < q.array_length; ++i) {
        visit_field(cursor, field, visitor, filter, static_cast<int>(depth + 1), static_cast<int>(i));
        cursor += field->width;
    }

    visitor->array_end(field->type, static_cast<int>(depth));
}

} // namespace ncore::rfl
