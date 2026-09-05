#pragma once

// MaterialTemplate has been removed (Esoterica-style materials).
// Use Shader as MaterialComponent::Source and MaterialCreateDesc for material_create().
// See ncore/include/ncore/resources/material_shader.h and docs/materials_esoterica.md

#error "MaterialTemplate was removed. Include material_shader.h and use Shader + MaterialCreateDesc."
