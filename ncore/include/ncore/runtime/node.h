#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/object.h>
#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_query.h>

namespace nc {

class EcsWorld;
class Scene;

/**
 * @brief Component is a lightweight wrapper over an EcsComponent.
 * For use in the Scene API.
 */
struct NCAPI Component {
    EcsComponent EcsId = 0;
    bool Active        = false;
    bool Toggleable    = false;
    NSTRUCTV( Component, NC_F( Component, EcsId ), NC_F( Component, Active ) )
};

struct NCAPI NodeAddedEvent {
    NSTRUCT1( NodeAddedEvent )
};

/**
 * @brief Node is a lightweight wrapper over an EcsEntity. Node provides
 * first-class operations for Scene hierarchy and relationships. You can
 * add any components to a Node as POD or even non-POD classes/structs.
 */
class NCAPI Node : public NcObject {
    NCLASS( Node, NcObject )

public:
    using NodePool = PagedPool<Node>;

    // lets increase the max number later if needed
    inline static const uint32_t MAX_TRACKED_COMPONENTS = 16;

    Node() = default;
    Node( const String& p_name, Scene* p_scene, Node* p_parent );
    ~Node() override;

    Node( const Node& )            = delete;
    Node& operator=( const Node& ) = delete;

    bool operator==( const Node& other ) const;
    bool operator==( const Node* other ) const;
    bool operator!=( const Node& other ) const;

    class NCAPI ChildRange {
    public:
        struct NCAPI Iterator {
            using iterator_category = std::input_iterator_tag;
            using value_type        = Node;
            using difference_type   = std::ptrdiff_t;
            using pointer           = Node*;
            using reference         = Node&;

            reference operator*();
            Iterator& operator++();
            bool operator!=( const Iterator& o ) const;
            bool operator!=( std::nullptr_t ) const
            {
                return !done_;
            }

        private:
            friend class ChildRange;
            Iterator( EcsTableIterator iter, Scene* scene, bool end );

            EcsTableIterator iter_;
            Scene* scene_  = nullptr;
            int32_t index_ = 0;
            int32_t count_ = 0;
            bool done_     = true;
        };

        ChildRange( EcsQuery& query, Scene* scene );
        ~ChildRange() = default;

        ChildRange( const ChildRange& )            = delete;
        ChildRange& operator=( const ChildRange& ) = delete;
        ChildRange( ChildRange&& o ) noexcept;
        ChildRange& operator=( ChildRange&& o ) noexcept;

        Iterator begin();
        Iterator end();

    private:
        EcsQuery query;
        Scene* scene_;
    };

    /**
     * @brief Recreate a new parent-child entity relationship.
     * @param parent The new parent for this node.
     */
    void reparent_to( Node* parent );

    /**
     * @brief Checks whether this node is a child of the specified node.
     */
    bool is_descendant_of( Node& node );

    /**
     * @brief Queue removal of this Node from the Scene.
     */
    void destroy();

    /**
     * @brief Add a named child to this Node.
     * @return The new child node instance.
     */
    Node* create_child( const String& name = String() );
    ChildRange get_children();
    uint32_t get_child_count();
    void destroy_children();

    /**
     * @brief Get immutable pointer to component instance, no staging.
     */
    const void* get_component_const( const rtti::TypeInfo* type ) const;
    /**
     * @brief Get mutable pointer to component instance.
     * @return The matching component owned by this node.
     */
    void* get_component( const rtti::TypeInfo* type ) const;

    /**
     * @brief Get read-only pointer to component instance of type T.
     * @return The matching component owned by this node.
     */
    template<class T>
    const T* get_component() const
    {
        auto result = get_component_const( rtti::TypeRegistry::find<T>() );
        return static_cast<const T*>( result );
    }

    /**
     * @brief Get read-write pointer to component instance of type T.
     * @return The matching component owned by this node.
     */
    template<class T>
    T* get_component()
    {
        auto result = get_component( rtti::TypeRegistry::find<T>() );
        return static_cast<T*>( result );
    }

    Span<Component> get_components();

    /**
     * @brief Create in-place (default-constructs) a component owned by this Node.
     * @return Mutable pointer to constructed component instance, or existing one.
     */
    void* add_component( const rtti::TypeInfo* type );

    /**
     * @brief Create and set (copy-constructs) a component owned by this Node.
     * @param type RTTI type of the component.
     * @param data Existing data to copy.
     * @return Mutable pointer to new component instance, or existing one.
     */
    void* add_component( const rtti::TypeInfo* type, const void* data );

    /**
     * @brief Create and set a component owned by this Node.
     * @param args Optional value to set to the component using copy-semantics.
     * @return The new mutable pointer to component instance, or the existing one.
     */
    template<class T, class... Args>
    T* add_component( Args&&... args )
    {
        auto type = rtti::TypeRegistry::find<T>();
        if constexpr (sizeof...( Args ) != 0) {
            T val( std::forward<Args>( args )... );
            return static_cast<T*>( add_component( type, &val ) );
        } else {
            return static_cast<T*>( add_component( type ) );
        }
    }

    /**
     * @brief Return true if this Node has component of given RTTI type.
     */
    bool has_component( const rtti::TypeInfo* type ) const;
    /**
     * @brief Remove component owned by this node by type.
     */
    void remove_component( const rtti::TypeInfo* type );

    template<class T>
    bool has_component() const
    {
        return has_component_( rtti::TypeRegistry::find<T>() );
    }

    template<class T>
    void remove_component()
    {
        return remove_component_( rtti::TypeRegistry::find<T>() );
    }

    template<class T>
    void emit_event( const T& data, EcsEntity target ) const
    {
        emit_event_( rtti::TypeRegistry::find<T>(), target, &data );
    }

    template<class T>
    void emit_event( EcsEntity target ) const
    {
        emit_event_( rtti::TypeRegistry::find<T>(), target, nullptr );
    }

    void set_component_enabled( const rtti::TypeInfo* type, bool enabled );
    bool is_component_enabled( const rtti::TypeInfo* type ) const;

    template<class T>
    void set_component_enabled( bool enabled )
    {
        set_component_enabled( rtti::TypeRegistry::find<T>(), enabled );
    }

    template<class T>
    bool is_component_enabled() const
    {
        return is_component_enabled( rtti::TypeRegistry::find<T>() );
    }

    /**
     * @brief Signal that a component has been modified.
     */
    void mark_component_modified( const rtti::TypeInfo* type ) const;
    template<class T>
    void mark_component_modified()
    {
        mark_component_modified( rtti::TypeRegistry::find<T>() );
    }

    StringView get_name() const;
    void set_name( StringView name );
    uint64_t get_id() const;

    bool* get_active();

    inline Scene* get_scene() const
    {
        return scene;
    }

private:
    friend class Scene;

    void track_ecs_component( const rtti::TypeInfo* type, EcsComponent id );
    bool has_component_( const rtti::TypeInfo* type ) const;
    void remove_component_( const rtti::TypeInfo* type );
    void emit_event_( const rtti::TypeInfo* type, EcsEntity target, const void* data ) const;

    bool active           = true;
    Scene* scene          = nullptr;
    Node* parent          = nullptr;
    NodePool* node_pool   = nullptr; // owned by a Scene instance.
    EcsEntity internal_id = INVALID_ENTITY_ID;
    EcsQuery child_query{};
    Array<Component, MAX_TRACKED_COMPONENTS> components{};
    uint32_t component_count = 0;
};

} // namespace nc
