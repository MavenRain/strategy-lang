/* The JSON target. The tcc-json kit only. See json.h. */
#include <stdio.h>
#include <string.h>

#include "json.h"

#define JSON_TYPE_MAX 4096u

/* new-lang.sh replaces the placeholder with the language name. */
static const char LANG_NAME[] = "strategy-lang";

typedef struct {
  Machine *m;
  const char *def; /* the current instance, for diagnostics */
  char *data;
  size_t len;
  size_t cap;
  unsigned depth;
} Out;

static int put(Out *o, const char *text, size_t n) {
  if (o->len + n + 1 > o->cap) {
    size_t cap = o->cap == 0 ? 4096 : o->cap;
    char *data;
    while (cap < o->len + n + 1)
      cap *= 2;
    data = arena_alloc(o->m->arena, cap);
    if (data == NULL)
      return diag_fail(o->m->diag, "OOM", o->def, "the arena limit is reached");
    if (o->len > 0)
      memcpy(data, o->data, o->len);
    o->data = data;
    o->cap = cap;
  }
  memcpy(o->data + o->len, text, n);
  o->len += n;
  o->data[o->len] = '\0';
  return 1;
}

static int put_text(Out *o, const char *text) {
  return put(o, text, strlen(text));
}

/* A JSON string. Names and printed types are ASCII; escape all the same. */
static int put_string(Out *o, const char *text) {
  char esc[8];
  const char *p;
  int ok = put(o, "\"", 1);
  for (p = text; ok && *p != '\0'; p++) {
    unsigned char ch = (unsigned char)*p;
    int plain = ch >= 0x20 && ch != '"' && ch != '\\';
    if (plain)
      ok = put(o, p, 1);
    else {
      snprintf(esc, sizeof esc, "\\u%04x", ch);
      ok = put(o, esc, 6);
    }
  }
  return ok && put(o, "\"", 1);
}

static int refuse(Out *o, const char *what) {
  return diag_fail(o->m->diag, "JSON_VALUE", o->def, "the instance holds %s, which has no JSON form", what);
}

static int internal(Out *o, const char *what) {
  return diag_fail(o->m->diag, "INTERNAL", o->def, "the JSON printer found %s", what);
}

/* 1 when V is a number or a canonical value. A trap is a compile error. */
static int ready(Out *o, const Value *v) {
  switch (v->kind) {
  case VAL_NAT:
  case VAL_OP:
    return 1;
  case VAL_TRAP:
    return diag_fail(o->m->diag, "EVAL_OVERFLOW", o->def, "the instance value traps (a Nat overflow)");
  case VAL_UNIV:
  case VAL_PI:
  case VAL_SIGMA:
    return refuse(o, "a type");
  case VAL_LAM:
    return refuse(o, "a function");
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    return refuse(o, "a stuck term");
  }
  return internal(o, "an unknown value kind");
}

static int value_json(Out *o, const Value *type, const Value *v);

static int sum_json(Out *o, const Value *type, const Value *v) {
  if (val_is(v, OP_INL))
    return put_text(o, "{\"inl\":") && value_json(o, type->args[0], v->args[0]) && put_text(o, "}");
  if (val_is(v, OP_INR))
    return put_text(o, "{\"inr\":") && value_json(o, type->args[1], v->args[0]) && put_text(o, "}");
  return internal(o, "a Sum value that is not inl or inr");
}

/* none is null, so `some` wraps a value that can be null (ledger rule). */
static int option_json(Out *o, const Value *type, const Value *v) {
  const Value *elem = type->args[0];
  int wrap = val_is(elem, OP_OPTION);
  if (val_is(v, OP_NONE))
    return put_text(o, "null");
  if (!val_is(v, OP_SOME))
    return internal(o, "an Option value that is not none or some");
  return (!wrap || put_text(o, "{\"some\":")) && value_json(o, elem, v->args[0]) && (!wrap || put_text(o, "}"));
}

/* Walks the spine, so a long list does not nest the C stack. */
static int list_json(Out *o, const Value *type, const Value *v) {
  const Value *cell = v;
  int first = 1;
  int ok = put_text(o, "[");
  while (ok && val_is(cell, OP_CONS)) {
    ok = (first || put_text(o, ",")) && value_json(o, type->args[0], cell->args[0]) && ready(o, cell->args[1]);
    cell = cell->args[1];
    first = 0;
  }
  if (!ok)
    return 0;
  return val_is(cell, OP_NIL) ? put_text(o, "]") : internal(o, "a List value that is not nil or cons");
}

