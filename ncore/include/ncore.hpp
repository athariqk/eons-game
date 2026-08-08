// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <ncore/application.h>
#include <ncore/core/errors.h>
#include <ncore/core/matrix.h>
#include <ncore/core/object.h>
#include <ncore/core/random.h>
#include <ncore/core/rect.h>
#include <ncore/core/reference.h>
#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/game_world.h>
#include <ncore/resources/mesh.h>
#include <ncore/runtime/components/material.h>
#include <ncore/runtime/components/mesh.h>
#include <ncore/runtime/components/resource.h>
#include <ncore/runtime/components/rigidbody.h>
#include <ncore/runtime/components/sprite.h>
#include <ncore/runtime/components/transform.h>
#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_system.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>
#include <ncore/services/io/resource_service.h>
#include <ncore/services/service.h>
#include <ncore/services/service_registry.h>
#include <ncore/services/video/render_service.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>
