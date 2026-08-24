#ifndef HYPHA_DISCOVERED_MANIFEST_H
#define HYPHA_DISCOVERED_MANIFEST_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_DISCOVERED_MANIFEST_KIND(V) \
  V(Raw)                                     \
  V(Path)

// clang-format off
typedef enum {
#define DEFINE_KIND(Name) kDiscovered##Name,
  FOR_EACH_DISCOVERED_MANIFEST_KIND(DEFINE_KIND)
#undef DEFINE_KIND
  kTotalNumberOfDiscoveredManifestKinds,
} DiscoveredManifestKind;
// clang-format on

typedef struct {
  DiscoveredManifestKind kind;
  char* value;
} DiscoveredManifest;

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_DISCOVERED_MANIFEST_H
