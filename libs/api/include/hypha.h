#ifndef HYPHA_H
#define HYPHA_H

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

#define CONTEXT_REGISTRY_KEY_ORCHESTRATOR "hypha_orchestrator"

#ifndef container_of
#define container_of(ptr, type, member)               \
  ({                                                  \
    const typeof(((type*)0)->member)* __mptr = (ptr); \
    (type*)((char*)__mptr - offsetof(type, member));  \
  })
#endif  // container_of

typedef void* OrchestratorHandle;

typedef struct _HistoryLog HistoryLog;

void InitHypha();

extern char* RenderJsonnet(char* name, char* code);
extern char* RenderTemplate(char* tpl, char* data, bool is_yaml);

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_H
