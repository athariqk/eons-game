#include "ast_visitor.h"

#include <set>
#include <sstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
#include "clang/AST/RecordLayout.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/Type.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;

namespace {

std::string get_qualified_name( const DeclContext* dc )
{
    std::string qname;
    const auto* ctx = dc;
    while (ctx) {
        if (const auto* ns = dyn_cast<NamespaceDecl>( ctx )) {
            if (!ns->isAnonymousNamespace() && !ns->isInlineNamespace()) {
                if (!qname.empty())
                    qname = "::" + qname;
                qname = ns->getNameAsString() + qname;
            }
            ctx = ns->getDeclContext();
        } else if (const auto* rec = dyn_cast<RecordDecl>( ctx )) {
            if (!qname.empty())
                qname = "::" + qname;
            qname = rec->getNameAsString() + qname;
            ctx   = rec->getDeclContext();
        } else if (const auto* ed = dyn_cast<EnumDecl>( ctx )) {
            if (!qname.empty())
                qname = "::" + qname;
            qname = ed->getNameAsString() + qname;
            ctx   = ed->getDeclContext();
        } else {
            break;
        }
    }
    return qname;
}

// Qualified name of a template itself (TemplateDecl isn't a DeclContext,
// so it needs its own helper rather than reusing get_qualified_name).
std::string get_qualified_template_name( const TemplateDecl* tpl )
{
    std::string ctx_name = get_qualified_name( tpl->getDeclContext() );
    std::string name     = tpl->getNameAsString();
    return ctx_name.empty() ? name : ctx_name + "::" + name;
}

// A qualified_name from get_qualified_name() is meant for display / registry
// keys. When emitting it as an actual C++ type reference in generated code,
// prefix it with "::" so it always resolves from global scope regardless of
// what's visible inside the generated file's `namespace nc { ... }` wrapper.
std::string qualify_for_cpp( const std::string& qualified_name )
{
    return qualified_name.empty() ? qualified_name : "::" + qualified_name;
}

bool has_reflect_attr( const Decl* d )
{
    for (const auto* attr : d->attrs()) {
        if (const auto* aa = dyn_cast<AnnotateAttr>( attr )) {
            if (aa->getAnnotation() == "Reflect")
                return true;
        }
    }
    return false;
}

bool inherits_from_object( const CXXRecordDecl* rd )
{
    for (const auto& base : rd->bases()) {
        if (const auto* base_type = base.getType()->getAsCXXRecordDecl()) {
            if (get_qualified_name( base_type ) == "nc::Object")
                return true;
            if (inherits_from_object( base_type ))
                return true;
        }
    }
    return false;
}

// NC_COMPONENT / NC_COMPONENT_API expand to `struct T : nc::detail::EcsComponentBase<T>`.
bool is_ecs_component_base( const CXXRecordDecl* rd )
{
    for (const auto& base : rd->bases()) {
        const auto* base_type = base.getType()->getAsCXXRecordDecl();
        if (!base_type)
            continue;
        // ClassTemplateSpecializationDecl: qualified name is "nc::detail::EcsComponentBase"
        // (template args omitted by getQualifiedNameAsString on some paths — also check short name).
        if (get_qualified_name( base_type ) == "nc::detail::EcsComponentBase")
            return true;
        if (base_type->getNameAsString() == "EcsComponentBase")
            return true;
    }
    return false;
}

// NSTRUCT_V injects get_class_info() into the class body.
bool has_nstruct_v_marker( const CXXRecordDecl* rd )
{
    for (const auto* m : rd->methods()) {
        if (m->getNameAsString() == "get_class_info")
            return true;
    }
    return false;
}

std::string get_type_name( const QualType& qt, const PrintingPolicy& pp )
{
    std::string type_str;
    llvm::raw_string_ostream os( type_str );
    qt.print( os, pp );
    return type_str;
}

std::vector<std::string> get_template_args( const TemplateSpecializationType* tst, const PrintingPolicy& pp )
{
    std::vector<std::string> args;
    for (const auto& arg : tst->template_arguments()) {
        if (arg.getKind() == TemplateArgument::Type) {
            args.push_back( get_type_name( arg.getAsType(), pp ) );
        } else if (arg.getKind() == TemplateArgument::Integral) {
            args.push_back( std::to_string( arg.getAsIntegral().getZExtValue() ) );
        }
    }
    return args;
}

std::string make_dep_key( const std::string& type_name )
{
    std::string key = type_name;
    std::replace( key.begin(), key.end(), ':', '_' );
    std::replace( key.begin(), key.end(), '<', '_' );
    std::replace( key.begin(), key.end(), '>', '_' );
    std::replace( key.begin(), key.end(), ' ', '_' );
    std::replace( key.begin(), key.end(), ',', '_' );
    std::replace( key.begin(), key.end(), '*', '_' );
    // Collapse consecutive underscores to avoid reserved identifiers
    std::string result;
    bool last_was_underscore = false;
    for (char c : key) {
        if (c == '_') {
            if (!last_was_underscore) {
                result += c;
                last_was_underscore = true;
            }
        } else {
            result += c;
            last_was_underscore = false;
        }
    }
    return result;
}

} // anonymous namespace

