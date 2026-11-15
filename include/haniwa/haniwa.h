#ifndef HANIWA_H
#define HANIWA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HANIWA_APP_URI "haniwa://app/index.html"
#define HANIWA_VERSION "0.1.0"

typedef struct HaniwaResource {
    const unsigned char *bytes;
    size_t length;
    const char *mime_type;
    int status;
} HaniwaResource;

/* Resolves an embedded application resource without touching the filesystem. */
HaniwaResource haniwa_resource_for_uri(const char *uri);

/* Starts the platform event loop and owns it until the last window closes. */
int haniwa_run(int smoke_test);

#ifdef __cplusplus
}
#endif

#endif
