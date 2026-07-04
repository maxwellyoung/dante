// dialog.c — rule database core: symbols, facts, criteria, matching, parsing
// Pure C, no Raylib — unit-testable headless (tests/test_dialog.c).
//
// Matching model (Ruskin GDC 2012): a query is a flat associative array of
// facts. Each rule is a tuple of interval criteria that must ALL pass; a
// criterion naming a fact absent from the query rejects the rule. Score =
// criteria count, so specific rules beat general ones. Rules are kept sorted
// by score descending, so the search early-outs once a score tier matches.
#include "dialog.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// ── Symbol table ────────────────────────────────────────────────────
static char sym_names[DLG_MAX_SYMBOLS][DLG_SYMBOL_LEN];
static int sym_count = 0;

int dlg_intern(const char *s) {
    if (!s || !s[0]) return -1;
    for (int i = 0; i < sym_count; i++)
        if (strcmp(sym_names[i], s) == 0) return i;
    if (sym_count >= DLG_MAX_SYMBOLS) return -1;
    strncpy(sym_names[sym_count], s, DLG_SYMBOL_LEN - 1);
    sym_names[sym_count][DLG_SYMBOL_LEN - 1] = '\0';
    return sym_count++;
}

const char *dlg_symbol_name(int id) {
    if (id < 0 || id >= sym_count) return "?";
    return sym_names[id];
}

// ── Database ────────────────────────────────────────────────────────
static DlgRule rules[DLG_MAX_RULES];
static int rule_count = 0;
static char text_pool[DLG_TEXT_POOL];
static int text_used = 0;
static char last_error[160] = "";
static float dlg_now = 0;

// Memory — persistent facts written by rules or code, merged into queries
typedef struct {
    int key;
    float value;
    float expire_at;  // 0 = never
    bool used;
} DlgMemFact;
static DlgMemFact memory[DLG_MAX_MEMORY];

void dlg_set_time(float now) { dlg_now = now; }

void dlg_reset(void) {
    rule_count = 0;
    text_used = 0;
    last_error[0] = '\0';
}

int dlg_rule_count(void) { return rule_count; }
const char *dlg_last_error(void) { return last_error; }

// ── Memory store ────────────────────────────────────────────────────
static DlgMemFact *mem_find(int key) {
    for (int i = 0; i < DLG_MAX_MEMORY; i++)
        if (memory[i].used && memory[i].key == key) return &memory[i];
    return NULL;
}

static void mem_set_id(int key, float value, float ttl, bool increment) {
    if (key < 0) return;
    DlgMemFact *m = mem_find(key);
    if (!m) {
        for (int i = 0; i < DLG_MAX_MEMORY; i++)
            if (!memory[i].used) { m = &memory[i]; break; }
        if (!m) return;
        m->used = true;
        m->key = key;
        m->value = 0;
    }
    m->value = increment ? m->value + value : value;
    m->expire_at = ttl > 0 ? dlg_now + ttl : 0;
}

void dlg_mem_set(const char *key, float value, float ttl) {
    mem_set_id(dlg_intern(key), value, ttl, false);
}

float dlg_mem_get(const char *key, bool *found) {
    DlgMemFact *m = mem_find(dlg_intern(key));
    bool ok = m && (m->expire_at <= 0 || m->expire_at > dlg_now);
    if (found) *found = ok;
    return ok ? m->value : 0;
}

void dlg_mem_clear(void) {
    memset(memory, 0, sizeof(memory));
}

// ── Query ───────────────────────────────────────────────────────────
void dlg_query_init(DlgQuery *q) { q->count = 0; }

void dlg_query_add(DlgQuery *q, const char *key, float value) {
    if (q->count >= DLG_MAX_QUERY) return;
    int k = dlg_intern(key);
    if (k < 0) return;
    q->facts[q->count].key = k;
    q->facts[q->count].value = value;
    q->count++;
}

void dlg_query_add_sym(DlgQuery *q, const char *key, const char *value) {
    dlg_query_add(q, key, (float)dlg_intern(value));
}

