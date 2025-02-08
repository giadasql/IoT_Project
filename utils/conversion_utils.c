#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// Boolean Conversion
void boolean_to_string(char *buffer, size_t size, void *state) {
    bool value = *(bool *)state;
    snprintf(buffer, size, "%s", value ? "true" : "false");
}

void boolean_update_state(const char *payload, void *state) {
    *(int *)state = (strcmp(payload, "true") == 0) ? 1 : 0;
}

// Integer Conversion
void integer_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%d", *(int *)state);
}

void integer_update_state(const char *payload, void *state) {
    *(int *)state = atoi(payload);
}
