#include "haniwa/haniwa.h"

#import <AppKit/AppKit.h>
#import <WebKit/WebKit.h>

@interface HaniwaSchemeHandler : NSObject <WKURLSchemeHandler>
@end

@implementation HaniwaSchemeHandler
- (void)webView:(WKWebView *)webView startURLSchemeTask:(id<WKURLSchemeTask>)task {
    (void)webView;
    const char *uri = task.request.URL.absoluteString.UTF8String;
    HaniwaResource resource = haniwa_resource_for_uri(uri);
    NSData *body = [NSData dataWithBytes:resource.bytes length:resource.length];
    NSDictionary<NSString *, NSString *> *headers = @{
        @"Content-Type" : [NSString stringWithUTF8String:resource.mime_type],
        @"Cache-Control" : @"no-store",
    };
    NSHTTPURLResponse *response =
        [[NSHTTPURLResponse alloc] initWithURL:task.request.URL
                                    statusCode:resource.status
                                   HTTPVersion:@"HTTP/1.1"
                                  headerFields:headers];
    [task didReceiveResponse:response];
    [task didReceiveData:body];
    [task didFinish];
}

- (void)webView:(WKWebView *)webView stopURLSchemeTask:(id<WKURLSchemeTask>)task {
    (void)webView;
    (void)task;
}
@end

static int haniwa_exit_code = 0;

@interface HaniwaAppDelegate : NSObject <NSApplicationDelegate, WKNavigationDelegate>
@property(nonatomic, strong) NSWindow *window;
@property(nonatomic, strong) WKWebView *webView;
@property(nonatomic, strong) HaniwaSchemeHandler *schemeHandler;
@property(nonatomic) BOOL smokeTest;
@end

@implementation HaniwaAppDelegate
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    NSRect frame = NSMakeRect(0, 0, 880, 600);
    self.window = [[NSWindow alloc]
        initWithContentRect:frame
                  styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                             NSWindowStyleMaskMiniaturizable |
                             NSWindowStyleMaskResizable)
                    backing:NSBackingStoreBuffered
                      defer:NO];
    self.window.title = @"Haniwa";

    WKWebViewConfiguration *configuration = [[WKWebViewConfiguration alloc] init];
    self.schemeHandler = [[HaniwaSchemeHandler alloc] init];
    [configuration setURLSchemeHandler:self.schemeHandler forURLScheme:@"haniwa"];
    self.webView = [[WKWebView alloc] initWithFrame:frame configuration:configuration];
    self.webView.navigationDelegate = self;
    self.window.contentView = self.webView;
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
    [self.webView
        loadRequest:[NSURLRequest
                        requestWithURL:[NSURL URLWithString:@HANIWA_APP_URI]]];
    if (self.smokeTest) {
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(15 * NSEC_PER_SEC)),
                       dispatch_get_main_queue(), ^{
                         if (self.window.isVisible) {
                             [self.window close];
                         }
                       });
    }
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation {
    (void)webView;
    (void)navigation;
    if (self.smokeTest) {
        haniwa_exit_code = 0;
        [self.window close];
    }
}

- (void)webView:(WKWebView *)webView
    didFailNavigation:(WKNavigation *)navigation
            withError:(NSError *)error {
    (void)webView;
    (void)navigation;
    (void)error;
    if (self.smokeTest) {
        [self.window close];
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)app {
    (void)app;
    return YES;
}
@end

int haniwa_run(int smoke_test) {
    @autoreleasepool {
        haniwa_exit_code = smoke_test ? 1 : 0;
        NSApplication *app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        HaniwaAppDelegate *delegate = [[HaniwaAppDelegate alloc] init];
        delegate.smokeTest = smoke_test != 0;
        app.delegate = delegate;
        [app run];
    }
    return haniwa_exit_code;
}
