/* Nested proof JSON must retain types, including after normalization. */
#include <stdio.h>
#include <string.h>

#include "front/check.h"
#include "front/front.h"
#include "json.h"

typedef struct {
  const char *name;
  const char *carrier;
  const char *point;
  const char *image;
  const char *printed_image;
  const char *point_json;
  const char *inner;
} ProofCase;

static const ProofCase CASES[] = {
  {"family", "Circle", "base", "fun (c : Circle) => c", "fun c => c", "\"base\"", "cong image loop"},
  {"nat", "Nat", "7", "fun (c : Circle) => 7", "fun c => 7", "7", "cong image loop"},
  {"proof", "Eq Circle base base", "loop", "fun (c : Circle) => loop", "fun c => loop", "{\"path\":\"loop\"}", "cong image loop"},
  {"option", "Option Nat", "some 7", "fun (c : Circle) => some 7", "fun c => some 7", "7", "cong image loop"},
  {"list", "List Nat", "nil", "fun (c : Circle) => nil", "fun c => nil", "[]", "cong image loop"},
  {"function", "Nat -> Nat", "fun (n : Nat) => n", "fun (c : Circle) (n : Nat) => n", "fun c => fun n => n", "\"fun n => n\"", "cong image loop"},
  {"type", "Type 0", "Nat", "fun (c : Circle) => Nat", "fun c => Nat", "\"Nat\"", "cong image loop"},
  {"bound carrier", "List Nat", "nil", "fun (c : Circle) => nil", "fun c => nil", "[]", "makePath (List Nat) nil"},
  {"fold path case", "Nat", "7", "fun (c : Circle) => 7", "fun c => 7", "7", "cong (fun (c : Circle) => fold 7 (cong image loop) c) loop"},
};

static int check_json(Machine *m, const ProofCase *test) {
  const char *doc = NULL;
  const char *outer;
  const char *value;
  size_t len = 0;
  char expected[1024];
  int n = snprintf(expected, sizeof expected,
    "\"value\":{\"cong\":{\"fn\":\"fun x => x\",\"path\":{\"trans\":["
    "{\"refl\":%s},{\"cong\":{\"fn\":\"%s\",\"path\":{\"path\":\"loop\"}}}]}}}",
    test->point_json, test->printed_image);
  if (n < 0 || (size_t)n >= sizeof expected || !json_document(m, &doc, &len))
    return 0;
  outer = strstr(doc, "\"name\":\"outer\",");
  value = outer == NULL ? NULL : strstr(outer, "\"value\":");
  if (value == NULL || strncmp(value, expected, (size_t)n) != 0) {
    fprintf(stderr, "FAIL %s: expected %s, got %s\n", test->name, expected, value == NULL ? doc : value);
    return 0;
  }
  return 1;
}

static int check_case(const ProofCase *test) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine m;
  char source[8192];
  int ok = 0;
  int n = snprintf(source, sizeof source,
    "def makePath : (A : Type 0) -> (x : A) -> Eq A x x := "
    "fun (A : Type 0) (x : A) => cong (fun (c : Circle) => x) loop\n"
    "def image : Circle -> (%s) := %s\n"
    "def saved : Eq (%s) (%s) (%s) := refl\n"
    "def inner : Eq (%s) (%s) (%s) := %s\n"
    "def mixed : Eq (%s) (%s) (%s) := trans saved inner\n"
    "def outer : Eq (%s) (%s) (%s) := cong (fun (x : (%s)) => x) mixed\n",
    test->carrier, test->image,
    test->carrier, test->point, test->point,
    test->carrier, test->point, test->point, test->inner,
    test->carrier, test->point, test->point,
    test->carrier, test->point, test->point, test->carrier);
  uint32_t i;
  arena_init(&arena, 64u * 1024u * 1024u);
  diag_init(&diag);
  if (n < 0 || (size_t)n >= sizeof source
      || !front_load(&arena, test->name, source, (size_t)n, &decls, &diag)
      || !check_program(&arena, &decls, &m, &diag)
      || !check_json(&m, test))
    goto done;
  for (i = 0; i < m.def_count; i++) {
    const Core *normal;
    const Value *value;
    if (m.defs[i].origin != ORIGIN_PROGRAM)
      continue;
    value = def_value(&m, i);
    normal = quote_value(&m, 0, value);
    m.defs[i].value = eval_core(&m, NULL, normal);
    if (m.defs[i].value == NULL || !conv_values(&m, 0, value, m.defs[i].value))
      goto done;
  }
  ok = check_json(&m, test);
done:
  if (!ok) {
    fprintf(stderr, "FAIL proof carrier %s\n", test->name);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

int main(void) {
  size_t i;
  unsigned failed = 0;
  for (i = 0; i < sizeof CASES / sizeof CASES[0]; i++)
    failed += !check_case(&CASES[i]);
  printf("proof regressions: %zu checked, %u failed\n", sizeof CASES / sizeof CASES[0], failed);
  return failed != 0;
}
