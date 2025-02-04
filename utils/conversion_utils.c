#include "conversion_utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Boolean Conversion
void boolean_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%s", (*(int *)state) == 1 ? "true" : "false");

    printf("Boolean to string: %s\n", buffer);
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

// Decimal Conversion
void decimal_to_string(char *buffer, size_t size, void *state) {
    snprintf(buffer, size, "%.2f", *(float *)state);
}

void decimal_update_state(const char *payload, void *state) {
    *(float *)state = atof(payload);
}
