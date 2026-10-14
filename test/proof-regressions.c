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

/* S3: an off-diagonal decision keeps its refutation as a normal form string
   (rule R5), and elim on a variable stays stuck (it is not case 0). */
static const char ENUM_SOURCE[] =
  "family Phase := dawn | noon | dusk\n"
  "def code : (y : Phase) -> Type 0 := fun (y : Phase) => fold Unit Empty Empty y\n"
  "def no : Sum (Eq Phase dawn dusk) ((e : Eq Phase dawn dusk) -> Empty) := "
  "inr (fun (e : Eq Phase dawn dusk) => transport code e unit)\n"
  "def pick : (p : Phase) -> Nat := fun (p : Phase) => elim (fun (q : Phase) => Nat) 1 2 3 p\n"
  "def caseZero : (p : Phase) -> Nat := fun (p : Phase) => 1\n";
static const char REFUTE_PREFIX[] = "\"value\":{\"inr\":{\"refute\":\"";
#define ENUM_CHECKS 2u

/* The number of failed checks. */
static unsigned check_enums(void) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine m;
  const Value *program[4];
  const char *doc = NULL;
  const char *no = NULL;
  size_t len = 0;
  size_t prefix = sizeof REFUTE_PREFIX - 1u;
  uint32_t i;
  uint32_t k = 0;
  int refute = 0;
  int stuck = 0;
  arena_init(&arena, 64u * 1024u * 1024u);
  diag_init(&diag);
  if (front_load(&arena, "enums", ENUM_SOURCE, sizeof ENUM_SOURCE - 1u, &decls, &diag)
      && check_program(&arena, &decls, &m, &diag)) {
    for (i = 0; i < m.def_count; i++)
      if (m.defs[i].origin == ORIGIN_PROGRAM && k < 4u)
        program[k++] = def_value(&m, i);
    no = json_document(&m, &doc, &len) ? strstr(doc, "\"name\":\"no\",") : NULL;
    no = no == NULL ? NULL : strstr(no, "\"value\":");
    refute = no != NULL && strncmp(no, REFUTE_PREFIX, prefix) == 0 && no[prefix] != '"';
    stuck = k == 4u && !conv_values(&m, 0, program[2], program[3]);
  }
  if (!refute)
    fprintf(stderr, "FAIL refutation: expected %s and a normal form, got %s\n", REFUTE_PREFIX, no == NULL ? (doc == NULL ? "no document" : doc) : no);
  if (!stuck)
    fprintf(stderr, "FAIL stuck elim: elim on a variable is equal to case 0\n");
  if (diag.set)
    diag_print(&diag, stderr);
  arena_release(&arena);
  return (unsigned)!refute + (unsigned)!stuck;
}

typedef struct {
  const char *name;
  const char *source;
  const char *json;
  const char *error;
} JsonCase;

