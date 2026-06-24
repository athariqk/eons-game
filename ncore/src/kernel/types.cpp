#include <ncore/kernel/types.h>

#include <ncore/kernel/object.h>
#include <ncore/utils/assert.h>

namespace ncore::rfl {

// ======================================================================
// ClassRegistry
// ======================================================================

const TypeInfo &Registry::get(TypeId id) noexcept {
    auto *p = find(id);
    NC_ASSERT(p != nullptr, "Type not registered");
    return *p;
}

const TypeInfo &Registry::get(std::string_view name) noexcept {
    auto *p = find(name);
    NC_ASSERT(p != nullptr, "Type not registered");
    return *p;
}

bool Registry::register_primitive_types() {
	// !!TODO!!: make ALL these happen inside engine init instead of static init
	// 
    // static init of fundamental TypeInfo objects
    Registry::emplace<bool>("bool");
    Registry::emplace<int32_t>("int32_t");
    Registry::emplace<uint32_t>("uint32_t");
    Registry::emplace<int64_t>("int64_t");
    Registry::emplace<uint64_t>("uint64_t");
    Registry::emplace<float>("float");
    Registry::emplace<double>("double");
    Registry::emplace<size_t>("size_t");
    Registry::emplace<uint8_t>("uint8_t");
    Registry::emplace<StringClass, std::string>("std::string");
    Registry::emplace<VectorClass<std::vector<int>>, std::vector<int>>("std::vector<int>");

    // TODO: should this be here?
    Registry::emplace<RecordInfo, ncore::NObject>("NObject");

    return true;
}

// ======================================================================
// ClassInfo::visit
// ======================================================================

void RecordInfo::visit(void const *instance, RecordVisitor *visitor, PropertyFlags filter,
                       unsigned depth) const noexcept {
    if (!instance) {
        visitor->primitive(this, nullptr);
        return;
    }

    visitor->class_begin(this, static_cast<int>(depth));
    for (auto &f: fields()) {
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

void RecordInfo::visit_field(const void *ptr, const FieldInfo *field, RecordVisitor *visitor, PropertyFlags filter,
                             int depth, int array_elem) const noexcept {
    if (!has_any_flag(field->flags, filter))
        return;

    auto &q = field->qualifier;

    if (q.is_array)
        visitor->array_element(field->type, depth, array_elem);
    else
        visitor->class_member(field, depth);

    if (field->type->is_record()) {
        auto *c = static_cast<const RecordInfo *>(field->type);
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

void RecordInfo::visit_array(void const *ptr, FieldInfo const *field, RecordVisitor *visitor, PropertyFlags filter,
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
