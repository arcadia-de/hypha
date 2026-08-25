#include <cstring>
#include <gtest/gtest.h>

#include "hypha/resource_bootstrap.h"
#include "hypha/resource_graph.h"

class ResourceBootstrapTest : public ::testing::Test {  // NOLINT(cppcoreguidelines-special-member-functions)
 public:
  ResourceGraph* graph = nullptr;

  void SetUp() override {
    graph = NewResourceGraph();
  }

  void TearDown() override {
    FreeResourceGraph(graph);
  }
};

TEST_F(ResourceBootstrapTest, InjectsOneResourcePerDef) {
  const CoreResourceDef defs[] = {
      {.kind = "Controller",
       .name = "symlink-controller",
       .ns = nullptr,
       .provides = "Symlink",
       .flags = kResourceFlagStatic},
      {.kind = "PackageBackend",
       .name = "brew-backend",
       .ns = nullptr,
       .provides = "Brew",
       .flags = kResourceFlagStatic},
  };

  ASSERT_TRUE(BootstrapCoreResources(graph, defs, 2));
  ASSERT_EQ(GetNumberOfResourcesInResourceGraph(graph), 2u);
}

TEST_F(ResourceBootstrapTest, DefaultsToCoreNamespaceAndIsStaticAndReady) {
  const CoreResourceDef defs[] = {
      {.kind = "Controller",
       .name = "symlink-controller",
       .ns = nullptr,
       .provides = "Symlink",
       .flags = kResourceFlagStatic},
  };

  ASSERT_TRUE(BootstrapCoreResources(graph, defs, 1));
  Resource* res = GetResourceInGraph(graph, 0);
  ASSERT_NE(res, nullptr);

  // TODO(@s0cks):
  //  EXPECT_STREQ(res->kind, "Controller");
  EXPECT_STREQ(res->info.name, "symlink-controller");

  ResourceNamespace expected_ns;
  SetResourceNamespace(expected_ns, kCoreResourceNamespace);
  EXPECT_TRUE(ResourceNamespaceEq(res->info.ns, expected_ns));

  EXPECT_TRUE(IsResourceStatic(res));
  EXPECT_FALSE(IsResourceDynamic(res));
  EXPECT_EQ(res->state, kResourceReady);
  // Never pending: the scheduler/reconcile dispatch loop only ever treats kResourcePending
  // as work to do (see QueueReconcileTaskForResource), so a static resource must never be
  // observed/normalized/planned/applied.
  EXPECT_FALSE(IsResourcePending(res));
}

TEST_F(ResourceBootstrapTest, DynamicUserResourceDefaultsToNonReservedNamespace) {
  // A resource that never goes through BootstrapCoreResources (i.e. anything a manifest
  // produces) has a zero-initialized ResourceInfo -- confirm that decodes as the default
  // (unreserved, non-static) namespace with no explicit setup required.
  Resource* res = AllocNewResouceInGraph(graph);
  ASSERT_NE(res, nullptr);
  EXPECT_TRUE(IsDefaultResourceNamespace(res->info.ns));
  EXPECT_FALSE(IsReservedResourceNamespace(res->info.ns));
  EXPECT_FALSE(IsResourceStatic(res));
  EXPECT_TRUE(IsResourceDynamic(res));
}

TEST_F(ResourceBootstrapTest, IdIsDeterministicAcrossGraphs) {
  const CoreResourceDef defs[] = {
      {.kind = "Controller",
       .name = "symlink-controller",
       .ns = nullptr,
       .provides = "Symlink",
       .flags = kResourceFlagStatic},
  };

  ASSERT_TRUE(BootstrapCoreResources(graph, defs, 1));
  Resource* first = GetResourceInGraph(graph, 0);

  ResourceGraph* other = NewResourceGraph();
  ASSERT_TRUE(BootstrapCoreResources(other, defs, 1));
  Resource* second = GetResourceInGraph(other, 0);

  EXPECT_EQ(uuid_compare(first->id, second->id), 0);
  FreeResourceGraph(other);
}

TEST_F(ResourceBootstrapTest, DifferentNamesYieldDifferentIds) {
  const CoreResourceDef defs[] = {
      {.kind = "Controller",
       .name = "symlink-controller",
       .ns = nullptr,
       .provides = "Symlink",
       .flags = kResourceFlagStatic},
      {.kind = "Controller",
       .name = "dir-controller",
       .ns = nullptr,
       .provides = "Directory",
       .flags = kResourceFlagStatic},
  };

  ASSERT_TRUE(BootstrapCoreResources(graph, defs, 2));
  Resource* a = GetResourceInGraph(graph, 0);
  Resource* b = GetResourceInGraph(graph, 1);
  EXPECT_NE(uuid_compare(a->id, b->id), 0);
}

TEST_F(ResourceBootstrapTest, FindResourceProvidingLocatesByAnnotation) {
  const CoreResourceDef defs[] = {
      {.kind = "Controller",
       .name = "symlink-controller",
       .ns = nullptr,
       .provides = "Symlink",
       .flags = kResourceFlagStatic},
      {.kind = "Controller",
       .name = "dir-controller",
       .ns = nullptr,
       .provides = "Directory",
       .flags = kResourceFlagStatic},
      {.kind = "PackageBackend",
       .name = "brew-backend",
       .ns = nullptr,
       .provides = "Brew",
       .flags = kResourceFlagStatic},
  };
  ASSERT_TRUE(BootstrapCoreResources(graph, defs, 3));

  Resource* found = nullptr;
  ASSERT_TRUE(FindResourceProviding(graph, "Controller", "Symlink", &found));
  ASSERT_NE(found, nullptr);
  EXPECT_STREQ(found->info.name, "symlink-controller");

  found = nullptr;
  ASSERT_TRUE(FindResourceProviding(graph, "PackageBackend", "Brew", &found));
  ASSERT_NE(found, nullptr);
  EXPECT_STREQ(found->info.name, "brew-backend");

  // Same `provides` value, different kind -- must not cross-match.
  found = nullptr;
  EXPECT_FALSE(FindResourceProviding(graph, "PackageBackend", "Symlink", &found));
  EXPECT_EQ(found, nullptr);

  // Nonexistent capability.
  EXPECT_FALSE(FindResourceProviding(graph, "Controller", "DoesNotExist", nullptr));
}

TEST_F(ResourceBootstrapTest, StaticResourceSatisfiesDependencyByName) {
  const CoreResourceDef defs[] = {
      {.kind = "PackageBackend",
       .name = "brew-backend",
       .ns = nullptr,
       .provides = "Brew",
       .flags = kResourceFlagStatic},
  };
  ASSERT_TRUE(BootstrapCoreResources(graph, defs, 1));

  Resource* dependent = AllocNewResouceInGraph(graph);
  dependent->kind = 1;
  dependent->info.name = strdup("ripgrep");
  dependent->state = kResourcePending;

  char** deps = (char**)malloc(sizeof(char*));
  deps[0] = strdup("brew-backend");
  dependent->depends_on = deps;
  dependent->num_depends_on = 1;

  EXPECT_TRUE(DependenciesAreSatisfied(graph, dependent));
}
