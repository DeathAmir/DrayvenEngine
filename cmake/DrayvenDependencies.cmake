include(FetchContent)

function(drayven_setup_dependencies)
  if(DRAYVEN_ENABLE_EIGEN)
    if(DRAYVEN_FETCH_DEPS)
      set(EIGEN_BUILD_DOC OFF CACHE BOOL "" FORCE)
      set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
      FetchContent_Declare(eigen
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen.git
        GIT_TAG 3.4.0
        GIT_SHALLOW TRUE)
      FetchContent_MakeAvailable(eigen)
    else()
      find_package(Eigen3 3.4 CONFIG REQUIRED)
    endif()
  endif()

  if(DRAYVEN_ENABLE_NETWORKING)
    if(DRAYVEN_FETCH_DEPS)
      set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
      set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
      set(BUILD_LIBCURL_DOCS OFF CACHE BOOL "" FORCE)
      set(BUILD_MISC_DOCS OFF CACHE BOOL "" FORCE)
      set(HTTP_ONLY ON CACHE BOOL "" FORCE)
      set(CURL_USE_LIBPSL OFF CACHE BOOL "" FORCE)
      set(CURL_ZLIB OFF CACHE BOOL "" FORCE)
      set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
      FetchContent_Declare(curl
        GIT_REPOSITORY https://github.com/curl/curl.git
        GIT_TAG curl-8_20_0
        GIT_SHALLOW TRUE)
      FetchContent_MakeAvailable(curl)
    else()
      find_package(CURL REQUIRED)
    endif()
  endif()

  if(DRAYVEN_ENABLE_MBEDTLS)
    if(DRAYVEN_FETCH_DEPS)
      set(ENABLE_PROGRAMS OFF CACHE BOOL "" FORCE)
      set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
      set(USE_SHARED_MBEDTLS_LIBRARY OFF CACHE BOOL "" FORCE)
      set(USE_STATIC_MBEDTLS_LIBRARY ON CACHE BOOL "" FORCE)
      FetchContent_Declare(mbedtls
        GIT_REPOSITORY https://github.com/Mbed-TLS/mbedtls.git
        GIT_TAG mbedtls-3.6.7
        GIT_SHALLOW TRUE
        GIT_SUBMODULES_RECURSE TRUE)
      FetchContent_MakeAvailable(mbedtls)
    else()
      find_package(MbedTLS REQUIRED)
    endif()
  endif()
endfunction()
