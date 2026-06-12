#include <cstdio>
#include <cstring>

#include "Assert.h"
#include "logger/Logger.h"

#define NC_FE_1(M, S, a) M(S, a)
#define NC_FE_2(M, S, a, ...) M(S, a) NC_FE_1(M, S, __VA_ARGS__)
#define NC_FE_3(M, S, a, ...) M(S, a) NC_FE_2(M, S, __VA_ARGS__)
#define NC_FE_4(M, S, a, ...) M(S, a) NC_FE_3(M, S, __VA_ARGS__)
#define NC_FE_5(M, S, a, ...) M(S, a) NC_FE_4(M, S, __VA_ARGS__)
#define NC_FE_6(M, S, a, ...) M(S, a) NC_FE_5(M, S, __VA_ARGS__)
#define NC_FE_7(M, S, a, ...) M(S, a) NC_FE_6(M, S, __VA_ARGS__)
#define NC_FE_8(M, S, a, ...) M(S, a) NC_FE_7(M, S, __VA_ARGS__)

#define NC_FE_SEL(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define NC_FOR_EACH(M, S, ...)                                                                                         \
    NC_FE_SEL(__VA_ARGS__, NC_FE_8, NC_FE_7, NC_FE_6, NC_FE_5, NC_FE_4, NC_FE_3, NC_FE_2, NC_FE_1)(M, S, __VA_ARGS__)

/*  ---------------- SERIALIZER/DESERIALIZER MACRO -------------------- */

#define NC_INIFILE_WRITE(SECTION, FIELD) ini[#SECTION][#FIELD] = FIELD;
#define NC_INIFILE_READ(SECTION, FIELD) FIELD = ini[#SECTION][#FIELD].as<decltype(FIELD)>();
#define NC_PRINT_FIELD(SECTION, FIELD) ss << "\n\t" << #FIELD " = " << FIELD;

#define NC_DEF_CFG_MAP(TYPE_NAME, ...)                                                                                 \
    void serialize(ini::IniFile &ini) const { NC_FOR_EACH(NC_INIFILE_WRITE, TYPE_NAME, __VA_ARGS__) }                  \
    void deserialize(ini::IniFile &ini){NC_FOR_EACH(NC_INIFILE_READ, TYPE_NAME, __VA_ARGS__)} std::string print()      \
        const {                                                                                                        \
        std::ostringstream ss;                                                                                         \
        ss << #TYPE_NAME;                                                                                              \
        NC_FOR_EACH(NC_PRINT_FIELD, TYPE_NAME, __VA_ARGS__)                                                            \
        return ss.str();                                                                                               \
    }

/* --------------- LOGGERS -------------------------- */

#ifndef NC_LOG_CHANNEL
#define NC_LOG_CHANNEL ncore::log::DEFAULT
#endif

#define NC_LOG_TRACE_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().get_or_create(cat)->log(ncore::log::Level::Trace, ncore::log::SourceLoc{},      \
                                                               __VA_ARGS__)
#define NC_LOG_DEBUG_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().get_or_create(cat)->log(ncore::log::Level::Debug, ncore::log::SourceLoc{},      \
                                                               __VA_ARGS__)
#define NC_LOG_INFO_C(cat, ...)                                                                                        \
    ncore::log::Logger::get_instance().get_or_create(cat)->log(ncore::log::Level::Info, ncore::log::SourceLoc{},       \
                                                               __VA_ARGS__)
#define NC_LOG_WARN_C(cat, ...)                                                                                        \
    ncore::log::Logger::get_instance().get_or_create(cat)->log(ncore::log::Level::Warn, ncore::log::SourceLoc{},       \
                                                               __VA_ARGS__)
#define NC_LOG_ERROR_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().get_or_create(cat)->log(                                                        \
        ncore::log::Level::Error, ncore::log::SourceLoc{__FILE__, __func__, __LINE__}, __VA_ARGS__)
#define NC_LOG_FATAL_C(cat, ...)                                                                                       \
    ncore::log::Logger::get_instance().get_or_create(cat)->log(                                                        \
        ncore::log::Level::Fatal, ncore::log::SourceLoc{__FILE__, __func__, __LINE__}, __VA_ARGS__)

#define NC_LOG_TRACE(...) NC_LOG_TRACE_C(NC_LOG_CHANNEL, __VA_ARGS__)
#define NC_LOG_DEBUG(...) NC_LOG_DEBUG_C(NC_LOG_CHANNEL, __VA_ARGS__)
#define NC_LOG_INFO(...) NC_LOG_INFO_C(NC_LOG_CHANNEL, __VA_ARGS__)
#define NC_LOG_WARN(...) NC_LOG_WARN_C(NC_LOG_CHANNEL, __VA_ARGS__)
#define NC_LOG_ERROR(...) NC_LOG_ERROR_C(NC_LOG_CHANNEL, __VA_ARGS__)
#define NC_LOG_FATAL(...) NC_LOG_FATAL_C(NC_LOG_CHANNEL, __VA_ARGS__)
