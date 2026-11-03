// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/application.h>
#include <ncore/core/errors.h>
#include <ncore/core/random.h>
#include <ncore/game_world.h>
#include <ncore/object/object_info.h>
#include <ncore/object/reference.h>
#include <ncore/runtime/components.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/io/resource_archive.h>
#include <ncore/runtime/io/resource_loader.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/resources/audio_clip.h>
#include <ncore/runtime/resources/cubemap.h>
#include <ncore/runtime/resources/font.h>
#include <ncore/runtime/resources/image.h>
#include <ncore/runtime/resources/material.h>
#include <ncore/runtime/resources/mesh.h>
#include <ncore/runtime/resources/shader.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/asset_service.h>
#include <ncore/services/service.h>
#include <ncore/services/servicedb.h>
#include <ncore/services/video/render_service.h>
#include <ncore/structs/matrix.h>
#include <ncore/structs/rect.h>
#include <ncore/structs/rid.h>
#include <ncore/types/typedb.h>
#include <ncore/types/typeinfo.h>
#include <ncore/types/types.h>
#include <ncore/structs/vector.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>
