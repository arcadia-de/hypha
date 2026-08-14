#include "event_trie.h"

#include <stdlib.h>
#include <string.h>

#include "hypha/event.h"
#include "hypha/log.h"

static const int kDotIndex = 26;
static const int kSlashIndex = 27;
static const int kDashIndex = 28;
static const int kWildcardIndex = 29;

static inline int GetIndex(const char c) {
  switch (c) {
    case '*':
      return kWildcardIndex;
    case '.':
      return kDotIndex;
    case '/':
      return kSlashIndex;
    case '-':
      return kDashIndex;
    default:
      return ((int)c) - ((int)'a');
  }
}

EventRoute* NewEventRoute(void) {
  EventRoute* node = (EventRoute*)malloc(sizeof(EventRoute));
  if (node) {
    memset(node, 0, sizeof(EventRoute));
    node->subscriptions = NULL;
    for (int i = 0; i < HYPHA_EVENT_ALPHABET_SIZE; i++)
      node->children[i] = NULL;
  }

  return node;
}

static inline Subscription* NewSub(EventCallbackFn fn, void* data, void (*free_data)(void*)) {
  if (!fn)
    return NULL;

  Subscription* sub = (Subscription*)malloc(sizeof(Subscription));
  if (sub) {
    memset(sub, 0, sizeof(Subscription));
    sub->next = NULL;
    sub->fn = fn;
    sub->data = data;
    sub->free_data = free_data;
  }

  return sub;
}

static inline void AppendSub(Subscription** head, EventCallbackFn fn, void* data, void (*free_data)(void*)) {
  Subscription* new_sub = NewSub(fn, data, free_data);
  if ((*head) == NULL) {
    (*head) = new_sub;
    return;
  }

  Subscription* last = (*head);
  while (last->next != NULL)
    last = last->next;

  last->next = new_sub;
}

static inline void FreeSub(Subscription* sub) {
  if (!sub)
    return;

  if (sub->data && sub->free_data)
    sub->free_data(sub->data);

  free(sub);
}

static inline void FreeSubList(Subscription* subs) {
  Subscription* next = NULL;
  while (subs != NULL) {
    next = subs;
    subs = subs->next;

    FreeSub(next);
  }
}

static inline bool InsertWithWildcard(EventRoute* current, const char* ptr, EventCallbackFn cb, void* data,
                                      void (*free_data)(void*)) {
  if ((*ptr) == '\0') {
    current->terminal = true;
    AppendSub(&current->subscriptions, cb, data, free_data);
    return true;
  }

  const int index = GetIndex(*ptr);
  if (index < 0 || index >= HYPHA_EVENT_ALPHABET_SIZE)
    return false;

  if (!current->children[index])
    current->children[index] = NewEventRoute();

  return InsertWithWildcard(current->children[index], ptr + 1, cb, data, free_data);
}

bool Insert(EventRoute* root, const char* event, EventCallbackFn cb, void* data, void (*free_data)(void*)) {
  if (!root || !event)
    return false;
  return InsertWithWildcard(root, event, cb, data, free_data);
}

static inline bool SearchWithWildcard(EventRoute* current, const char* ptr, EventRoute** node) {
  if (*ptr == '\0') {
    if (current != NULL && current->terminal) {
      *node = current;
      return true;
    }
    return false;
  }

  const int index = GetIndex(*ptr);
  if (index >= 0 && index < HYPHA_EVENT_ALPHABET_SIZE && current->children[index]) {
    if (SearchWithWildcard(current->children[index], ptr + 1, node))
      return true;
  }

  if (current->children[kWildcardIndex]) {
    EventRoute* wildcard_node = current->children[kWildcardIndex];
    const char* lookahead = ptr;

    while (*lookahead != '\0') {
      if (*lookahead == '.') {
        if (SearchWithWildcard(wildcard_node, lookahead, node))
          return true;

        break;
      }

      lookahead++;
    }

    if (*lookahead == '\0' && wildcard_node->terminal) {
      *node = wildcard_node;
      return true;
    }
  }

  return false;
}

bool Search(EventRoute* root, const char* event, EventRoute** node) {
  if (!root || !event || !node) {
    if (node)
      (*node) = NULL;
    return false;
  }

  (*node) = NULL;
  return SearchWithWildcard(root, event, node);
}

void FreeEventRoute(EventRoute* root) {
  if (!root)
    return;

  if (root->subscriptions != NULL)
    FreeSubList(root->subscriptions);

  for (int i = 0; i < HYPHA_EVENT_ALPHABET_SIZE; i++) {
    if (root->children[i])
      FreeEventRoute(root->children[i]);
  }
}

bool Subscribe(EventRoute* root, const char* p, EventCallbackFn cb, void* data, void (*free_data)(void*)) {
  return Insert(root, p, cb, data, free_data);
}

bool Publish(EventRoute* root, const char* p, void* event) {
  if (!root || !p)
    return false;

  EventRoute* node = NULL;
  if (!Search(root, p, &node))
    return false;

  if (!node)
    return false;

  Subscription* sub = node->subscriptions;
  while (sub != NULL) {
    sub->fn(p, event, sub->data);
    sub = sub->next;
  }

  return true;
}