// FNV-1a hash (matches ncore's detail::fnv1a)
uint64_t compute_type_id_hash( const std::string& name )
{
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME  = 1099511628211ULL;
    uint64_t h                    = FNV_OFFSET;
    for (char c : name) {
        h ^= static_cast<uint8_t>( c );
        h *= FNV_PRIME;
    }
    return h ? h : 1;
}

//------------------------------------------------------------------------------

class ReflectionVisitor : public RecursiveASTVisitor<ReflectionVisitor> {
public:
    explicit ReflectionVisitor( ASTContext& ctx ) : ctx_( ctx ), pp( ctx.getPrintingPolicy() ) {}

    bool VisitRecordDecl( RecordDecl* rd )
    {
        if (!rd->isCompleteDefinition())
            return true;

        // Only reflect declarations written in the file currently being
        // compiled. Types pulled in transitively via #include get reflected
        // when *that* header is itself passed to the tool; re-processing them
        // here would emit duplicate TypeRegistry registrations in every
        // generated file that happens to include this header.
        const SourceManager& sm = ctx_.getSourceManager();
        if (!sm.isInMainFile( sm.getExpansionLoc( rd->getLocation() ) ))
            return true;

        // in ReflectionVisitor::VisitRecordDecl, after the main-file check passes:
        ++stats.records_visited;

        if (const auto* tmpl_check = dyn_cast<CXXRecordDecl>( rd )) {
            // Skip the uninstantiated pattern of a class template: its
            // members have dependent types that can't be meaningfully
            // reflected. Concrete instantiations are visited separately as
            // their own RecordDecls.
            if (tmpl_check->getDescribedClassTemplate())
                return true;
        }

        bool has_refl_fields = false;
        ReflectedRecord rec;
        rec.name             = rd->getNameAsString();
        rec.qualified_name   = get_qualified_name( rd );
        rec.is_class         = isa<CXXRecordDecl>( rd );
        rec.is_ecs_component = false;
        rec.has_nstruct_v    = false;

        // Compute record size and alignment from EXPORT fields only
        size_t max_end   = 0;
        size_t max_align = 1;
        for (auto* field : rd->fields()) {
            if (!has_reflect_attr( field ))
                continue;
            size_t fsize  = ctx_.getTypeSizeInChars( field->getType() ).getQuantity();
            size_t falign = ctx_.getTypeAlignInChars( field->getType() ).getQuantity();
            max_align     = std::max( max_align, falign );
        }

        // Compute record size and alignment from ASTRecordLayout when safe
        rec.record_size      = 1;
        rec.record_alignment = 1;
        if (const auto* cxx_rd = dyn_cast<CXXRecordDecl>( rd )) {
            if (cxx_rd->isCompleteDefinition() && !cxx_rd->isDependentType() && !cxx_rd->getDescribedClassTemplate()) {
                const auto& layout   = ctx_.getASTRecordLayout( cxx_rd );
                rec.record_size      = layout.getSize().getQuantity();
                rec.record_alignment = layout.getAlignment().getQuantity();
            }
        }

        if (rec.is_class) {
            const auto* crd = cast<CXXRecordDecl>( rd );
            rec.is_ecs_component = is_ecs_component_base( crd );
            rec.has_nstruct_v    = has_nstruct_v_marker( crd );
            // Find whichever base actually leads to Object -- not just the
            // first base -- since multiple inheritance means they can differ.
            for (const auto& base : crd->bases()) {
                const auto* base_type = base.getType()->getAsCXXRecordDecl();
                if (!base_type)
                    continue;
                if (get_qualified_name( base_type ) == "nc::Object" || inherits_from_object( base_type )) {
                    rec.parent_name = get_qualified_name( base_type );
                    break;
                }
            }
        }

        for (auto* field : rd->fields()) {
            if (!has_reflect_attr( field ))
                continue;

            if (has_refl_fields || has_reflect_attr( rd ))
                ++stats.records_with_refl_fields;

            if (field->isBitField()) {
                auto& diags     = ctx_.getDiagnostics();
                unsigned diagID = diags.getCustomDiagID(
                    DiagnosticsEngine::Warning,
                    "reflected field '%0' is a bit-field; its offset cannot be computed, skipping"
                );
                diags.Report( field->getLocation(), diagID ) << field->getNameAsString();
                continue;
            }

            has_refl_fields = true;
            ReflectedField f;
            f.name          = field->getNameAsString();
            f.field_size    = ctx_.getTypeSizeInChars( field->getType() ).getQuantity();
            f.pointer_count = 0;
            f.array_length  = 0;
            f.is_cstring    = false;

            // Compute field offset from ASTRecordLayout (safe for non-CXX types)
            f.field_offset = 0;
            if (const auto* cxx_rd = dyn_cast<CXXRecordDecl>( rd )) {
                if (cxx_rd->isCompleteDefinition()) {
                    const auto& layout = ctx_.getASTRecordLayout( cxx_rd );
                    f.field_offset =
                        ctx_.toCharUnitsFromBits( layout.getFieldOffset( field->getFieldIndex() ) ).getQuantity();
                }
            }

            // Strip array/pointer to get element type for TypeId
            QualType elem_type = field->getType();
            while (const auto* cat = elem_type->getAsArrayTypeUnsafe()) {
                if (const auto* cta = dyn_cast<ConstantArrayType>( cat ))
                    f.array_length = cta->getSize().getZExtValue();
                elem_type = cat->getElementType();
            }
            if (elem_type->isPointerType()) {
                f.pointer_count  = 1;
                QualType pointee = elem_type->getPointeeType();
                f.is_cstring     = pointee->isCharType() && pointee.isConstQualified();
                elem_type        = pointee;
            }

            // Always desugar: typedef / using / elaborated → canonical type.
            // TypeId and registry name are driven by the underlying type only.
            QualType canonical = elem_type.getCanonicalType();

            // Display / field type_name: prefer alias spelling when present
            if (const auto* tt = elem_type->getAs<TypedefType>()) {
                f.type_name = tt->getDecl()->getQualifiedNameAsString(); // "nc::String"
            } else {
                f.type_name = get_type_name( elem_type, pp );
            }

            if (const auto* et = canonical->getAs<EnumType>()) {
                f.element_type_name = get_qualified_name( et->getDecl() );
            } else if (const auto* tst = canonical->getAs<TemplateSpecializationType>()) {
                f.element_type_name = get_type_name( QualType( tst, 0 ), pp );
            } else if (const auto* rd = canonical->getAsCXXRecordDecl()) {
                // Class template specializations: get_qualified_name() uses
                // getNameAsString() which drops template args ("std::basic_string").
                // type_id<T>() hashes the full desugared specialization, so emit
                // the full printed canonical type to keep the TypeIds aligned.
                if (isa<ClassTemplateSpecializationDecl>( rd )) {
                    f.element_type_name = get_type_name( canonical, pp );
                } else {
                    f.element_type_name = get_qualified_name( rd );
                }
            } else {
                f.element_type_name = get_type_name( canonical, pp );
            }

            f.type_id_hash = compute_type_id_hash( f.element_type_name );
            collect_dep_types( canonical );

            rec.fields.push_back( f );
        }

        // Register the type if:
        //  - it has REFLECT-annotated fields, OR
        //  - it carries the REFLECT attribute itself, OR
        //  - it inherits from nc::Object (auto-detected, no annotation needed).
        // Object-derived classes always get a TypeRegistry entry so that
        // derived-class parent_id lookups resolve at runtime.
        // ECS_COMPONENT / NSTRUCT_V types still emit (for provide_fields /
        // TypeId static_assert) even when fieldless — registration itself is
        // done by the header macros at static init.
        if (has_refl_fields || has_reflect_attr( rd ) || !rec.parent_name.empty() || rec.is_ecs_component ||
            rec.has_nstruct_v)
            result.records.push_back( rec );

        return true;
    }

