#pragma once

namespace nc {

enum class WindowFlag : uint8_t {
    RESIZABLE = 1 << 0
};

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
