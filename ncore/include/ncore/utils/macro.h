#include <ncore/utils/logger/logger.h>

#define NC_CONCAT_IMPL(a, b) a##b
#define NC_CONCAT(a, b) NC_CONCAT_IMPL(a, b)

/* --------------- LOGGERS -------------------------- */

#ifndef NC_LOG_CHANNEL_NAME
#define NC_LOG_CHANNEL_NAME ncore::log::DEFAULT
#endif

#define NC_LOG_TRACE_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().channel(cat)->write(ncore::log::Level::Trace, ncore::log::SourceLoc{},          \
                                                           __VA_ARGS__)
#define NC_LOG_DEBUG_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().channel(cat)->write(ncore::log::Level::Debug, ncore::log::SourceLoc{},          \
                                                           __VA_ARGS__)
#define NC_LOG_INFO_C(cat, ...)                                                                                        \
    ncore::log::Logger::get_instance().channel(cat)->write(ncore::log::Level::Info, ncore::log::SourceLoc{},           \
                                                           __VA_ARGS__)
#define NC_LOG_WARN_C(cat, ...)                                                                                        \
    ncore::log::Logger::get_instance().channel(cat)->write(ncore::log::Level::Warn, ncore::log::SourceLoc{},           \
                                                           __VA_ARGS__)
#define NC_LOG_ERROR_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().channel(cat)->write(                                                            \
        ncore::log::Level::Error, ncore::log::SourceLoc{__FILE__, __func__, __LINE__}, __VA_ARGS__)
#define NC_LOG_FATAL_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().channel(cat)->write(                                                            \
        ncore::log::Level::Fatal, ncore::log::SourceLoc{__FILE__, __func__, __LINE__}, __VA_ARGS__)

#define NC_LOG_TRACE(...) NC_LOG_TRACE_C(NC_LOG_CHANNEL_NAME, __VA_ARGS__)
#define NC_LOG_DEBUG(...) NC_LOG_DEBUG_C(NC_LOG_CHANNEL_NAME, __VA_ARGS__)
#define NC_LOG_INFO(...) NC_LOG_INFO_C(NC_LOG_CHANNEL_NAME, __VA_ARGS__)
#define NC_LOG_WARN(...) NC_LOG_WARN_C(NC_LOG_CHANNEL_NAME, __VA_ARGS__)
#define NC_LOG_ERROR(...) NC_LOG_ERROR_C(NC_LOG_CHANNEL_NAME, __VA_ARGS__)
#define NC_LOG_FATAL(...) NC_LOG_FATAL_C(NC_LOG_CHANNEL_NAME, __VA_ARGS__)