    ReflectionResult result;
    ReflectionStats stats;

private:
    void collect_dep_types( const QualType& qt );
    void register_container_type( const QualType& qt );
    void register_plain_record_type( const CXXRecordDecl* rd, const QualType& qt );
    void register_enum_type( const EnumDecl* ed );

    ASTContext& ctx_;
    PrintingPolicy pp;
    std::set<std::string> seen_dep_keys;
};

//------------------------------------------------------------------------------

void ReflectionVisitor::collect_dep_types( const QualType& qt )
{
    // Caller is expected to pass already-canonical types, but desugar
    // defensively so nested sugar still collapses.
    QualType canonical = qt.getCanonicalType();

    if (const auto* tst = canonical->getAs<TemplateSpecializationType>()) {
        // Nested deps first (e.g. allocator / traits / element type).
        for (const auto& arg : tst->template_arguments()) {
            if (arg.getKind() == TemplateArgument::Type)
                collect_dep_types( arg.getAsType() );
        }
        register_container_type( QualType( tst, 0 ) );
        return;
    }

    // Canonical class-template specializations (std::vector, std::array,
    // nc::Res, std::variant, …) desugar to a RecordType whose decl is a
    // ClassTemplateSpecializationDecl — NOT a TemplateSpecializationType.
    // Field TypeIds hash get_type_name(canonical, pp) for these, so register
    // under the same string (see VisitRecordDecl element_type_name branch).
    if (const auto* rd = canonical->getAsCXXRecordDecl()) {
        if (isa<ClassTemplateSpecializationDecl>( rd )) {
            const auto* spec = cast<ClassTemplateSpecializationDecl>( rd );
            const auto args  = spec->getTemplateArgs().asArray();
            for (const auto& arg : args) {
                if (arg.getKind() == TemplateArgument::Type)
                    collect_dep_types( arg.getAsType() );
            }
            register_container_type( canonical );
            return;
        }

        // Plain (non-template) record used as a field/element type — e.g.
        // gfx::VertexLayoutElement inside DynamicArray<VertexLayoutElement>.
        // RTTI lookup must never return null for these: VectorClass::visit
        // dereferences element types. Types that the record pass itself
        // registers (REFLECT / NSTRUCT / ECS / Object-derived) are skipped so
        // their field lists aren't clobbered by an empty registration.
        register_plain_record_type( rd, canonical );
        return;
    }

    if (const auto* et = canonical->getAs<EnumType>()) {
        register_enum_type( et->getDecl() );
        return;
    }
}

