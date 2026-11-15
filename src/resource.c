#include "haniwa/haniwa.h"

#include <string.h>

static const unsigned char app_page[] =
    "<!doctype html>"
    "<html lang=\"en\">"
    "<meta charset=\"utf-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<title>Haniwa</title>"
    "<style>"
    ":root{color-scheme:light dark;font-family:system-ui,sans-serif}"
    "body{margin:0;min-height:100vh;display:grid;place-items:center;background:#17120f;"
    "color:#f4e8da}"
    "main{max-width:34rem;padding:3rem}"
    "h1{font-size:clamp(3rem,10vw,6rem);margin:0;letter-spacing:-.06em}"
    "p{font-size:1.1rem;line-height:1.6;color:#d7c3b1}"
    "code{color:#ffb36b}"
    "</style>"
    "<main><h1>Haniwa</h1>"
    "<p>The native host owns this window and event loop. The system web view "
    "loaded this embedded page through <code>haniwa://</code>; no browser "
    "engine or <code>file://</code> authority was bundled.</p></main>"
    "</html>";

static const unsigned char not_found_page[] =
    "<!doctype html><meta charset=\"utf-8\"><title>Not found</title>"
    "<h1>404: Haniwa resource not found</h1>";

HaniwaResource haniwa_resource_for_uri(const char *uri) {
    if (uri != NULL && strcmp(uri, HANIWA_APP_URI) == 0) {
        HaniwaResource resource = {
            app_page,
            sizeof(app_page) - 1,
            "text/html; charset=utf-8",
            200,
        };
        return resource;
    }

    HaniwaResource resource = {
        not_found_page,
        sizeof(not_found_page) - 1,
        "text/html; charset=utf-8",
        404,
    };
    return resource;
}