static int is_enum(const Machine *m, const FamilyInfo *f) {
  uint32_t i;
  for (i = 0; i < f->ctor_count; i++)
    if (!m->ctors[f->first_ctor + i].is_path && m->ctors[f->first_ctor + i].field_count != 0)
      return 0;
  return 1;
}

/* Only constants: the name. One constructor: an object of the fields.
   Otherwise: an object with "tag" and the fields. A field type is in the
   context of the family parameters, as in check.c. */
static int family_json(Out *o, const Value *type, const Value *v) {
  const FamilyInfo *f;
  const CtorInfo *ci;
  const Env *env = NULL;
  uint32_t i;
  int ok;
  if (type->inst >= o->m->family_count || !val_is(v, OP_CTOR) || v->inst >= o->m->ctor_count)
    return internal(o, "a family value that is not a constructor");
  f = &o->m->families[type->inst];
  ci = &o->m->ctors[v->inst];
  if (ci->family != type->inst || v->argc != f->param_count + ci->field_count || type->argc != f->param_count)
    return internal(o, "a constructor of another family");
  if (is_enum(o->m, f))
    return put_string(o, ci->name);
  for (i = 0; i < f->param_count; i++)
    env = env_push(o->m, env, type->args[i]);
  ok = put_text(o, "{");
  if (f->ctor_count != 1)
    ok = ok && put_text(o, "\"tag\":") && put_string(o, ci->name);
  for (i = 0; ok && i < ci->field_count; i++) {
    const FieldInfo *fi = &ci->fields[i];
    const Value *ft = fi->recursive ? type : eval_core(o->m, env, fi->type);
    int lead = i == 0 && f->ctor_count == 1;
    ok = ft != NULL && (lead || put_text(o, ",")) && put_string(o, fi->name) && put_text(o, ":")
      && value_json(o, ft, v->args[f->param_count + i]);
  }
  return ok && put_text(o, "}");
}

/* The ends of an Eq proof: the carrier type and the two sides. The carrier
   is NULL under cong, where the codomain is not known. */
typedef struct {
  const Value *carrier;
  const Value *left;
  const Value *right;
} Ends;

/* V in the surface syntax, as a JSON string. */
static int printed_json(Out *o, const Value *v) {
  char *text = arena_alloc(o->m->arena, JSON_TYPE_MAX);
  if (text == NULL)
    return diag_fail(o->m->diag, "OOM", o->def, "the arena limit is reached");
  if (!value_print(o->m, NULL, 0, v, text, JSON_TYPE_MAX))
    return diag_fail(o->m->diag, "JSON_TYPE", o->def, "the point of a proof exceeds the printer limits");
  return put_string(o, text);
}

/* refl with its point. A point that is a function or a type has no JSON
   form, so its printed normal form is the point (a string). */
static int refl_json(Out *o, const Value *carrier, const Value *point) {
  if (!put_text(o, "{\"refl\":"))
    return 0;
  if (carrier->kind != VAL_PI && carrier->kind != VAL_UNIV)
    return value_json(o, carrier, point) && put_text(o, "}");
  return printed_json(o, point) && put_text(o, "}");
}

/* A stuck symm, trans, cong or transport of an Eq type is a proof tree, not
   a stuck term (rulings R5, R7). */
static int stuck_proof(const Value *type, const Value *v) {
  return val_is(type, OP_EQ) && v->kind == VAL_STUCK
    && (v->op == OP_SYMM || v->op == OP_TRANS || v->op == OP_CONG || v->op == OP_TRANSPORT);
}

/* The path constructor of V, or NULL. */
static const CtorInfo *path_ctor(const Machine *m, const Value *v) {
  if (!val_is(v, OP_CTOR) || v->inst >= m->ctor_count || !m->ctors[v->inst].is_path)
    return NULL;
  return &m->ctors[v->inst];
}

static int proof_ends(Out *o, const Value *v, Ends *e);