void ReflectionVisitor::register_container_type( const QualType& qt )
{
    std::string full_type = get_type_name( qt, pp );
    std::string key       = make_dep_key( full_type );
    if (seen_dep_keys.count( key ))
        return;
    seen_dep_keys.insert( key );

    // Skip types already registered by TypeRegistry::initialize() — these are
    // registered as their proper TypeInfo subclass (StringClass, etc.); a
    // redundant RecordInfo would overwrite the kind. Field TypeIds already
    // hash the full specialization, so they match StringClass's registration.
    if (full_type.find( "basic_string" ) != std::string::npos)
        return;

    uint64_t hash    = compute_type_id_hash( full_type );
    size_t sizeof_ct = ctx_.getTypeSizeInChars( qt ).getQuantity();
    size_t align_ct  = ctx_.getTypeAlignInChars( qt ).getQuantity();

    // std::vector gets VectorClass so RTTI visitation walks its elements —
    // without it every DynamicArray field would serialize as an empty record.
    const bool is_vector       = full_type.rfind( "std::vector<", 0 ) == 0;
    const std::string info_type =
        is_vector ? ( "::nc::rtti::VectorClass<" + full_type + ">" ) : "::nc::rtti::RecordInfo";

    result.dep_types.push_back(
        { key, "    // " + full_type +
                   "\n"
                   "    {\n"
                   "        static " + info_type + " nc_ct(\n"
                   "            \"" +
                   full_type +
                   "\",\n"
                   "            ::nc::rtti::TypeId{" +
                   std::to_string( hash ) +
                   "ULL},\n"
                   "            " +
                   std::to_string( sizeof_ct ) + ", " + std::to_string( align_ct ) +
                   "\n"
                   "        );\n"
                   "        ::nc::rtti::TypeRegistry::register_type( &nc_ct );\n"
                   "    }\n" }
    );
}

