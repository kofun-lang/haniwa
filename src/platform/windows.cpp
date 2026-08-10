#include "haniwa/haniwa.h"

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wrl.h>

#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"

#include <cstring>
#include <string>

using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;

namespace {
constexpr wchar_t kWindowClass[] = L"HaniwaWindow";
HWND window_handle = nullptr;
ComPtr<ICoreWebView2Controller> webview_controller;
ComPtr<ICoreWebView2> webview;
bool smoke_test = false;
int exit_code = 0;

void fail(const wchar_t *message) {
    MessageBoxW(window_handle, message, L"Haniwa", MB_OK | MB_ICONERROR);
    PostQuitMessage(1);
}

void resize_webview() {
    if (webview_controller == nullptr || window_handle == nullptr) {
        return;
    }
    RECT bounds;
    GetClientRect(window_handle, &bounds);
    webview_controller->put_Bounds(bounds);
}

HRESULT respond_to_resource(ICoreWebView2Environment *environment,
                            ICoreWebView2WebResourceRequestedEventArgs *args) {
    ComPtr<ICoreWebView2WebResourceRequest> request;
    HRESULT result = args->get_Request(&request);
    if (FAILED(result)) {
        return result;
    }

    wchar_t *wide_uri = nullptr;
    result = request->get_Uri(&wide_uri);
    if (FAILED(result)) {
        return result;
    }
    int size =
        WideCharToMultiByte(CP_UTF8, 0, wide_uri, -1, nullptr, 0, nullptr, nullptr);
    std::string uri(static_cast<size_t>(size > 0 ? size : 0), '\0');
    if (size > 1) {
        WideCharToMultiByte(CP_UTF8, 0, wide_uri, -1, uri.data(), size, nullptr,
                            nullptr);
        uri.resize(static_cast<size_t>(size - 1));
    }
    CoTaskMemFree(wide_uri);

    HaniwaResource resource = haniwa_resource_for_uri(uri.c_str());
    ComPtr<IStream> stream;
    stream.Attach(
        SHCreateMemStream(resource.bytes, static_cast<UINT>(resource.length)));
    if (stream == nullptr) {
        return E_OUTOFMEMORY;
    }

    std::wstring headers = L"Content-Type: text/html; charset=utf-8\r\n"
                           L"Cache-Control: no-store";
    ComPtr<ICoreWebView2WebResourceResponse> response;
    result = environment->CreateWebResourceResponse(
        stream.Get(), resource.status, resource.status == 200 ? L"OK" : L"Not Found",
        headers.c_str(), &response);
    if (FAILED(result)) {
        return result;
    }
    return args->put_Response(response.Get());
}

void create_webview() {
    auto scheme = Make<CoreWebView2CustomSchemeRegistration>(L"haniwa");
    scheme->put_TreatAsSecure(TRUE);
    scheme->put_HasAuthorityComponent(TRUE);

    auto options = Make<CoreWebView2EnvironmentOptions>();
    ComPtr<ICoreWebView2EnvironmentOptions4> options4;
    if (FAILED(options.As(&options4))) {
        fail(L"The WebView2 SDK does not support custom schemes.");
        return;
    }
    ICoreWebView2CustomSchemeRegistration *registrations[] = {scheme.Get()};
    if (FAILED(options4->SetCustomSchemeRegistrations(1, registrations))) {
        fail(L"Could not register the haniwa:// scheme.");
        return;
    }

    HRESULT result = CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, options.Get(),
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT environment_result,
               ICoreWebView2Environment *environment) -> HRESULT {
                if (FAILED(environment_result) || environment == nullptr) {
                    fail(L"Microsoft Edge WebView2 Runtime is required.");
                    return environment_result;
                }
                ComPtr<ICoreWebView2Environment> captured_environment = environment;
                return environment->CreateCoreWebView2Controller(
                    window_handle,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [captured_environment](
                            HRESULT controller_result,
                            ICoreWebView2Controller *controller) -> HRESULT {
                            if (FAILED(controller_result) || controller == nullptr) {
                                fail(L"Could not create the WebView2 controller.");
                                return controller_result;
                            }
                            webview_controller = controller;
                            controller->get_CoreWebView2(&webview);
                            resize_webview();
                            webview->AddWebResourceRequestedFilter(
                                L"haniwa://*", COREWEBVIEW2_WEB_RESOURCE_CONTEXT_ALL);
                            EventRegistrationToken token;
                            HRESULT handler_result = webview->add_WebResourceRequested(
                                Callback<ICoreWebView2WebResourceRequestedEventHandler>(
                                    [captured_environment](
                                        ICoreWebView2 *,
                                        ICoreWebView2WebResourceRequestedEventArgs
                                            *args) -> HRESULT {
                                        return respond_to_resource(
                                            captured_environment.Get(), args);
                                    })
                                    .Get(),
                                &token);
                            if (FAILED(handler_result)) {
                                fail(L"Could not install the haniwa:// handler.");
                                return handler_result;
                            }
                            if (smoke_test) {
                                EventRegistrationToken navigation_token;
                                webview->add_NavigationCompleted(
                                    Callback<
                                        ICoreWebView2NavigationCompletedEventHandler>(
                                        [](ICoreWebView2 *,
                                           ICoreWebView2NavigationCompletedEventArgs
                                               *args) -> HRESULT {
                                            BOOL succeeded = FALSE;
                                            args->get_IsSuccess(&succeeded);
                                            exit_code = succeeded ? 0 : 1;
                                            DestroyWindow(window_handle);
                                            return S_OK;
                                        })
                                        .Get(),
                                    &navigation_token);
                                SetTimer(window_handle, 1, 15000, nullptr);
                            }
                            return webview->Navigate(L"haniwa://app/index.html");
                        })
                        .Get());
            })
            .Get());
    if (FAILED(result)) {
        fail(L"Could not initialize WebView2.");
    }
}

