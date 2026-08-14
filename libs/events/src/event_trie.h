#ifndef HYPHA_EVENT_TRIE_H
#define HYPHA_EVENT_TRIE_H

#include "hypha/event.h"

typedef struct _Subscription {
  struct _Subscription* next;

  EventCallbackFn fn;
  void* data;
  void (*free_data)(void*);
} Subscription;

struct _EventRoute {
  EventRoute* children[HYPHA_EVENT_ALPHABET_SIZE];
  bool terminal;
  Subscription* subscriptions;
};

bool Search(EventRoute* root, const char* p, EventRoute** node);
bool Insert(EventRoute* root, const char* event, EventCallbackFn cb, void* data, void (*free_data)(void*));

#endif  // HYPHA_EVENT_TRIE_H
