#pragma once

#include <editor/editor_frame.h>
#include <ncore/types/typeinfo.h>

namespace sea::editor {

/**
 * @brief Base class for feature-specific inspectors that extend the generic RTTI sheet.
 */
class InspectorPlugin {
public:
    virtual ~InspectorPlugin() = default;

    virtual bool can_handle( const nc::types::TypeInfo* type, void* instance ) const = 0;

    virtual void parse_begin( EditorFrame& frame, const nc::types::TypeInfo* type, void* instance )
    {
        ( void ) frame;
        ( void ) type;
        ( void ) instance;
    }

    virtual void parse_end( EditorFrame& frame, const nc::types::TypeInfo* type, void* instance )
    {
        ( void ) frame;
        ( void ) type;
        ( void ) instance;
    }

    /**
     * @brief Skip the generic RTTI sheet entirely for instances this plugin owns.
     */
    virtual bool replaces_default() const
    {
        return false;
    }

    /**
     * @brief Godot-inspired per-property hook.
     * Called by PropertyGrid while drawing fields of a record.
     * Return true to suppress the default drawing for this field (you should
     * have drawn your own row via begin_prop_row or similar).
     * Drop-in replacement for complex per-field logic; we only take the essential.
     */
    virtual bool parse_property(
        EditorFrame& frame, const nc::types::TypeInfo* type, void* instance, const nc::types::FieldInfo& field,
        const char* label, int depth
    )
    {
        ( void ) frame;
        ( void ) type;
        ( void ) instance;
        ( void ) field;
        ( void ) label;
        ( void ) depth;
        return false;
    }
};

} // namespace sea::editor
