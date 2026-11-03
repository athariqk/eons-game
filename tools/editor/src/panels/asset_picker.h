#pragma once

#include <editor.h>

#include <io/editor_assets.h>

namespace nc {
class ResourceLoader;
} // namespace nc

namespace nc::types {
struct IRefOps;
struct TypeInfo;
} // namespace nc::types

namespace nc::editor {

/**
 * @brief Draw and edit a reflected Ref<T> slot (TypeKind::REF).
 *
 * For IResource-derived targets this shows the asset path (or "None"), accepts
 * ASSET_PATH drag-drop, and opens a filtered asset popup via Assign. Assignment
 * goes through ResourceLoader::load + IRefOps::share so the handle refcount is
 * correct. Clear resets the slot; Create tries a default-constructed instance
 * when the target type allows it.
 *
 * @return true when the Ref slot was modified.
 */
bool draw_ref_property(
    EditorFrame& ctx, AssetDatabase& assets, ResourceLoader& loader, const types::TypeInfo* ref_type, void* ref_slot
);

} // namespace nc::editor
