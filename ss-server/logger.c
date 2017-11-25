//
// Created by marius on 11/25/17.
//

#include <time.h>
#include <memory.h>
#include "logger.h"

static const char *level_to_str(int level) {
    switch (level) {
        case LEVEL_DEBUG:
            return "DEBUG";
        case LEVEL_INFO:
            return "INFO";
        case LEVEL_WARNING:
            return "WARNING";
        case LEVEL_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

file_logger *new_logger(char *file_name) {
    file_logger *logger = malloc(sizeof(logger));
    logger->file = fopen(file_name, "a");
    return logger;
}

void destroy_logger(file_logger *this) {
    fclose(this->file);
    free(this);
}

void __log(file_logger *this, const char *message, int level) {
    time_t currentTime;
    time(&currentTime);
    char timeStr[30];
    strcpy(timeStr, ctime(&currentTime));
    timeStr[strlen(timeStr) - 2] = '\0';

    fprintf(this->file, "[%s].%s : %s\n", timeStr, level_to_str(level), message);
}

void debug(file_logger *this, const char *message) {
    __log(this, message, LEVEL_DEBUG);
}

void info(file_logger *this, const char *message) {
    __log(this, message, LEVEL_INFO);
}

void warning(file_logger *this, const char *message) {
    __log(this, message, LEVEL_WARNING);
}

void error(file_logger *this, const char *message) {
    __log(this, message, LEVEL_ERROR);
}
