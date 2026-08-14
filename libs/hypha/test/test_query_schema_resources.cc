#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hypha.h"
#include "query.h"
#include "query_schema_resources.h"

static ResourceAnnotation neovim_annotations[] = {
    {"managed-by", "hypha"},
};
static char* neovim_labels[] = {"editor", "dotfile"};

static ResourceAnnotation zshrc_annotations[] = {
    {"managed-by", "hypha"},
    {"source", "files/zshrc"},
};
static char* zshrc_labels[] = {"dotfile"};
static char* zshrc_deps[] = {"Package/neovim"};

static Resource MakeResource(char* id, char* kind, char** labels, uint32_t num_labels, ResourceAnnotation* annotations,
                             uint32_t num_annotations, char** deps, uint32_t num_deps, ResourceState state) {
  Resource res = {0};
  res.id = id;
  res.kind = kind;
  res.info.labels = labels;
  res.info.num_labels = num_labels;
  res.info.annotations = annotations;
  res.info.num_annotations = num_annotations;
  res.depends_on = deps;
  res.num_depends_on = num_deps;
  res.state = state;
  return res;
}

static Resource test_resources[3];

static void SetUp(void) {
  test_resources[0] =
      MakeResource("Package/neovim", "Package", neovim_labels, 2, neovim_annotations, 1, NULL, 0, kResourceReady);
  test_resources[1] = MakeResource("Package/ripgrep", "Package", NULL, 0, NULL, 0, NULL, 0, kResourceReady);
  test_resources[2] =
      MakeResource("Symlink/zshrc", "Symlink", zshrc_labels, 1, zshrc_annotations, 2, zshrc_deps, 1, kResourceFailed);
}

static void TestTheirExactExampleQuery(void) {
  ResourcesQueryContext ctx = {.resources = test_resources, .count = 3};
  QuerySchema schema = HyphaResourcesQuerySchema(&ctx);

  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "resources(kind: Package) { id }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: resources(kind: Package) { id }\n  -> %s\n", json);
  assert(strstr(json, "\"Package/neovim\""));
  assert(strstr(json, "\"Package/ripgrep\""));
  assert(!strstr(json, "\"Symlink/zshrc\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: exact example query from the request works end to end\n");
}

static void TestMultipleFieldsAndState(void) {
  ResourcesQueryContext ctx = {.resources = test_resources, .count = 3};
  QuerySchema schema = HyphaResourcesQuerySchema(&ctx);

  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "resources(kind: Symlink) { id state labels }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: resources(kind: Symlink) { id state labels }\n  -> %s\n", json);
  assert(strstr(json, "\"Failed\""));
  assert(strstr(json, "\"dotfile\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: state and labels fields\n");
}

static void TestNestedAnnotations(void) {
  ResourcesQueryContext ctx = {.resources = test_resources, .count = 3};
  QuerySchema schema = HyphaResourcesQuerySchema(&ctx);

  char* err = NULL;
  ResultNode* result =
      QueryExecute(&schema, "resources(id: \"Symlink/zshrc\") { id annotations { name value } }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: resources(id: \"Symlink/zshrc\") { id annotations { name value } }\n  -> %s\n", json);
  assert(strstr(json, "\"managed-by\""));
  assert(strstr(json, "\"source\""));
  assert(strstr(json, "\"files/zshrc\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: nested annotations { name value } resolves against real Resource/ResourceAnnotation structs\n");
}

static void TestLabelFilterAndDependsOn(void) {
  ResourcesQueryContext ctx = {.resources = test_resources, .count = 3};
  QuerySchema schema = HyphaResourcesQuerySchema(&ctx);

  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "resources(label: \"dotfile\") { id dependsOn }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: resources(label: \"dotfile\") { id dependsOn }\n  -> %s\n", json);
  assert(strstr(json, "\"Package/neovim\""));    // has the dotfile label
  assert(strstr(json, "\"Symlink/zshrc\""));     // has the dotfile label
  assert(!strstr(json, "\"Package/ripgrep\""));  // no labels at all
  assert(strstr(json, "\"Package/neovim\""));    // in zshrc's dependsOn list

  free(json);
  ResultNodeFree(result);
  printf("PASS: label filter (reusing ResourceHasLabel) and dependsOn scalar list\n");
}

static void TestNoFilterReturnsEverything(void) {
  ResourcesQueryContext ctx = {.resources = test_resources, .count = 3};
  QuerySchema schema = HyphaResourcesQuerySchema(&ctx);

  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "resources { id }", &err);
  assert(result);
  assert(!err);
  assert(result->object_fields[0].value->num_array_items == 3);

  ResultNodeFree(result);
  printf("PASS: no filter arguments returns every resource\n");
}

int main(void) {
  SetUp();
  TestTheirExactExampleQuery();
  TestMultipleFieldsAndState();
  TestNestedAnnotations();
  TestLabelFilterAndDependsOn();
  TestNoFilterReturnsEverything();
  printf("\nALL RESOURCES SCHEMA TESTS PASSED\n");
  return 0;
}
