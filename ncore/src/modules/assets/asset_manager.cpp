#include <modules/assets/asset_manager.h>

namespace ncore {

void AssetManager::unload_all()
{
    pools.clear();
}

} // namespace ncore
