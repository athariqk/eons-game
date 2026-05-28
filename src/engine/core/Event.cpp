#include "Event.h"

#include <format>

BaseEvent::~BaseEvent() = default;

std::string MouseEvent::ToString() {
    return std::format("MouseEvent<is_panning={},rel_motion={},m_button={}>", m_isPanning, m_relativeMotion.ToString(),
                       m_mouseButton);
}

std::string KeyEvent::ToString() {
    return std::format("KeyEvent<type={},key={},is_repeat={}>", (int)m_type, (int)m_key, (int)m_repeat);
}
