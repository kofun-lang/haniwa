cmake_minimum_required(VERSION 3.24)

project(haniwa VERSION 0.1.0 LANGUAGES C)

include(CTest)

add_library(haniwa_core STATIC
  src/resource.c
)
target_include_directories(haniwa_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(haniwa_core PUBLIC c_std_11)

if(APPLE)
  enable_language(OBJC)
  add_executable(haniwa MACOSX_BUNDLE
    src/main.c
    src/platform/macos.m
  )
  target_compile_options(haniwa PRIVATE
    $<$<COMPILE_LANGUAGE:OBJC>:-fobjc-arc>
  )
  target_link_libraries(haniwa PRIVATE
    haniwa_core
    "-framework AppKit"
    "-framework WebKit"
  )
elseif(WIN32)
  enable_language(CXX)
  include(FetchContent)

  set(HANIWA_WEBVIEW2_VERSION "1.0.4129.50" CACHE STRING
    "Pinned Microsoft.Web.WebView2 NuGet package version")
  FetchContent_Declare(webview2
    URL "https://api.nuget.org/v3-flatcontainer/microsoft.web.webview2/${HANIWA_WEBVIEW2_VERSION}/microsoft.web.webview2.${HANIWA_WEBVIEW2_VERSION}.nupkg"
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(webview2)

  if(CMAKE_GENERATOR_PLATFORM STREQUAL "ARM64")
    set(HANIWA_WEBVIEW2_ARCH arm64)
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(HANIWA_WEBVIEW2_ARCH x64)
  else()
    set(HANIWA_WEBVIEW2_ARCH x86)
  endif()

  add_executable(haniwa WIN32
    src/platform/windows.cpp
  )
  target_compile_features(haniwa PRIVATE cxx_std_17)
  target_compile_definitions(haniwa PRIVATE
    UNICODE
    _UNICODE
    WEBVIEW2_STATIC
  )
  target_include_directories(haniwa PRIVATE
    "${webview2_SOURCE_DIR}/build/native/include"
  )
  target_link_libraries(haniwa PRIVATE
    haniwa_core
    "${webview2_SOURCE_DIR}/build/native/${HANIWA_WEBVIEW2_ARCH}/WebView2LoaderStatic.lib"
    ole32
    shlwapi
    user32
  )
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  pkg_check_modules(WEBKITGTK IMPORTED_TARGET webkit2gtk-4.1)
  if(NOT WEBKITGTK_FOUND)
    pkg_check_modules(WEBKITGTK REQUIRED IMPORTED_TARGET webkit2gtk-4.0)
  endif()

  add_executable(haniwa
    src/main.c
    src/platform/linux.c
  )
  target_link_libraries(haniwa PRIVATE
    haniwa_core
    PkgConfig::GTK3
    PkgConfig::WEBKITGTK
  )
endif()

target_include_directories(haniwa PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(haniwa_core PRIVATE -Wall -Wextra -Wpedantic)
  target_compile_options(haniwa PRIVATE
    $<$<COMPILE_LANGUAGE:C>:-Wall;-Wextra;-Wpedantic>
    $<$<COMPILE_LANGUAGE:CXX>:-Wall;-Wextra;-Wpedantic>
    $<$<COMPILE_LANGUAGE:OBJC>:-Wall;-Wextra;-Wpedantic>
  )
endif()

if(BUILD_TESTING)
  add_executable(haniwa_resource_test tests/resource_test.c)
  target_link_libraries(haniwa_resource_test PRIVATE haniwa_core)
  add_test(NAME resource_contract COMMAND haniwa_resource_test)
endif()
