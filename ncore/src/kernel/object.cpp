#include <ncore/kernel/object.h>

namespace ncore {

bool NObject::is_a( rfl::TypeId target ) const
{
    for (auto info = rfl::Registry::find_record( get_type_id() ); info;
         info      = rfl::Registry::find_record( info->parent_id )) {
        if (info->id == target)
            return true;
        if (!info->parent_id.valid())
            break;
    }
    return false;
}

} // namespace ncore