LRESULT CALLBACK window_procedure(HWND handle, UINT message, WPARAM wparam,
                                  LPARAM lparam) {
    (void)wparam;
    (void)lparam;
    switch (message) {
    case WM_SIZE:
        resize_webview();
        return 0;
    case WM_TIMER:
        if (smoke_test) {
            DestroyWindow(handle);
        }
        return 0;
    case WM_DESTROY:
        webview.Reset();
        webview_controller.Reset();
        PostQuitMessage(exit_code);
        return 0;
    default:
        return DefWindowProcW(handle, message, wparam, lparam);
    }
}
} // namespace

extern "C" int haniwa_run(int requested_smoke_test) {
    smoke_test = requested_smoke_test != 0;
    exit_code = smoke_test ? 1 : 0;
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW window_class = {};
    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = kWindowClass;
    window_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    if (RegisterClassW(&window_class) == 0) {
        return 1;
    }

    window_handle =
        CreateWindowExW(0, kWindowClass, L"Haniwa", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                        CW_USEDEFAULT, 880, 600, nullptr, nullptr, instance, nullptr);
    if (window_handle == nullptr) {
        return 1;
    }
    ShowWindow(window_handle, SW_SHOW);
    create_webview();

    MSG message;
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, wchar_t *command,
                    int show) {
    (void)instance;
    (void)previous;
    (void)command;
    (void)show;
    int argument_count = 0;
    wchar_t **arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    if (argument_count == 2 && wcscmp(arguments[1], L"--version") == 0) {
        MessageBoxW(nullptr,
                    L"haniwa "
                    L"0.1.0",
                    L"Haniwa", MB_OK);
        LocalFree(arguments);
        return 0;
    }
    bool requested_smoke_test =
        argument_count == 2 && wcscmp(arguments[1], L"--smoke-test") == 0;
    LocalFree(arguments);

    HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(result)) {
        return 1;
    }
    int result_code = haniwa_run(requested_smoke_test ? 1 : 0);
    CoUninitialize();
    return result_code;
}
