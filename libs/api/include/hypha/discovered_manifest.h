#ifndef HYPHA_DISCOVERED_MANIFEST_H
#define HYPHA_DISCOVERED_MANIFEST_H

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

#define FOR_EACH_DISCOVERED_MANIFEST_KIND(V) \
  V(Path)                                    \
  V(RawJson)                                 \
  V(RawYaml)                                 \
  V(RawJsonnet)

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

#define DEFINE_KIND_CHECK(Name)                                         \
  static inline bool Is##Name##Manifest(const DiscoveredManifest* dm) { \
    return dm && dm->kind == kDiscovered##Name;                         \
  }

FOR_EACH_DISCOVERED_MANIFEST_KIND(DEFINE_KIND_CHECK)
#undef DEFINE_KIND_CHECK

static inline bool IsRawManifest(const DiscoveredManifest* dm) {
  if (!dm)
    return false;

  switch (dm->kind) {
    case kDiscoveredRawJsonnet:
    case kDiscoveredRawJson:
    case kDiscoveredRawYaml:
      return true;
    default:
      return false;
  }
}

#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // HYPHA_DISCOVERED_MANIFEST_H
