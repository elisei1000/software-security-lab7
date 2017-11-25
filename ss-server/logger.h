//
// Created by marius on 11/25/17.
//

#ifndef SS_SERVER_LOGGER_H
#define SS_SERVER_LOGGER_H

#include <stdio.h>
#include <malloc.h>

#define LEVEL_DEBUG    4
#define LEVEL_INFO     3
#define LEVEL_WARNING  2
#define LEVEL_ERROR    1

typedef struct {
    FILE *file;
} file_logger;

file_logger *new_logger(char *file_name);

void destroy_logger(file_logger *this);

/*
 * Private methods
 */
void __log(file_logger *this, const char *message, int level);

/*
 * Public methods
 */
void debug(file_logger *this, const char *message);

void info(file_logger *this, const char *message);

void warning(file_logger *this, const char *message);

void error(file_logger *this, const char *message);

#endif //SS_SERVER_LOGGER_H
