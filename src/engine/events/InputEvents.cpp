#include "InputEvents.h"

namespace Aeon {

std::string MouseMotionEvent::ToString() {
    return std::format("MouseMotionEvent<position={},delta={}>", position.ToString(), delta.ToString());
}

std::string KeyboardEvent::ToString() {
    return std::format("KeyboardEvent<action={},key={},repeat={}>", (int) action, (int) key, (int) repeat);
}

} // namespace Aeon