/* The ends of a path constructor: its ends at its parameters and fields. */
static int path_ends(Out *o, const CtorInfo *ci, const Value *v, Ends *e) {
  const FamilyInfo *f = &o->m->families[ci->family];
  const Env *env = NULL;
  uint32_t i;
  if (v->argc != f->param_count + ci->field_count)
    return internal(o, "a path constructor with the wrong number of arguments");
  for (i = 0; i < v->argc; i++)
    env = env_push(o->m, env, v->args[i]);
  e->carrier = val_op(o->m, VAL_OP, OP_FAMILY, ci->family, 0, v->args, f->param_count);
  e->left = eval_core(o->m, env, ci->lhs);
  e->right = eval_core(o->m, env, ci->rhs);
  return e->carrier != NULL && e->left != NULL && e->right != NULL;
}

/* trans p q: the left end from p unless p is refl, the right end from q
   unless q is refl. */
static int trans_ends(Out *o, const Value *v, Ends *e) {
  Ends p;
  Ends q;
  int prefl = val_is(v->args[0], OP_REFL);
  int qrefl = val_is(v->args[1], OP_REFL);
  if (prefl && qrefl)
    return internal(o, "a stuck trans of two refl proofs");
  if ((!prefl && !proof_ends(o, v->args[0], &p)) || (!qrefl && !proof_ends(o, v->args[1], &q)))
    return 0;
  if (prefl)
    p = q;
  if (qrefl)
    q = p;
  e->carrier = p.carrier != NULL ? p.carrier : q.carrier;
  e->left = p.left;
  e->right = q.right;
  return 1;
}

/* cong f p: f at the ends of p. transport P e u: the sides of P at the
   right end of e, which must be an Eq type. */
static int image_ends(Out *o, const Value *v, Ends *e) {
  Ends p;
  const Value *at;
  if (!proof_ends(o, v->args[1], &p))
    return 0;
  if (v->op == OP_CONG) {
    e->carrier = v->cong_carrier;
    e->left = apply_value(o->m, v->args[0], p.left);
    e->right = apply_value(o->m, v->args[0], p.right);
    return e->left != NULL && e->right != NULL;
  }
  at = apply_value(o->m, v->args[0], p.right);
  if (!val_is(at, OP_EQ))
    return internal(o, "a transport proof whose type is not Eq");
  e->carrier = at->args[0];
  e->left = at->args[1];
  e->right = at->args[2];
  return 1;
}

/* The ends of the proof V (not refl), from its structure. */
static int proof_ends(Out *o, const Value *v, Ends *e) {
  const CtorInfo *ci = path_ctor(o->m, v);
  if (ci != NULL)
    return path_ends(o, ci, v, e);
  if (v == NULL || v->kind != VAL_STUCK)
    return refuse(o, "a stuck term");
  if (v->op == OP_SYMM) {
    if (!proof_ends(o, v->args[0], e))
      return 0;
    v = e->left;
    e->left = e->right;
    e->right = v;
    return 1;
  }
  if (v->op == OP_TRANS)
    return trans_ends(o, v, e);
  if (v->op == OP_CONG || v->op == OP_TRANSPORT)
    return image_ends(o, v, e);
  return refuse(o, "a stuck term");
}

static int proof_json(Out *o, const Ends *t, const Value *v);

/* A path constructor: its name, then its fields. The field types are a
   telescope over the parameters and the earlier fields. */
static int path_json(Out *o, const CtorInfo *ci, const Value *v) {
  const FamilyInfo *f = &o->m->families[ci->family];
  const Env *env = NULL;
  uint32_t i;
  int ok;
  if (v->argc != f->param_count + ci->field_count)
    return internal(o, "a path constructor with the wrong number of arguments");
  for (i = 0; i < f->param_count; i++)
    env = env_push(o->m, env, v->args[i]);
  ok = put_text(o, "{\"path\":") && put_string(o, ci->name);
  for (i = 0; ok && i < ci->field_count; i++) {
    const Value *ft = eval_core(o->m, env, ci->fields[i].type);
    const Value *x = v->args[f->param_count + i];
    ok = ft != NULL && put_text(o, ",") && put_string(o, ci->fields[i].name) && put_text(o, ":") && value_json(o, ft, x);
    env = env_push(o->m, env, x);
  }
  return ok && put_text(o, "}");
}

/* trans p q: the middle point is the left end of q when p is refl, else
   the right end of p. */
