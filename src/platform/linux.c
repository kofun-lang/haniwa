#include "haniwa/haniwa.h"

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

static void handle_haniwa_uri(WebKitURISchemeRequest *request, void *user_data) {
    (void)user_data;
    const char *uri = webkit_uri_scheme_request_get_uri(request);
    HaniwaResource resource = haniwa_resource_for_uri(uri);
    GInputStream *stream = g_memory_input_stream_new_from_data(
        resource.bytes, (gssize)resource.length, NULL);
    webkit_uri_scheme_request_finish(request, stream, (gint64)resource.length,
                                     resource.mime_type);
    g_object_unref(stream);
}

typedef struct SmokeState {
    GtkWidget *window;
    int exit_code;
    guint timeout_id;
} SmokeState;

static void cancel_smoke_timeout(SmokeState *state) {
    if (state->timeout_id != 0) {
        g_source_remove(state->timeout_id);
        state->timeout_id = 0;
    }
}

static void smoke_load_changed(WebKitWebView *web_view, WebKitLoadEvent event,
                               void *user_data) {
    (void)web_view;
    SmokeState *state = user_data;
    if (event == WEBKIT_LOAD_FINISHED) {
        state->exit_code = 0;
        cancel_smoke_timeout(state);
        gtk_widget_destroy(state->window);
    }
}

static gboolean smoke_load_failed(WebKitWebView *web_view, WebKitLoadEvent event,
                                  const char *failing_uri, GError *error,
                                  void *user_data) {
    (void)web_view;
    (void)event;
    (void)failing_uri;
    (void)error;
    SmokeState *state = user_data;
    cancel_smoke_timeout(state);
    gtk_widget_destroy(state->window);
    return TRUE;
}

static gboolean smoke_timeout(void *user_data) {
    SmokeState *state = user_data;
    state->timeout_id = 0;
    gtk_widget_destroy(state->window);
    return G_SOURCE_REMOVE;
}

int haniwa_run(int smoke_test) {
    gtk_init(NULL, NULL);

    WebKitWebContext *context = webkit_web_context_new();
    webkit_web_context_register_uri_scheme(context, "haniwa", handle_haniwa_uri, NULL,
                                           NULL);
    WebKitSecurityManager *security = webkit_web_context_get_security_manager(context);
    webkit_security_manager_register_uri_scheme_as_local(security, "haniwa");
    webkit_security_manager_register_uri_scheme_as_secure(security, "haniwa");

    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Haniwa");
    gtk_window_set_default_size(GTK_WINDOW(window), 880, 600);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    GtkWidget *web_view = webkit_web_view_new_with_context(context);
    gtk_container_add(GTK_CONTAINER(window), web_view);
    SmokeState smoke = {
        .window = window,
        .exit_code = smoke_test ? 1 : 0,
        .timeout_id = 0,
    };
    if (smoke_test) {
        g_signal_connect(web_view, "load-changed", G_CALLBACK(smoke_load_changed),
                         &smoke);
        g_signal_connect(web_view, "load-failed", G_CALLBACK(smoke_load_failed),
                         &smoke);
        smoke.timeout_id = g_timeout_add_seconds(15, smoke_timeout, &smoke);
    }
    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(web_view), HANIWA_APP_URI);

    gtk_widget_show_all(window);
    gtk_main();
    g_object_unref(context);
    return smoke.exit_code;
}
