#ifndef C_LOGGER_LOGGER_H
#define C_LOGGER_LOGGER_H

#define ERROR_TYPE "ERROR: "
#define INFO_TYPE "INFO: "

#ifndef NDEBUG

#define LOG_INFO(file, message, ...) \
    logger_log(file, INFO_TYPE, message, ##__VA_ARGS__) \

#define LOG_ERROR(file, message, ...) \
    logger_log(file, ERROR_TYPE, message, ##__VA_ARGS__) \

#else

#define LOG_INFO(file, message, ...)

#define LOG_ERROR(file, message, ...)

#endif

void logger_log(FILE* file, const char* type, const char* message, ...);

#endif //C_LOGGER_LOGGER_H
