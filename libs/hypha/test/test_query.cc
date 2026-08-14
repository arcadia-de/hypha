#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "query.h"

// ---------------------------------------------------------------------
// A synthetic domain, deliberately unrelated to hypha's Resource type --
// this test is proving the ENGINE is generic, not re-testing the
// resources() schema binding (see test_hypha_resources_schema.c for that).
// ---------------------------------------------------------------------

typedef struct {
  char* name;
  char* species;
} Pet;

typedef struct {
  char* name;
  char** tags;
  int num_tags;
  Pet* pets;
  int num_pets;
} Person;

static Pet alice_pets[] = {{"Rex", "Dog"}, {"Milo", "Cat"}};
static char* alice_tags[] = {"engineer", "hiker"};
static Person people[] = {
    {"Alice", alice_tags, 2, alice_pets, 2},
    {"Bob", NULL, 0, NULL, 0},
};
static const int num_people = 2;

static FieldResolverResult PersonName(void* obj) {
  Person* p = (Person*)obj;
  return (FieldResolverResult){.kind = kFieldResultScalar, .scalar = strdup(p->name)};
}

static FieldResolverResult PersonTags(void* obj) {
  Person* p = (Person*)obj;
  char** list = (char**)malloc(sizeof(char*) * (p->num_tags > 0 ? p->num_tags : 1));
  for (int i = 0; i < p->num_tags; i++)
    list[i] = strdup(p->tags[i]);
  return (FieldResolverResult){
      .kind = kFieldResultScalarList, .scalar_list = list, .scalar_list_count = (uint32_t)p->num_tags};
}

static FieldResolverResult PersonPets(void* obj) {
  Person* p = (Person*)obj;
  QueryObject* list = (QueryObject*)malloc(sizeof(QueryObject) * (p->num_pets > 0 ? p->num_pets : 1));
  for (int i = 0; i < p->num_pets; i++)
    list[i] = (QueryObject){.object = &p->pets[i], .type_name = "Pet"};
  return (FieldResolverResult){
      .kind = kFieldResultObjectList, .object_list = list, .object_list_count = (uint32_t)p->num_pets};
}

static FieldResolverResult PetName(void* obj) {
  return (FieldResolverResult){.kind = kFieldResultScalar, .scalar = strdup(((Pet*)obj)->name)};
}

static FieldResolverResult PetSpecies(void* obj) {
  return (FieldResolverResult){.kind = kFieldResultScalar, .scalar = strdup(((Pet*)obj)->species)};
}

static const FieldDef person_fields[] = {
    {"name", PersonName},
    {"tags", PersonTags},
    {"pets", PersonPets},
};
static const FieldDef pet_fields[] = {
    {"name", PetName},
    {"species", PetSpecies},
};
static const TypeDef types[] = {
    {"Person", person_fields, 3},
    {"Pet", pet_fields, 2},
};

static RootResult ResolvePeople(const QueryArg* args, void* context) {
  (void)context;
  const char* name_filter = QueryArgGet(args, "name");

  QueryObject* matched = (QueryObject*)malloc(sizeof(QueryObject) * num_people);
  uint32_t count = 0;
  for (int i = 0; i < num_people; i++) {
    if (name_filter && strcmp(people[i].name, name_filter) != 0)
      continue;
    matched[count++] = (QueryObject){.object = &people[i], .type_name = "Person"};
  }
  return (RootResult){.objects = matched, .count = count};
}

static const RootFieldDef roots[] = {
    {"people", ResolvePeople, "Person"},
};

static const QuerySchema schema = {
    .roots = roots,
    .num_roots = 1,
    .types = types,
    .num_types = 2,
    .context = NULL,  // this test's data is static globals, no context needed
};

// ---------------------------------------------------------------------