static int trans_json(Out *o, const Ends *t, const Value *v) {
  Ends p;
  Ends q;
  int prefl = val_is(v->args[0], OP_REFL);
  if (!proof_ends(o, v->args[prefl ? 1 : 0], prefl ? &q : &p))
    return 0;
  p.carrier = t->carrier;
  p.left = t->left;
  p.right = prefl ? q.left : p.right;
  q.carrier = t->carrier;
  q.left = p.right;
  q.right = t->right;
  return put_text(o, "{\"trans\":[") && proof_json(o, &p, v->args[0]) && put_text(o, ",")
    && proof_json(o, &q, v->args[1]) && put_text(o, "]}");
}

/* The proof tree of V at the ends T. No proof is erased (ruling R5). */
static int proof_tree(Out *o, const Ends *t, const Value *v) {
  const CtorInfo *ci = path_ctor(o->m, v);
  Ends sub;
  if (val_is(v, OP_REFL))
    return t->carrier != NULL ? refl_json(o, t->carrier, t->left) : refuse(o, "a refl proof under cong, whose carrier is not known");
  if (ci != NULL)
    return path_json(o, ci, v);
  if (v == NULL || v->kind != VAL_STUCK)
    return refuse(o, "a stuck term");
  if (v->op == OP_SYMM) {
    sub.carrier = t->carrier;
    sub.left = t->right;
    sub.right = t->left;
    return put_text(o, "{\"symm\":") && proof_json(o, &sub, v->args[0]) && put_text(o, "}");
  }
  if (v->op == OP_TRANS)
    return trans_json(o, t, v);
  if (v->op == OP_CONG)
    return proof_ends(o, v->args[1], &sub) && put_text(o, "{\"cong\":{\"fn\":") && printed_json(o, v->args[0])
      && put_text(o, ",\"path\":") && proof_json(o, &sub, v->args[1]) && put_text(o, "}}");
  if (v->op == OP_TRANSPORT)
    return proof_ends(o, v->args[1], &sub) && put_text(o, "{\"transport\":{\"family\":") && printed_json(o, v->args[0])
      && put_text(o, ",\"path\":") && proof_json(o, &sub, v->args[1]) && put_text(o, ",\"value\":")
      && value_json(o, apply_value(o->m, v->args[0], sub.left), v->args[2]) && put_text(o, "}}");
  return refuse(o, "a stuck term");
}

static int proof_json(Out *o, const Ends *t, const Value *v) {
  int ok;
  if (o->depth >= JSON_DEPTH_LIMIT)
    return diag_fail(o->m->diag, "JSON_DEPTH", o->def, "the instance nests deeper than %u levels", JSON_DEPTH_LIMIT);
  o->depth++;
  ok = proof_tree(o, t, v);
  o->depth--;
  return ok;
}

static int op_json(Out *o, const Value *type, const Value *v) {
  char num[24];
  Ends ends;
  switch (type->op) {
  case OP_NAT:
    if (v->kind != VAL_NAT)
      return internal(o, "a Nat value that is not a number");
    snprintf(num, sizeof num, "%llu", (unsigned long long)v->nat);
    return put_text(o, num);
  case OP_FLAG:
    if (val_is(v, OP_FLAG_YES))
      return put_text(o, "true");
    return val_is(v, OP_FLAG_NO) ? put_text(o, "false") : internal(o, "a Flag value that is not yes or no");
  case OP_UNIT:
    return put_text(o, "{}");
  case OP_EQ:
    ends.carrier = type->args[0];
    ends.left = type->args[1];
    ends.right = type->args[2];
    return proof_json(o, &ends, v);
  case OP_PROD:
    if (!val_is(v, OP_PAIR))
      return internal(o, "a Prod value that is not a pair");
    return put_text(o, "{\"first\":") && value_json(o, type->args[0], v->args[0]) && put_text(o, ",\"second\":")
      && value_json(o, type->args[1], v->args[1]) && put_text(o, "}");
  case OP_SUM:
    return sum_json(o, type, v);
  case OP_OPTION:
    return option_json(o, type, v);
  case OP_LIST:
    return list_json(o, type, v);
  case OP_FAMILY:
    return family_json(o, type, v);
  case OP_UNIT_VAL:
  case OP_FLAG_YES:
  case OP_FLAG_NO:
  case OP_PAIR:
  case OP_INL:
  case OP_INR:
  case OP_NONE:
  case OP_SOME:
  case OP_NIL:
  case OP_CONS:
  case OP_PACK:
  case OP_REFL:
  case OP_CTOR:
  case OP_FIRST:
  case OP_SECOND:
  case OP_EITHER:
  case OP_OPTION_ELIM:
  case OP_PURE:
  case OP_MAP:
  case OP_BIND:
  case OP_FOLD_NAT:
  case OP_FOLD_LIST:
  case OP_FOLD_FAMILY:
  case OP_UNFOLD:
  case OP_FILTER:
  case OP_WITNESS:
  case OP_PAYLOAD:
  case OP_SYMM:
  case OP_TRANS:
  case OP_TRANSPORT:
  case OP_CONG:
  case OP_NAT_ADD:
  case OP_NAT_SUB:
  case OP_NAT_MUL:
  case OP_NAT_EQ:
  case OP_NAT_LE:
  case OP_FLAG_IF:
  case OP_PROJ:
  case OP_ABSURD:
    return internal(o, "a type that is not a type former");
  case OP_EMPTY:
    return refuse(o, "a value of the empty type");
  }
  return internal(o, "an unknown operation");
}

