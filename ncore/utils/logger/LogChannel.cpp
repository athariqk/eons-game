#include "LogChannel.h"

namespace ncore::log {

LogChannel::LogChannel() { name = DEFAULT.data(); }

LogChannel::LogChannel(std::string_view name, std::vector<std::shared_ptr<Sink>> p_sinks) :
    name(name), sinks(p_sinks) {}

void LogChannel::flush() {
    for (auto &sink: sinks)
        sink->flush();
}

void LogChannel::set_level(Level p_level) { level = p_level; }

} // namespace ncore::log