void ReflectionVisitor::register_plain_record_type( const CXXRecordDecl* rd, const QualType& qt )
{
    if (!rd->isCompleteDefinition() || rd->isDependentType())
        return;

    // Skip types the record pass itself registers (REFLECT / NSTRUCT_V /
    // ECS_COMPONENT / nc::Object-derived): an empty dep registration would
    // clobber their field lists depending on static-init order.
    if (has_reflect_attr( rd ) || is_ecs_component_base( rd ) || has_nstruct_v_marker( rd ) || inherits_from_object( rd ))
        return;
    for (const auto* field : rd->fields()) {
        if (has_reflect_attr( field ))
            return;
    }

    // Field TypeIds for plain records hash get_qualified_name() (see
    // VisitRecordDecl element_type_name branch) — use the same spelling.
    std::string full_type = get_qualified_name( rd );
    std::string key       = make_dep_key( full_type );
    if (seen_dep_keys.count( key ))
        return;
    seen_dep_keys.insert( key );

    uint64_t hash    = compute_type_id_hash( full_type );
    size_t sizeof_ct = ctx_.getTypeSizeInChars( qt ).getQuantity();
    size_t align_ct  = ctx_.getTypeAlignInChars( qt ).getQuantity();

    // Empty RecordInfo: enough for RTTI lookup and is_record() dispatch; the
    // type round-trips as default-constructed (no serializable fields).
    result.dep_types.push_back(
        { key, "    // " + full_type + " (plain record)\n"
               "    {\n"
               "        static ::nc::rtti::RecordInfo nc_ct(\n"
               "            \"" +
               full_type +
               "\",\n"
               "            ::nc::rtti::TypeId{" +
               std::to_string( hash ) +
               "ULL},\n"
               "            " +
               std::to_string( sizeof_ct ) + ", " + std::to_string( align_ct ) +
               "\n"
               "        );\n"
               "        ::nc::rtti::TypeRegistry::register_type( &nc_ct );\n"
               "    }\n" }
    );
}

