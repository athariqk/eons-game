#include "InputEvents.h"

namespace ncore {

std::string MouseMotionEvent::ToString() {
    return std::format("MouseMotionEvent<position={},delta={}>", position.to_string(), delta.to_string());
}

std::string KeyboardEvent::ToString() {
    return std::format("KeyboardEvent<action={},key={},repeat={}>", (int) action, (int) key, (int) repeat);
}

} // namespace ncore
