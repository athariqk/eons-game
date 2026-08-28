#pragma once

namespace nc {

enum class WindowMode {
    WINDOWED,
    MAXIMIZED,
    FULLSCREEN
};
NENUM(
    WindowMode, NENUM_ELEMENT( WindowMode, WINDOWED ), NENUM_ELEMENT( WindowMode, MAXIMIZED ),
    NENUM_ELEMENT( WindowMode, FULLSCREEN )
);

enum class CursorType : uint8_t {
    DEFAULT = 0,
    TEXT,
    CROSSHAIR,
    POINTER,
    RESIZE_NS,   // horizontal resize
    RESIZE_EW,   // vertical resize
    RESIZE_NWSE, // top left/bottom right diagonal resize
    RESIZE_NESW, // top right/bottom left diagonal resize
    WAIT,
    COUNT
};

enum class MessageBoxType : uint8_t {
    INFO,
    WARNING,
    ERROR,
    COUNT
};

} // namespace nc