/* The payload type is the Sigma body at the witness. */
static int pack_json(Out *o, const Value *type, const Value *v) {
  const Value *payload;
  if (!val_is(v, OP_PACK))
    return internal(o, "a Sigma value that is not pack");
  if (!put_text(o, "{\"witness\":") || !value_json(o, type->dom, v->args[0]))
    return 0;
  payload = closure_apply(o->m, type, v->args[0]);
  return payload != NULL && put_text(o, ",\"payload\":") && value_json(o, payload, v->args[1]) && put_text(o, "}");
}

/* A function on an enumeration (ruling R6). A function on a family with a
   path constructor is not a table: a table drops the path images (R7). */
static int table_type(const Machine *m, const Value *type) {
  return type != NULL && type->kind == VAL_PI && val_is(type->dom, OP_FAMILY) && is_enumeration(m, type->dom->inst);
}

/* A refutation A -> Empty (ruling R5). Empty has no arguments, so the
   codomain cannot depend on the argument. */
static int refute_type(Machine *m, const Value *type, uint32_t level) {
  const Value *cod = type != NULL && type->kind == VAL_PI ? closure_apply(m, type, val_var(m, level)) : NULL;
  return val_is(cod, OP_EMPTY);
}

/* One key for each constructor in declaration order. The value of a key is
   the function at that constructor, at the codomain at that constructor. */
static int table_json(Out *o, const Value *type, const Value *v) {
  const FamilyInfo *f = &o->m->families[type->dom->inst];
  uint32_t j;
  int ok = put_text(o, "{");
  for (j = 0; ok && j < f->ctor_count; j++) {
    const Value *cj = val_op(o->m, VAL_OP, OP_CTOR, f->first_ctor + j, 0, NULL, 0);
    const Value *cod = cj == NULL ? NULL : closure_apply(o->m, type, cj);
    const Value *at = cod == NULL ? NULL : apply_value(o->m, v, cj);
    ok = (j == 0u || put_text(o, ",")) && put_string(o, o->m->ctors[f->first_ctor + j].name) && put_text(o, ":") && at != NULL && value_json(o, cod, at);
  }
  return ok && put_text(o, "}");
}

/* A refutation keeps its evidence: its printed normal form (ruling R5,
   the O8 precedent). Never false, null or an empty object. */
static int refute_json(Out *o, const Value *v) {
  return put_text(o, "{\"refute\":") && printed_json(o, v) && put_text(o, "}");
}

static int pi_json(Out *o, const Value *type, const Value *v) {
  if (table_type(o->m, type))
    return table_json(o, type, v);
  if (refute_type(o->m, type, 0))
    return refute_json(o, v);
  return refuse(o, "a function");
}

static int typed_json(Out *o, const Value *type, const Value *v) {
  switch (type->kind) {
  case VAL_OP:
    return op_json(o, type, v);
  case VAL_SIGMA:
    return pack_json(o, type, v);
  case VAL_PI:
    return pi_json(o, type, v);
  case VAL_UNIV:
    return refuse(o, "a type");
  case VAL_NAT:
  case VAL_TRAP:
  case VAL_LAM:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    return refuse(o, "a value of a stuck type");
  }
  return internal(o, "an unknown type kind");
}

