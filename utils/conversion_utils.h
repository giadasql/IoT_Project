#ifndef CONVERSION_UTILS_H
#define CONVERSION_UTILS_H

#include <stddef.h>

// Conversion Function Prototypes
void boolean_to_string(char *buffer, size_t size, void *state);
void boolean_update_state(const char *payload, void *state);

void integer_to_string(char *buffer, size_t size, void *state);
void integer_update_state(const char *payload, void *state);

#endif // CONVERSION_UTILS_H