// Look up a fact: query first, then live (unexpired) memory.
static bool fact_lookup(const DlgQuery *q, int key, float *out) {
    for (int i = 0; i < q->count; i++)
        if (q->facts[i].key == key) { *out = q->facts[i].value; return true; }
    DlgMemFact *m = mem_find(key);
    if (m && (m->expire_at <= 0 || m->expire_at > dlg_now)) {
        *out = m->value;
        return true;
    }
    return false;
}

// ── Matching ────────────────────────────────────────────────────────
static bool rule_matches(const DlgRule *r, const DlgQuery *q) {
    if (r->disabled) return false;
    if (r->resay > 0 && dlg_now - r->last_said < r->resay) return false;
    // Never-twice (Firewatch): once every variant has been heard, a rule
    // without an explicit `resay` goes quiet for the rest of the run.
    // resay is the opt-in for recycling ambient material.
    if (r->resay <= 0 && r->line_count > 0) {
        unsigned int full = (1u << r->line_count) - 1u;
        if ((r->said_mask & full) == full) return false;
    }
    for (int i = 0; i < r->crit_count; i++) {
        float v;
        if (!fact_lookup(q, r->crit[i].key, &v)) return false;
        if (v < r->crit[i].lo || v > r->crit[i].hi) return false;
    }
    return true;
}

bool dlg_match(const DlgQuery *q, DlgResponse *out) {
    int best_score = -1;
    DlgRule *ties[16];
    int tie_count = 0;
    // Rules are sorted by crit_count descending — once we have a match, any
    // rule with fewer criteria can't win, so we stop at the tier boundary.
    for (int i = 0; i < rule_count; i++) {
        DlgRule *r = &rules[i];
        if (best_score >= 0 && r->crit_count < best_score) break;
        if (!rule_matches(r, q)) continue;
        if (r->crit_count > best_score) {
            best_score = r->crit_count;
            tie_count = 0;
        }
        if (tie_count < 16) ties[tie_count++] = r;
    }
    if (tie_count == 0) return false;

    DlgRule *r = ties[rand() % tie_count];
    // Variant cycling: pick randomly among unsaid variants; when the whole
    // response group has been heard, reset and start over.
    unsigned int full = (1u << r->line_count) - 1u;
    if ((r->said_mask & full) == full) r->said_mask = 0;
    int avail[DLG_MAX_VARIANTS], n = 0;
    for (int i = 0; i < r->line_count; i++)
        if (!(r->said_mask & (1u << i))) avail[n++] = i;
    int v = n > 0 ? avail[rand() % n] : 0;

    out->rule = r;
    out->variant = v;
    out->line = r->lines[v];
    return true;
}

void dlg_commit(DlgResponse *res) {
    DlgRule *r = res->rule;
    if (!r) return;
    r->said_mask |= (1u << res->variant);
    r->last_said = dlg_now;
    if (r->norepeat) r->disabled = true;  // response removes itself (slide 143)
    for (int i = 0; i < r->apply_count; i++)
        mem_set_id(r->apply[i].key, r->apply[i].value, r->apply[i].ttl,
                   r->apply[i].increment);
}

// ── Parser ──────────────────────────────────────────────────────────
// Line-based, writer-facing. See assets/dialogue/ev.rules for the format.

static void parse_error(const char *src, int lineno, const char *msg) {
    if (last_error[0]) return;  // keep the first error
    snprintf(last_error, sizeof(last_error), "%s:%d: %s", src, lineno, msg);
}

static const char *pool_store(const char *s, int len) {
    if (text_used + len + 1 > DLG_TEXT_POOL) return NULL;
    char *dst = &text_pool[text_used];
    memcpy(dst, s, (size_t)len);
    dst[len] = '\0';
    text_used += len + 1;
    return dst;
}

// Value token → float. Numbers parse as numbers; identifiers intern.
static float parse_value(const char *tok) {
    char *end;
    float f = strtof(tok, &end);
    if (end != tok && *end == '\0') return f;
    return (float)dlg_intern(tok);
}

