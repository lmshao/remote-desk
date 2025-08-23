/**
 * @author SHAO Liming <lmshao@163.com>
 * @copyright Copyright (c) 2025 SHAO Liming
 * @license MIT
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef LMSHAO_REMOTE_DESK_LOG_H
#define LMSHAO_REMOTE_DESK_LOG_H

#include <chrono>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace lmshao::remotedesk {

#ifdef _WIN32
#define COLOR_RED ""
#define COLOR_YELLOW ""
#define COLOR_RESET ""
#define FUNC_NAME_ __FUNCTION__
#define FILENAME_ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define REMOTEDESK_LOG_TIME_STR                                                                                        \
    ([]() -> const char * {                                                                                            \
        using namespace std::chrono;                                                                                   \
        static char buf[32];                                                                                           \
        auto now = system_clock::now();                                                                                \
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;                                          \
        std::time_t t = system_clock::to_time_t(now);                                                                  \
        std::tm tm;                                                                                                    \
        localtime_s(&tm, &t);                                                                                          \
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);                                                     \
        std::snprintf(buf + 19, sizeof(buf) - 19, ".%03d", static_cast<int>(ms.count()));                              \
        return buf;                                                                                                    \
    })()
#else
#define COLOR_RED "\033[31m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_RESET "\033[0m"
#define FUNC_NAME_ __FUNCTION__
#define FILENAME_ (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#define REMOTEDESK_LOG_TIME_STR                                                                                        \
    ([]() -> const char * {                                                                                            \
        using namespace std::chrono;                                                                                   \
        static char buf[32];                                                                                           \
        auto now = system_clock::now();                                                                                \
        auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;                                          \
        std::time_t t = system_clock::to_time_t(now);                                                                  \
        std::tm tm = *std::localtime(&t);                                                                              \
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);                                                     \
        std::snprintf(buf + 19, sizeof(buf) - 19, ".%03d", static_cast<int>(ms.count()));                              \
        return buf;                                                                                                    \
    })()
#endif

#ifdef DEBUG
#define LOG_DEBUG(fmt, ...)                                                                                            \
    printf("%s [DEBUG] %s:%d  %s() " fmt "\n", REMOTEDESK_LOG_TIME_STR, FILENAME_, __LINE__, FUNC_NAME_, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)                                                                                             \
    printf(COLOR_YELLOW "%s [WARN] %s:%d %s() " fmt "\n" COLOR_RESET, REMOTEDESK_LOG_TIME_STR, FILENAME_, __LINE__,    \
           FUNC_NAME_, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)                                                                                            \
    printf(COLOR_RED "%s [ERROR] %s:%d %s() " fmt "\n" COLOR_RESET, REMOTEDESK_LOG_TIME_STR, FILENAME_, __LINE__,      \
           FUNC_NAME_, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#define LOG_WARN(fmt, ...)
#define LOG_ERROR(fmt, ...)                                                                                            \
    printf(COLOR_RED "%s [ERROR] %s:%d %s() " fmt "\n" COLOR_RESET, REMOTEDESK_LOG_TIME_STR, FILENAME_, __LINE__,      \
           FUNC_NAME_, ##__VA_ARGS__)
#endif

} // namespace lmshao::remotedesk

#endif // LMSHAO_REMOTE_DESK_LOG_H