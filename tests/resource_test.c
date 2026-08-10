#include "haniwa/haniwa.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                                             \
    do {                                                                               \
        if (!(condition)) {                                                            \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #condition);       \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

int main(void) {
    HaniwaResource app = haniwa_resource_for_uri(HANIWA_APP_URI);
    REQUIRE(app.status == 200);
    REQUIRE(app.length > 0);
    REQUIRE(strcmp(app.mime_type, "text/html; charset=utf-8") == 0);
    REQUIRE(strstr((const char *)app.bytes, "haniwa://") != NULL);
    REQUIRE(strstr((const char *)app.bytes, "file://") != NULL);

    HaniwaResource missing = haniwa_resource_for_uri("haniwa://app/missing.html");
    REQUIRE(missing.status == 404);
    REQUIRE(missing.length > 0);
    return 0;
}
