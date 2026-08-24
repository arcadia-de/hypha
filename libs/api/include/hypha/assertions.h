#ifndef HYPHA_ASSERTIONS_H
#define HYPHA_ASSERTIONS_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#ifdef HYPHA_DEBUG

#include <assert.h>

#ifndef ASSERT
#define ASSERT(x) assert((x))
#endif  // ASSERT

#ifndef ASSERT_EQ
#define ASSERT_EQ(a, b) ASSERT(a == b)
#endif  // ASSERT_EQ

#ifndef ASSERT_NE
#define ASSERT_NE(a, b) ASSERT(a != b)
#endif  // ASSERT_NE

#else

#ifndef ASSERT
#define ASSERT(x)
#endif  // ASSERT

#ifndef ASSERT_EQ
#define ASSERT_EQ(a, b)
#endif  // ASSERT_EQ

#ifndef ASSERT_NE
#define ASSERT_NE(a, b)
#endif  // ASSERT_NE

#endif  // HYPHA_DEBUG

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_ASSERTIONS_H