static const JsonCase JSON_CASES[] = {
  {"dependent table keys",
   "family Side := leftSide | rightSide\n"
   "def Choices : Color -> Type 0 := fun (c : Color) => fold Side Color Side c\n"
   "def table : (c : Color) -> Choices c -> Nat := fun (c : Color) (d : Choices c) => 7\n",
   "\"value\":{\"red\":{\"leftSide\":7,\"rightSide\":7},\"green\":{\"red\":7,\"green\":7,\"blue\":7},\"blue\":{\"leftSide\":7,\"rightSide\":7}}", NULL},
  {"dependent function helpers are not instances",
   "def Result : Color -> Type 0 := fun (c : Color) => fold (Nat -> Nat) (Nat -> Nat) (Nat -> Nat) c\n"
   "def helper : (c : Color) -> Result c := fun (c : Color) => elim Result (fun (n : Nat) => n) (fun (n : Nat) => n) (fun (n : Nat) => n) c\n"
   "def Mixed : Color -> Type 0 := fun (c : Color) => fold Nat (Nat -> Nat) Nat c\n"
   "def mixed : (c : Color) -> Mixed c := fun (c : Color) => elim Mixed 1 (fun (n : Nat) => n) 3 c\n",
   "\"instances\":[]", NULL},
  {"dependent type helpers are not instances",
   "def Kind : Color -> Type 1 := fun (c : Color) => fold (Type 0) (Type 0) (Type 0) c\n"
   "def types : (c : Color) -> Kind c := fun (c : Color) => elim Kind Nat Unit Empty c\n",
   "\"instances\":[]", NULL},
  {"tables ending in refutations",
   "def refs : Color -> Empty -> Empty := fun (c : Color) (e : Empty) => e\n",
   "\"value\":{\"red\":{\"refute\":\"fun e => e\"},\"green\":{\"refute\":\"fun e => e\"},\"blue\":{\"refute\":\"fun e => e\"}}", NULL},
  {"overflowing refutation",
   "def broken : Empty -> Empty := fold (fun (f : Empty -> Empty) => f) (fun (e : Empty) => e) (natAdd 18446744073709551615 1)\n",
   NULL, "EVAL_OVERFLOW"},
  {"nested overflowing refutation",
   "def boxed : Option (Empty -> Empty) := some (fold (fun (f : Empty -> Empty) => f) (fun (e : Empty) => e) (natAdd 18446744073709551615 1))\n",
   NULL, "EVAL_OVERFLOW"},
  {"unroll path fields",
   "family St := s0 | s1\n"
   "def u : Eq (Pth St) (lasso St (cons s1 nil) s0 (cons s1 (cons s0 nil))) (lasso St (cons s1 (cons s0 nil)) s1 (cons s0 (cons s0 nil))) := unroll St (cons s1 nil) s0 s1 (cons s0 nil)\n",
   "\"value\":{\"path\":\"unroll\",\"upre\":[\"s1\"],\"uc\":\"s0\",\"ud\":\"s1\",\"uds\":[\"s0\"]}", NULL},
  {"lasso evidence leaves",
   "family St := s0 | s1\n"
   "family Ap := inS0 | inS1\n"
   "def label : St -> Ap -> Flag := fun (s : St) (a : Ap) => fold (fold flagYes flagNo a) (fold flagNo flagYes a) s\n"
   "def labN : St -> Nat -> Flag := fun (s : St) (n : Nat) => flagIf (natEq n 0) (label s inS0) (flagIf (natEq n 1) (label s inS1) flagNo)\n"
   "def f1 : Ltl := eventually (atom 1)\n"
   "def wd : List St := wordOf St (cons s0 (cons s0 (cons s0 nil))) s1 nil\n"
   "def S : Ltl -> Nat -> Type 0 := Sat Nat (fun (i : Nat) (a : Nat) => labN (lassoAt St wd s0 i) a) (lassoNext 3 4) 4\n"
   "def fI0 : S f1 0 := inr (pair unit (inr (pair unit (inr (pair unit (inl refl))))))\n",
   "\"value\":{\"inr\":{\"first\":{},\"second\":{\"inr\":{\"first\":{},\"second\":{\"inr\":{\"first\":{},\"second\":{\"inl\":{\"refl\":true}}}}}}}}", NULL},
};

static int check_json_case(const JsonCase *test) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine m;
  const char *doc = NULL;
  size_t len = 0;
  int ok = 0;
  arena_init(&arena, 64u * 1024u * 1024u);
  diag_init(&diag);
  if (front_load(&arena, test->name, test->source, strlen(test->source), &decls, &diag)
      && check_program(&arena, &decls, &m, &diag)) {
    int built = json_document(&m, &doc, &len);
    ok = test->error != NULL
      ? !built && diag.set && strcmp(diag.code, test->error) == 0 && doc == NULL && len == 0
      : built && !diag.set && doc != NULL && strstr(doc, test->json) != NULL;
  }
  if (!ok) {
    fprintf(stderr, "FAIL %s: expected %s, got %s\n", test->name,
      test->error != NULL ? test->error : test->json, doc == NULL ? "no document" : doc);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* S4b: a stuck elimList is not a value and not a proof. A closed def cannot
   hold one, so the test puts `lift` (a stuck elimList under trans) on a
   variable in place of the value of `pr`. The JSON build must refuse the proof with JSON_VALUE. */
static const char STUCK_SOURCE[] =
  "def r : Eq Nat 1 1 := refl\n"
  "def lift : List Nat -> Eq Nat 1 1 := fun (xs : List Nat) => trans r "
  "(elimList (fun (ys : List Nat) => Eq Nat 1 1) refl (fun (x : Nat) (ys : List Nat) (h : Eq Nat 1 1) => h) xs)\n"
  "def pr : Eq Nat 1 1 := refl\n";
#define STUCK_CHECKS 1u

static int check_stuck_elim_list(void) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine m;
  uint32_t program[3];
  const char *doc = NULL;
  size_t len = 0;
  uint32_t i;
  uint32_t k = 0;
  int ok = 0;
  arena_init(&arena, 64u * 1024u * 1024u);
  diag_init(&diag);
  if (front_load(&arena, "stuck elimList", STUCK_SOURCE, sizeof STUCK_SOURCE - 1u, &decls, &diag)
      && check_program(&arena, &decls, &m, &diag)) {
    for (i = 0; i < m.def_count; i++)
      if (m.defs[i].origin == ORIGIN_PROGRAM && k < 3u)
        program[k++] = i;
    if (k == 3u) {
      m.defs[program[2]].value = apply_value(&m, def_value(&m, program[1]), val_var(&m, 0));
      ok = m.defs[program[2]].value != NULL && !json_document(&m, &doc, &len)
        && diag.set && strcmp(diag.code, "JSON_VALUE") == 0 && doc == NULL;
    }
  }
  if (!ok) {
    fprintf(stderr, "FAIL stuck elimList: expected JSON_VALUE, got %s\n", doc == NULL ? "no document" : doc);
    if (diag.set)
      diag_print(&diag, stderr);
  }
  arena_release(&arena);
  return ok;
}

