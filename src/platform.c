#if defined(_WIN32)
   #include "platform_windows.c"
#elif defined(LINUX)
   #include "platform_linux.c"
#else
    static_assert(false, "no known platform found");
#endif
