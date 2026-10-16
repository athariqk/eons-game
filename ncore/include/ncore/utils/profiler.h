#pragma once

// Thin Tracy instrumentation wrappers for NCORE.
// When NCORE_ENABLE_TRACY is not defined, all macros expand to nothing.

#if defined( NCORE_ENABLE_TRACY )

#include <tracy/Tracy.hpp>

#define NC_ZONE_SCOPED              ZoneScoped
#define NC_ZONE_NAMED( name )       ZoneScopedN( name )
#define NC_ZONE_COLOR( name, color ) ZoneScopedNC( name, color )
#define NC_ZONE_TEXT( text, size )  ZoneText( text, size )
#define NC_ZONE_VALUE( value )      ZoneValue( value )
#define NC_FRAME_MARK               FrameMark
#define NC_FRAME_MARK_NAMED( name ) FrameMarkNamed( name )
#define NC_PLOT( name, value )      TracyPlot( name, value )
#define NC_PLOT_CONFIG( name, type ) TracyPlotConfig( name, type )

// Tracy color helpers (0xRRGGBB)
#define NC_COLOR_ECS      0x4EC9B0
#define NC_COLOR_RENDER   0x569CD6
#define NC_COLOR_AUDIO    0xC586C0
#define NC_COLOR_INPUT    0xDCDCAA
#define NC_COLOR_APP      0xCE9178
#define NC_COLOR_RESOURCE 0xB5CEA8

#else // !NCORE_ENABLE_TRACY

#define NC_ZONE_SCOPED
#define NC_ZONE_NAMED( name )
#define NC_ZONE_COLOR( name, color )
#define NC_ZONE_TEXT( text, size )
#define NC_ZONE_VALUE( value )
#define NC_FRAME_MARK
#define NC_FRAME_MARK_NAMED( name )
#define NC_PLOT( name, value )
#define NC_PLOT_CONFIG( name, type )

#define NC_COLOR_ECS      0
#define NC_COLOR_RENDER   0
#define NC_COLOR_AUDIO    0
#define NC_COLOR_INPUT    0
#define NC_COLOR_APP      0
#define NC_COLOR_RESOURCE 0

#endif // NCORE_ENABLE_TRACY