/* S6 (ruling R1 (a)): a two-quantifier sentence checks, but the normal form
   of its type is longer than JSON_TYPE_MAX, so the JSON build refuses it with
   JSON_TYPE. Each case appends SOURCE to the file EXAMPLE (the Makefile runs
   this test from the repository root). The program must check, the def NAME
   must have a value, and json_document must refuse with JSON_TYPE. The vA2
   and vC programs also hold exFyRef2, so their JSON_TYPE can come from
   exFyRef2 first. */
typedef struct {
  const char *name;
  const char *example;
  const char *source;
} SentenceCase;

static const SentenceCase SENTENCE_CASES[] = {
  {"fyEx2", "examples/pennies.lang",
   "def SA2 : Ix -> Type 0 := fun (j : Ix) => Sigma (i : Ix) (TS j i)\n"
   "def fyEx2 : (j : Ix) -> SA2 j := fun (j : Ix) => elim SA2 (pack ihh (pair (inl refl) (inr (inl refl)))) (pack iht (pair (inl refl) (inr (inl refl)))) (pack ith (pair (inl refl) (inr (inl refl)))) (pack itt (pair (inl refl) (inr (inl refl)))) j\n"},
  {"exFyRef2", "examples/pennies.lang",
   "def RA2 : Ix -> Type 0 := fun (i : Ix) => Sigma (j : Ix) (TR i j)\n"
   "def exFyRef2 : (i : Ix) -> RA2 i := fun (i : Ix) => elim RA2 (pack itt (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack ith (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack iht (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack ihh (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) i\n"},
  {"vA2", "examples/pennies.lang",
   "def RA2 : Ix -> Type 0 := fun (i : Ix) => Sigma (j : Ix) (TR i j)\n"
   "def exFyRef2 : (i : Ix) -> RA2 i := fun (i : Ix) => elim RA2 (pack itt (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack ith (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack iht (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack ihh (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) i\n"
   "def vA2 : Sum (Sigma (i : Ix) ((j : Ix) -> TS j i)) ((i : Ix) -> RA2 i) := inr exFyRef2\n"},
  {"vC", "examples/pennies.lang",
   "def RA2 : Ix -> Type 0 := fun (i : Ix) => Sigma (j : Ix) (TR i j)\n"
   "def exFyRef2 : (i : Ix) -> RA2 i := fun (i : Ix) => elim RA2 (pack itt (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack ith (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack iht (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) (pack ihh (inr (pair unit (inl (pair refl (inr (pair refl (inr refl)))))))) i\n"
   "def vC : Prod Cls (Sum (Sigma (i : Ix) ((j : Ix) -> TS j i)) ((i : Ix) -> RA2 i)) := pair positional (inr exFyRef2)\n"},
  {"tgSafe2", "examples/traingate.lang",
   "def TT : CIx -> TIx -> Type 0 := fun (c : CIx) (t : TIx) => fold (fold (ET c0 t0) (ET c1 t0) (ET c2 t0) (ET c3 t0) (ET c4 t0) (ET c5 t0) (ET c6 t0) (ET c7 t0) c) (fold (ET c0 t1) (ET c1 t1) (ET c2 t1) (ET c3 t1) (ET c4 t1) (ET c5 t1) (ET c6 t1) (ET c7 t1) c) (fold (ET c0 t2) (ET c1 t2) (ET c2 t2) (ET c3 t2) (ET c4 t2) (ET c5 t2) (ET c6 t2) (ET c7 t2) c) (fold (ET c0 t3) (ET c1 t3) (ET c2 t3) (ET c3 t3) (ET c4 t3) (ET c5 t3) (ET c6 t3) (ET c7 t3) c) (fold (ET c0 t4) (ET c1 t4) (ET c2 t4) (ET c3 t4) (ET c4 t4) (ET c5 t4) (ET c6 t4) (ET c7 t4) c) (fold (ET c0 t5) (ET c1 t5) (ET c2 t5) (ET c3 t5) (ET c4 t5) (ET c5 t5) (ET c6 t5) (ET c7 t5) c) (fold (ET c0 t6) (ET c1 t6) (ET c2 t6) (ET c3 t6) (ET c4 t6) (ET c5 t6) (ET c6 t6) (ET c7 t6) c) (fold (ET c0 t7) (ET c1 t7) (ET c2 t7) (ET c3 t7) (ET c4 t7) (ET c5 t7) (ET c6 t7) (ET c7 t7) c) t\n"
   "def FC : CIx -> Type 0 := fun (c : CIx) => (t : TIx) -> TT c t\n"
   "def TC7 : TIx -> Type 0 := fun (t : TIx) => TT c7 t\n"
   "def tgSafe2 : Sigma (c : CIx) (FC c) := pack c7 (fun (t : TIx) => elim TC7 (pair refl (inr (pair refl (inr refl)))) (pair refl (inr (pair refl (inr refl)))) (pair refl (inr (pair refl (inr refl)))) (pair refl (inr (pair refl (inr refl)))) (pair refl (inr refl)) (pair refl (inr refl)) (pair refl (inr refl)) (pair refl (inr refl)) t)\n"},
};

