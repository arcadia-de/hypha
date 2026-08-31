#ifndef HYPHA_PLATFORM_H
#define HYPHA_PLATFORM_H

#if defined(__linux__) || defined(__FreeBSD__)
#define HYPHA_LINUX 1
#elif defined(__APPPLE__)
#define HYPHA_OSX 1
#elif defined(_WIN32) || defined(_WIN64)
#define HYPHA_WINDOWS 1
#endif

#endif  // HYPHA_PLATFORM_H