static int value_json(Out *o, const Value *type, const Value *v) {
  int ok;
  /* Tables and refutations may be functions, but must still reject traps. */
  int fn = table_type(o->m, type) || refute_type(o->m, type, 0);
  if ((v->kind == VAL_TRAP || (!fn && !stuck_proof(type, v))) && !ready(o, v))
    return 0;
  if (o->depth >= JSON_DEPTH_LIMIT)
    return diag_fail(o->m->diag, "JSON_DEPTH", o->def, "the instance nests deeper than %u levels", JSON_DEPTH_LIMIT);
  o->depth++;
  ok = typed_json(o, type, v);
  o->depth--;
  return ok;
}

/* Every branch of a table must end in data (R6) or a refutation (R5).
   Use concrete constructors: a symbolic argument can hide a dependent
   table domain, function result or universe behind a stuck eliminator. */
static int pi_instance(Machine *m, const Value *type, uint32_t depth) {
  const FamilyInfo *f;
  uint32_t j;
  if (type == NULL)
    return 0;
  if (type->kind != VAL_PI && type->kind != VAL_UNIV)
    return 1;
  if (!table_type(m, type))
    return refute_type(m, type, 0);
  if (depth >= JSON_DEPTH_LIMIT)
    return diag_fail(m->diag, "JSON_DEPTH", m->def, "the instance nests deeper than %u levels", JSON_DEPTH_LIMIT);
  f = &m->families[type->dom->inst];
  for (j = 0; j < f->ctor_count; j++) {
    const Value *cj = val_op(m, VAL_OP, OP_CTOR, f->first_ctor + j, 0, NULL, 0);
    if (!pi_instance(m, closure_apply(m, type, cj), depth + 1u))
      return 0;
  }
  return 1;
}

/* Types, families and other functions are not instances. A proof is an
   instance (the no-collapse rule). A table and a refutation are instances
   (rulings R5 and R6). */
static int is_instance(Machine *m, const DefInfo *d) {
  if (d->origin != ORIGIN_PROGRAM)
    return 0;
  m->def = d->name;
  m->fuel = EVAL_FUEL_STEPS;
  m->depth = 0;
  switch (d->type->kind) {
  case VAL_PI:
    return pi_instance(m, d->type, 0);
  case VAL_UNIV:
    return 0;
  case VAL_OP:
  case VAL_SIGMA:
  case VAL_NAT:
  case VAL_TRAP:
  case VAL_LAM:
  case VAL_VAR:
  case VAL_APP:
  case VAL_STUCK:
    return 1;
  }
  return 1;
}

static int instance_json(Out *o, uint32_t index, char *type_text, int first) {
  Machine *m = o->m;
  const DefInfo *d = &m->defs[index];
  const Value *v;
  o->def = d->name;
  m->def = d->name;
  m->fuel = EVAL_FUEL_STEPS;
  m->depth = 0;
  v = def_value(m, index);
  if (v == NULL)
    return 0;
  if (!value_print(m, NULL, 0, d->type, type_text, JSON_TYPE_MAX))
    return diag_fail(m->diag, "JSON_TYPE", d->name, "the instance type exceeds the printer limits");
  return (first || put_text(o, ",")) && put_text(o, "{\"name\":") && put_string(o, d->name)
    && put_text(o, ",\"type\":") && put_string(o, type_text) && put_text(o, ",\"value\":")
    && value_json(o, d->type, v) && put_text(o, "}");
}

int json_document(Machine *m, const char **text, size_t *len) {
  Out o;
  char *type_text = arena_alloc(m->arena, JSON_TYPE_MAX);
  uint32_t i;
  int first = 1;
  int ok;
  memset(&o, 0, sizeof o);
  o.m = m;
  if (type_text == NULL)
    return diag_fail(m->diag, "OOM", NULL, "the arena limit is reached");
  ok = put_text(&o, "{") && put_string(&o, LANG_NAME) && put_text(&o, ":1,\"instances\":[");
  for (i = 0; ok && i < m->def_count; i++) {
    if (is_instance(m, &m->defs[i])) {
      ok = instance_json(&o, i, type_text, first);
      first = 0;
    }
  }
  if (!ok || m->diag->set || !put_text(&o, "]}\n"))
    return 0;
  *text = o.data;
  *len = o.len;
  return 1;
}
