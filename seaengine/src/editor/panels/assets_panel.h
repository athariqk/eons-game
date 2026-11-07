// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: assets_panel.h

#pragma once

#include "editor_panel.h"

namespace sea::editor {

class EditorAssets;
class EditorSelection;

/**
 * @brief Asset browser for asset previews, reload, and reimport.
 */
class AssetsPanel final : public EditorPanel {
public:
    AssetsPanel( EditorAssets& p_assets NC_LIFETIME_BOUND, EditorSelection& p_selection NC_LIFETIME_BOUND );

    void draw_contents( EditorFrame& frame ) override;

private:
    EditorAssets& assets;
    EditorSelection& selection;

    char search_str[64]  = {};
    int kind_filter      = 0; // 0 = all kinds, otherwise AssetKind + 1
    bool stale_only      = false;
    int sort_column      = 0;
    bool sort_descending = false;
    bool grid_view       = true;
};

} // namespace sea::editor
