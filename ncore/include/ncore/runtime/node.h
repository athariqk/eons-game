#pragma once

#include <ncore/core/collection.h>
#include <ncore/core/object.h>
#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_query.h>

namespace nc {

class EcsWorld;
class Scene;

/**
 * @brief Node is a wrapper over an EcsEntity. Node provides first-class
 * operations for Scene hierarchy and relationships. You can add any
 * components to a Node as POD or even non-POD classes/structs.
 */
class NCAPI Node : public NcObject {
    NCLASS( Node, NcObject )

public:
    using NodePool = PagedPool<Node>;

    Node() = default;
    Node( const String& p_name, Scene* p_scene, Node* p_parent );
    ~Node() override;

    Node( const Node& )            = delete;
    Node& operator=( const Node& ) = delete;

    class ChildRange {
    public:
        struct Iterator {
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
            Iterator( EcsIterator iter, Scene* scene, bool end );

            EcsIterator iter_;
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
        EcsQuery query_;
        Scene* scene_;
    };

    Node* create_child( const String& name = String() );
    ChildRange get_children();
    uint32_t get_child_count();
    void destroy_child( Node* child );
    void destroy_children();

    void reparent_to( Node* child );

    /**
     * @brief Return a read-only component of type T owned by this Node.
     */
    template<class T>
    const T* get_component() const
    {
        auto result = get_component_const_( rtti::TypeRegistry::find<T>() );
        return static_cast<const T*>( result );
    }

    /**
     * @brief Return a read-write singleton component of type T owned by this Node.
     */
    template<class T>
    T* get_component()
    {
        auto result = get_component_( rtti::TypeRegistry::find<T>() );
        return static_cast<T*>( result );
    }

    /**
     * @brief Create and set a component owned by this Node.
     * @param args Optional value to set to the component using copy-semantics.
     * @return The mutable component instance.
     */
    template<class T, class... Args>
    T* add_component( Args&&... args )
    {
        auto type = rtti::TypeRegistry::find<T>();
        if constexpr (sizeof...( Args ) != 0) {
            T val( std::forward<Args>( args )... );
            return static_cast<T*>( add_component_( type, &val ) );
        } else {
            return static_cast<T*>( emplace_component_( type ) );
        }
    }

    /**
     * @return True if this Node has component of given ID.
     */
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

    StringView get_name() const;
    uint64_t get_id() const;

    inline Scene* get_scene() const
    {
        return scene;
    }

private:
    friend class Scene;

    void* emplace_component_( const rtti::TypeInfo* type );
    void* add_component_( const rtti::TypeInfo* type, const void* data );
    void* get_component_( const rtti::TypeInfo* type ) const;             // returns mutable ptr
    const void* get_component_const_( const rtti::TypeInfo* type ) const; // returns const ptr, no staging
    bool has_component_( const rtti::TypeInfo* type ) const;
    void remove_component_( const rtti::TypeInfo* type ) const;
    void emit_event_( const rtti::TypeInfo* type, EcsEntity target, const void* data ) const;

    Scene* scene          = nullptr;
    Node* parent          = nullptr;
    NodePool* node_pool   = nullptr; // owned by Scene.
    EcsEntity internal_id = INVALID_ENTITY_ID;
    EcsQuery child_query{};
};

} // namespace nc