// One criterion token: key=v, key=lo..hi, key<v, key<=v, key>v, key>=v
static bool parse_criterion(DlgRule *r, const char *tok) {
    if (r->crit_count >= DLG_MAX_CRIT) return false;
    char key[DLG_SYMBOL_LEN];
    const char *op = strpbrk(tok, "=<>");
    if (!op || op == tok) return false;
    int klen = (int)(op - tok);
    if (klen >= DLG_SYMBOL_LEN) return false;
    memcpy(key, tok, (size_t)klen);
    key[klen] = '\0';

    DlgCriterion *c = &r->crit[r->crit_count];
    c->key = dlg_intern(key);
    char o = *op++;
    bool orEq = (*op == '=');
    if (orEq) op++;
    if (!*op) return false;

    if (o == '=') {
        const char *dots = strstr(op, "..");
        if (dots) {
            char lo[32];
            int llen = (int)(dots - op);
            if (llen <= 0 || llen >= 32) return false;
            memcpy(lo, op, (size_t)llen);
            lo[llen] = '\0';
            c->lo = parse_value(lo);
            c->hi = parse_value(dots + 2);
        } else {
            c->lo = c->hi = parse_value(op);
        }
    } else if (o == '<') {
        float v = parse_value(op);
        c->lo = -INFINITY;
        c->hi = orEq ? v : v - 0.0001f;
    } else {  // '>'
        float v = parse_value(op);
        c->lo = orEq ? v : v + 0.0001f;
        c->hi = INFINITY;
    }
    r->crit_count++;
    return true;
}

