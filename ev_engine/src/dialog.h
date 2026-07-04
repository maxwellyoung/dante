// dialog.h — Contextual dialogue: rule database + fuzzy pattern matching
// After Elan Ruskin's GDC 2012 "AI-driven Dynamic Dialog" (Valve response rules,
// L4D/TF2/Portal). World state flattens into facts; writers author rules in
// assets/dialogue/*.rules; the most specific matching rule wins; responses
// write memory back and chain followups into conversations.
//
// dialog.c        — pure core (no Raylib, no GameCtx) so it unit-tests headless
// dialog_game.c   — engine binding: builds queries from GameCtx, drives speech
#ifndef EV_DIALOG_H
#define EV_DIALOG_H

#include <stdbool.h>

#define DLG_MAX_SYMBOLS   512
#define DLG_SYMBOL_LEN    40
#define DLG_MAX_RULES     192
#define DLG_MAX_CRIT      12
#define DLG_MAX_VARIANTS  8
#define DLG_MAX_APPLY     4
#define DLG_MAX_QUERY     64
#define DLG_MAX_MEMORY    128
#define DLG_TEXT_POOL     (32 * 1024)
#define DLG_RULE_NAME_LEN 48

// A fact: one piece of world state. Key is an interned symbol; value is a
// float (string values are interned and stored as their symbol id).
typedef struct {
    int key;
    float value;
} DlgFact;

// A criterion: interval test lo <= value <= hi (Ruskin: every comparison is
// the same instruction stream). Fact absent from the query → rule rejects.
typedef struct {
    int key;
    float lo, hi;
} DlgCriterion;

// Memory write-back performed when a rule fires ("remember" clause).
typedef struct {
    int key;
    float value;
    float ttl;        // seconds until the fact expires; 0 = permanent
    bool increment;   // += instead of =
} DlgApply;

typedef struct {
    char name[DLG_RULE_NAME_LEN];
    int who;                              // speaker symbol (also a criterion)
    DlgCriterion crit[DLG_MAX_CRIT];
    int crit_count;                       // == score; more criteria = more specific
    const char *lines[DLG_MAX_VARIANTS];  // response group — variants of one beat
    int line_count;
    unsigned int said_mask;               // variant cycling (reset when exhausted)
    DlgApply apply[DLG_MAX_APPLY];
    int apply_count;
    int then_who;                         // followup speaker symbol, -1 = none
    int then_concept;                     // followup concept symbol
    float then_delay;                     // seconds after line ends
    bool norepeat;                        // disable after firing once
    bool disabled;
    float resay;                          // cooldown seconds between firings
    float last_said;                      // game time of last firing
} DlgRule;

// The flat pile of facts a character throws at the database.
typedef struct {
    DlgFact facts[DLG_MAX_QUERY];
    int count;
} DlgQuery;

// Result of a match. Commit separately so callers can inspect first.
typedef struct {
    DlgRule *rule;
    const char *line;  // chosen variant
    int variant;
} DlgResponse;

// ── Database lifecycle ──
void dlg_reset(void);                    // clear rules + text pool (memory kept)
bool dlg_load_file(const char *path);    // parse a .rules file (additive)
bool dlg_parse(const char *text, const char *srcname);  // additive; for tests
int dlg_rule_count(void);
const char *dlg_last_error(void);        // parse diagnostics ("" if none)

// ── Symbols ──
int dlg_intern(const char *s);           // name → stable id
const char *dlg_symbol_name(int id);

// ── Clock (drives fact TTLs and rule cooldowns) ──
void dlg_set_time(float now);

// ── Character/world memory — persistent store merged into every query ──
void dlg_mem_set(const char *key, float value, float ttl);
float dlg_mem_get(const char *key, bool *found);
void dlg_mem_clear(void);

// ── Query building + matching ──
void dlg_query_init(DlgQuery *q);
void dlg_query_add(DlgQuery *q, const char *key, float value);
void dlg_query_add_sym(DlgQuery *q, const char *key, const char *value);
bool dlg_match(const DlgQuery *q, DlgResponse *out);  // best rule, random tiebreak
void dlg_commit(DlgResponse *r);  // mark variant said, apply remembers, cooldowns

// ── Engine binding (dialog_game.c) ──
void dlg_game_init(void);           // load assets/dialogue/ev.rules
void dlg_game_update(float dt);     // clock, waypoint/idle triggers, followups, F6 reload
bool dlg_game_speak(const char *who, const char *concept);  // returns true if a line played
// Speak about a specific thing — adds object=<name> and step=N facts.
// Two Bots One Wrench: tag the prop, let the database decide if there's a line.
bool dlg_game_speak_about(const char *who, const char *concept,
                          const char *object, float step);
// Convenience for scenes: Gibbons speaks if he's present, else the narrator.
bool dlg_game_remark(const char *concept, const char *object, float step);
bool dlg_game_speaking(void);

#endif