static void TestBasicScalarField(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people { name }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: people { name }\n  -> %s\n", json);
  assert(strstr(json, "\"Alice\""));
  assert(strstr(json, "\"Bob\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: basic scalar field query\n");
}

static void TestFilterArgument(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people(name: \"Alice\") { name }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: people(name: \"Alice\") { name }\n  -> %s\n", json);
  assert(strstr(json, "\"Alice\""));
  assert(!strstr(json, "\"Bob\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: root argument filters correctly\n");
}

static void TestScalarListField(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people(name: \"Alice\") { name tags }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: people(name: \"Alice\") { name tags }\n  -> %s\n", json);
  assert(strstr(json, "\"engineer\""));
  assert(strstr(json, "\"hiker\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: scalar list field\n");
}

static void TestNestedObjectListField(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people(name: \"Alice\") { name pets { name species } }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: people(name: \"Alice\") { name pets { name species } }\n  -> %s\n", json);
  assert(strstr(json, "\"Rex\""));
  assert(strstr(json, "\"Dog\""));
  assert(strstr(json, "\"Milo\""));
  assert(strstr(json, "\"Cat\""));

  free(json);
  ResultNodeFree(result);
  printf("PASS: nested object-list field, arbitrary depth via recursion\n");
}

static void TestBobHasEmptyListsNotErrors(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people(name: \"Bob\") { name tags pets { name } }", &err);
  assert(result);
  assert(!err);

  char* json = ResultNodeToJSON(result);
  printf("query: people(name: \"Bob\") { name tags pets { name } }\n  -> %s\n", json);
  assert(strstr(json, "\"tags\":[]"));
  assert(strstr(json, "\"pets\":[]"));

  free(json);
  ResultNodeFree(result);
  printf("PASS: zero-length lists serialize as [] rather than erroring\n");
}

static void TestUnknownRootFieldErrors(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "bogus { name }", &err);
  assert(!result);
  assert(err);
  printf("query: bogus { name }\n  -> error: %s\n", err);
  assert(strstr(err, "bogus"));
  free(err);
  printf("PASS: unknown root field reported as an error, not a crash\n");
}

static void TestUnknownFieldErrors(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people { nonexistent_field }", &err);
  assert(!result);
  assert(err);
  printf("query: people { nonexistent_field }\n  -> error: %s\n", err);
  assert(strstr(err, "nonexistent_field"));
  free(err);
  printf("PASS: unknown field on a known type reported as an error\n");
}

static void TestObjectListWithoutSubSelectionErrors(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people { pets }", &err);
  assert(!result);
  assert(err);
  printf("query: people { pets }  (no sub-selection)\n  -> error: %s\n", err);
  free(err);
  printf("PASS: object-list field without a sub-selection is rejected\n");
}

static void TestMalformedQuerySyntaxErrors(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people { name", &err);  // missing closing brace
  assert(!result);
  assert(err);
  printf("query: people { name   (unterminated)\n  -> error: %s\n", err);
  free(err);
  printf("PASS: malformed syntax reported as a parse error, not a crash\n");
}

static void TestMultipleTopLevelSelections(void) {
  char* err = NULL;
  ResultNode* result = QueryExecute(&schema, "people(name: \"Alice\") { name } people(name: \"Bob\") { name }", &err);
  assert(result);
  assert(!err);
  assert(result->num_object_fields == 2);

  char* json = ResultNodeToJSON(result);
  printf("query: two top-level 'people' selections\n  -> %s\n", json);

  free(json);
  ResultNodeFree(result);
  printf("PASS: multiple top-level selections both resolve\n");
}

int main(void) {
  TestBasicScalarField();
  TestFilterArgument();
  TestScalarListField();
  TestNestedObjectListField();
  TestBobHasEmptyListsNotErrors();
  TestUnknownRootFieldErrors();
  TestUnknownFieldErrors();
  TestObjectListWithoutSubSelectionErrors();
  TestMalformedQuerySyntaxErrors();
  TestMultipleTopLevelSelections();
  printf("\nALL QUERY ENGINE TESTS PASSED\n");
  return 0;
}