// remember token: key=value, key+=delta, optional @ttl suffix (key=1@6)
static bool parse_apply(DlgRule *r, const char *tok) {
    if (r->apply_count >= DLG_MAX_APPLY) return false;
    char buf[96];
    strncpy(buf, tok, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    DlgApply *a = &r->apply[r->apply_count];
    a->ttl = 0;
    char *at = strrchr(buf, '@');
    if (at) { a->ttl = strtof(at + 1, NULL); *at = '\0'; }

    char *eq = strchr(buf, '=');
    if (!eq || eq == buf) return false;
    a->increment = (eq[-1] == '+');
    if (a->increment) eq[-1] = '\0';
    *eq = '\0';
    a->key = dlg_intern(buf);
    a->value = parse_value(eq + 1);
    r->apply_count++;
    return true;
}

bool dlg_parse(const char *text, const char *srcname) {
    last_error[0] = '\0';
    DlgRule *r = NULL;
    int lineno = 0;
    const char *p = text;

    while (*p) {
        lineno++;
        // Grab one line
        const char *eol = strchr(p, '\n');
        int len = eol ? (int)(eol - p) : (int)strlen(p);
        char line[512];
        if (len >= (int)sizeof(line)) len = (int)sizeof(line) - 1;
        memcpy(line, p, (size_t)len);
        line[len] = '\0';
        p = eol ? eol + 1 : p + len;

        // Strip comments + leading whitespace
        char *hash = strchr(line, '#');
        if (hash) *hash = '\0';
        char *s = line;
        while (*s == ' ' || *s == '\t' || *s == '\r') s++;
        if (!*s) continue;

        char *save = NULL;
        char *word = strtok_r(s, " \t\r", &save);
        if (!word) continue;

        if (strcmp(word, "rule") == 0) {
            if (r) { parse_error(srcname, lineno, "missing 'end' before new rule"); return false; }
            if (rule_count >= DLG_MAX_RULES) { parse_error(srcname, lineno, "too many rules"); return false; }
            r = &rules[rule_count];
            memset(r, 0, sizeof(*r));
            r->last_said = -1e9f;  // "never" — 0 would collide with game start
            r->who = -1;
            r->then_who = -1;
            r->then_concept = -1;
            char *name = strtok_r(NULL, " \t\r", &save);
            if (name) { strncpy(r->name, name, DLG_RULE_NAME_LEN - 1); }
        } else if (!r) {
            parse_error(srcname, lineno, "directive outside rule");
            return false;
        } else if (strcmp(word, "who") == 0) {
            char *whom = strtok_r(NULL, " \t\r", &save);
            if (!whom) { parse_error(srcname, lineno, "who needs a name"); return false; }
            r->who = dlg_intern(whom);
            // 'who' is also a criterion — partitioning key in Ruskin's terms
            DlgCriterion *c = &r->crit[r->crit_count++];
            c->key = dlg_intern("who");
            c->lo = c->hi = (float)r->who;
        } else if (strcmp(word, "criteria") == 0) {
            char *tok;
            while ((tok = strtok_r(NULL, " \t\r", &save)) != NULL) {
                if (!parse_criterion(r, tok)) {
                    parse_error(srcname, lineno, "bad criterion");
                    return false;
                }
            }
        } else if (strcmp(word, "say") == 0) {
            if (r->line_count >= DLG_MAX_VARIANTS) { parse_error(srcname, lineno, "too many say variants"); return false; }
            char *rest = save;  // remainder of line
            while (rest && (*rest == ' ' || *rest == '\t')) rest++;
            if (!rest || !*rest) { parse_error(srcname, lineno, "say needs text"); return false; }
            int rlen = (int)strlen(rest);
            while (rlen > 0 && (rest[rlen-1] == ' ' || rest[rlen-1] == '\r')) rlen--;
            if (rlen >= 2 && rest[0] == '"' && rest[rlen-1] == '"') { rest++; rlen -= 2; }
            const char *stored = pool_store(rest, rlen);
            if (!stored) { parse_error(srcname, lineno, "text pool full"); return false; }
            r->lines[r->line_count++] = stored;
        } else if (strcmp(word, "remember") == 0) {
            char *tok;
            while ((tok = strtok_r(NULL, " \t\r", &save)) != NULL) {
                if (!parse_apply(r, tok)) {
                    parse_error(srcname, lineno, "bad remember clause");
                    return false;
                }
            }
        } else if (strcmp(word, "then") == 0) {
            char *whom = strtok_r(NULL, " \t\r", &save);
            char *concept = strtok_r(NULL, " \t\r", &save);
            if (!whom || !concept) { parse_error(srcname, lineno, "then needs: who concept [after N]"); return false; }
            r->then_who = dlg_intern(whom);
            r->then_concept = dlg_intern(concept);
            r->then_delay = 0.4f;
            char *after = strtok_r(NULL, " \t\r", &save);
            if (after && strcmp(after, "after") == 0) {
                char *secs = strtok_r(NULL, " \t\r", &save);
                if (secs) r->then_delay = strtof(secs, NULL);
            }
        } else if (strcmp(word, "norepeat") == 0) {
            r->norepeat = true;
        } else if (strcmp(word, "resay") == 0) {
            char *secs = strtok_r(NULL, " \t\r", &save);
            if (secs) r->resay = strtof(secs, NULL);
        } else if (strcmp(word, "end") == 0) {
            if (r->line_count == 0) { parse_error(srcname, lineno, "rule has no say lines"); return false; }
            if (r->who < 0) { parse_error(srcname, lineno, "rule has no who"); return false; }
            rule_count++;
            r = NULL;
        } else {
            parse_error(srcname, lineno, "unknown directive");
            return false;
        }
    }
    if (r) { parse_error(srcname, lineno, "unterminated rule (missing 'end')"); return false; }

    // Sort by criteria count descending — enables the early-out in dlg_match.
    // Insertion sort: stable, and the table is small.
    for (int i = 1; i < rule_count; i++) {
        DlgRule tmp = rules[i];
        int j = i - 1;
        while (j >= 0 && rules[j].crit_count < tmp.crit_count) {
            rules[j + 1] = rules[j];
            j--;
        }
        rules[j + 1] = tmp;
    }
    return true;
}

bool dlg_load_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(last_error, sizeof(last_error), "%s: cannot open", path);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size <= 0 || size > 1024 * 1024) { fclose(f); return false; }
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return false; }
    size_t got = fread(buf, 1, (size_t)size, f);
    buf[got] = '\0';
    fclose(f);
    bool ok = dlg_parse(buf, path);
    free(buf);
    return ok;
}
