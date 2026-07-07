#include <ncore/kernel/object.h>

namespace nc {

bool NcObject::is_a( rtti::TypeId target ) const
{
    for (auto info = rtti::Registry::find_record( get_type_id() ); info;
         info      = rtti::Registry::find_record( info->parent_id )) {
        if (info->id == target)
            return true;
        if (!info->parent_id.valid())
            break;
    }
    return false;
}

} // namespace nc