/* Reads EXAMPLE and appends a newline and SOURCE, in the arena. */
static const char *sentence_text(Arena *arena, const SentenceCase *test, size_t *len) {
  FILE *f = fopen(test->example, "rb");
  long size = -1;
  size_t src_len = strlen(test->source);
  char *text = NULL;
  if (f == NULL)
    return NULL;
  if (fseek(f, 0, SEEK_END) == 0)
    size = ftell(f);
  if (size >= 0 && fseek(f, 0, SEEK_SET) == 0)
    text = arena_alloc(arena, (size_t)size + 1u + src_len + 1u);
  if (text != NULL && fread(text, 1, (size_t)size, f) == (size_t)size) {
    text[size] = '\n';
    memcpy(text + size + 1, test->source, src_len + 1u);
    *len = (size_t)size + 1u + src_len;
  } else {
    text = NULL;
  }
  fclose(f);
  return text;
}

static int check_sentence_case(const SentenceCase *test) {
  Arena arena;
  Diag diag;
  DeclList decls;
  Machine m;
  const char *text;
  const char *doc = NULL;
  size_t text_len = 0;
  size_t len = 0;
  uint32_t i;
  int valued = 0;
  int ok = 0;
  arena_init(&arena, (size_t)1 << 30);
  diag_init(&diag);
  text = sentence_text(&arena, test, &text_len);
  if (text != NULL && front_load(&arena, test->name, text, text_len, &decls, &diag)
      && check_program(&arena, &decls, &m, &diag)) {
    for (i = 0; i < m.def_count; i++)
      if (m.defs[i].origin == ORIGIN_PROGRAM && strcmp(m.defs[i].name, test->name) == 0)
        valued = def_value(&m, i) != NULL;
    ok = valued && !json_document(&m, &doc, &len)
      && diag.set && strcmp(diag.code, "JSON_TYPE") == 0 && doc == NULL;
  }
  if (!ok) {
    fprintf(stderr, "FAIL sentence %s: expected a checked def and JSON_TYPE, got %s\n",
      test->name, text == NULL ? "no source" : (doc == NULL ? "no document" : doc));
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
  failed += check_enums();
  for (i = 0; i < sizeof JSON_CASES / sizeof JSON_CASES[0]; i++)
    failed += !check_json_case(&JSON_CASES[i]);
  failed += !check_stuck_elim_list();
  for (i = 0; i < sizeof SENTENCE_CASES / sizeof SENTENCE_CASES[0]; i++)
    failed += !check_sentence_case(&SENTENCE_CASES[i]);
  printf("proof regressions: %zu checked, %u failed\n", sizeof CASES / sizeof CASES[0] + ENUM_CHECKS + sizeof JSON_CASES / sizeof JSON_CASES[0] + STUCK_CHECKS + sizeof SENTENCE_CASES / sizeof SENTENCE_CASES[0], failed);
  return failed != 0;
}
