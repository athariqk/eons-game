#include <cstdio>
#include <cstring>

#ifndef __FILE_NAME__
#ifdef _WIN32
// Workaround retrieved from https://stackoverflow.com/a/8488201
#define __FILE_NAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#else
#define __FILE_NAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#endif
#endif

#define NC_ASSERT_RET(expr, msg)                                                                                       \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            fprintf(stderr, "%s:%d. FALSE: %s\n. %s\n", __FILE_NAME__, __LINE__, #expr, msg);                          \
            return;                                                                                                    \
        }                                                                                                              \
    } while (0)

#define NC_ASSERT_RETVAL(expr, ret_val, msg)                                                                           \
    do {                                                                                                               \
        if (!(expr)) {                                                                                                 \
            fprintf(stderr, "%s:%d. FALSE: %s\n. %s\n", __FILE_NAME__, __LINE__, #expr, msg);                          \
            return ret_val;                                                                                            \
        }                                                                                                              \
    } while (0)

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