void ReflectionVisitor::register_enum_type( const EnumDecl* ed )
{
    std::string qname     = get_qualified_name( ed );
    std::string qname_cpp = qualify_for_cpp( qname );
    std::string key       = make_dep_key( qname.empty() ? ed->getNameAsString() : qname );
    if (seen_dep_keys.count( key ))
        return;
    seen_dep_keys.insert( key );

    // Skip enums with no enumerators (e.g. std::byte) to avoid zero-length arrays.
    if (ed->enumerator_begin() == ed->enumerator_end())
        return;

    bool is_unsigned = ed->getIntegerType()->isUnsignedIntegerType();

    uint64_t hash       = compute_type_id_hash( qname );
    size_t sizeof_enum  = ctx_.getTypeSizeInChars( ed->getIntegerType() ).getQuantity();
    size_t alignof_enum = ctx_.getTypeAlignInChars( ed->getIntegerType() ).getQuantity();

    std::ostringstream oss;
    oss << "    // Enum: " << qname << "\n"
        << "    {\n"
        << "        static const ::nc::rtti::EnumElement " << key << "_elems[] = {\n";

    for (const auto* enumerator : ed->enumerators()) {
        int64_t val = enumerator->getInitVal().getSExtValue();
        oss << "            { \"" << enumerator->getNameAsString() << "\",\n"
            << "              static_cast<int64_t>(" << val << ") },\n";
    }

    oss << "        };\n"
        << "        static ::nc::rtti::EnumInfo nc_enum(\n"
        << "            \"" << qname << "\",\n"
        << "            ::nc::rtti::TypeId{" << hash << "ULL},\n"
        << "            " << sizeof_enum << ", " << alignof_enum << "\n"
        << "        );\n"
        << "        ::nc::rtti::TypeRegistry::register_type( &nc_enum );\n"
        << "        nc_enum.elements_begin = " << key << "_elems;\n"
        << "        nc_enum.elements_end = " << key << "_elems + sizeof(" << key
        << "_elems) / sizeof(::nc::rtti::EnumElement);\n"
        << "        nc_enum.is_unsigned = " << ( is_unsigned ? "true" : "false" ) << ";\n"
        << "    }\n";

    result.dep_types.push_back( { key, oss.str() } );
}

//------------------------------------------------------------------------------

class ReflectionAstConsumer : public ASTConsumer {
public:
    explicit ReflectionAstConsumer( ReflectionResult& out, ReflectionStats& p_stats ) : result( out ), stats( p_stats )
    {}

    void HandleTranslationUnit( ASTContext& Context ) override
    {
        ReflectionVisitor visitor( Context );
        visitor.TraverseDecl( Context.getTranslationUnitDecl() );
        result = std::move( visitor.result );
        stats  = std::move( visitor.stats );
    }

private:
    ReflectionResult& result;
    ReflectionStats& stats;
};

class ReflectionFrontendAction : public ASTFrontendAction {
public:
    explicit ReflectionFrontendAction( ReflectionResult& out, ReflectionStats& p_stats ) :
        result( out ), stats( p_stats )
    {}

    std::unique_ptr<ASTConsumer> CreateASTConsumer( CompilerInstance& CI, llvm::StringRef InFile ) override
    {
        return std::make_unique<ReflectionAstConsumer>( result, stats );
    }

private:
    ReflectionResult& result;
    ReflectionStats& stats;
};

class ReflectionActionFactory : public clang::tooling::FrontendActionFactory {
public:
    explicit ReflectionActionFactory( ReflectionResult& out, ReflectionStats& p_stats ) :
        result( out ), stats( p_stats )
    {}

    std::unique_ptr<FrontendAction> create() override
    {
        return std::make_unique<ReflectionFrontendAction>( result, stats );
    }

private:
    ReflectionResult& result;
    ReflectionStats& stats;
};

std::unique_ptr<clang::tooling::FrontendActionFactory>
create_reflection_action_factory( ReflectionResult& out, ReflectionStats& stats )
{
    return std::make_unique<ReflectionActionFactory>( out, stats );
}
