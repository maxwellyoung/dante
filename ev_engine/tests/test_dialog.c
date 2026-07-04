// test_dialog.c — headless tests for the contextual dialogue rule database
// Compiles dialog.c directly (pure core, no Raylib).
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/dialog.h"

static int tests_run = 0, tests_failed = 0;
#define CHECK(cond, msg) do { \
    tests_run++; \
    if (!(cond)) { tests_failed++; printf("  FAIL: %s\n", msg); } \
} while (0)

static const char *RULES =
    "# test rules\n"
    "rule general_greet\n"
    "who gibbons\n"
    "criteria concept=greet\n"
    "say \"Hello.\"\n"
    "end\n"
    "\n"
    "rule lobby_greet\n"
    "who gibbons\n"
    "criteria concept=greet scene=lobby\n"
    "say \"Welcome to the lobby.\"\n"
    "say \"The lobby. Again.\"\n"
    "remember greeted=1 greet_count+=1\n"
    "then gibbons aside after 1.5\n"
    "end\n"
    "\n"
    "rule lobby_greet_low_health\n"
    "who gibbons\n"
    "criteria concept=greet scene=lobby health<30\n"
    "say \"You look terrible, sir.\"\n"
    "end\n"
    "\n"
    "rule range_rule\n"
    "who gibbons\n"
    "criteria concept=count n=3..5\n"
    "say \"Three to five.\"\n"
    "end\n"
    "\n"
    "rule once_rule\n"
    "who gibbons\n"
    "criteria concept=once\n"
    "say \"Only once.\"\n"
    "norepeat\n"
    "end\n"
    "\n"
    "rule cooldown_rule\n"
    "who gibbons\n"
    "criteria concept=cool\n"
    "say \"Cooled.\"\n"
    "resay 10\n"
    "end\n"
    "\n"
    "rule memory_gate\n"
    "who gibbons\n"
    "criteria concept=greet scene=lobby greeted=1 greet_count>=2\n"
    "say \"We keep meeting.\"\n"
    "end\n";

static bool speak(const char *concept, const char *scene, float health,
                  DlgResponse *out) {
    DlgQuery q;
    dlg_query_init(&q);
    dlg_query_add_sym(&q, "who", "gibbons");
    dlg_query_add_sym(&q, "concept", concept);
    if (scene) dlg_query_add_sym(&q, "scene", scene);
    if (health >= 0) dlg_query_add(&q, "health", health);
    return dlg_match(&q, out);
}

int main(void) {
    printf("=== dialog core tests ===\n");
    srand(12345);
    dlg_reset();
    dlg_mem_clear();
    dlg_set_time(0);

    // Parse
    CHECK(dlg_parse(RULES, "test"), "rules parse");
    CHECK(dlg_rule_count() == 7, "7 rules loaded");

    DlgResponse r;

    // Specificity: lobby rule (3 crit) beats general (2 crit)
    CHECK(speak("greet", "lobby", -1, &r), "lobby greet matches");
    CHECK(strcmp(r.rule->name, "lobby_greet") == 0, "specific beats general");

    // Even more specific with low health (4 crit)
    CHECK(speak("greet", "lobby", 12, &r), "low health greet matches");
    CHECK(strcmp(r.rule->name, "lobby_greet_low_health") == 0,
          "most specific wins");

    // Absent fact rejects: no scene → only general rule
    CHECK(speak("greet", NULL, -1, &r), "general greet matches");
    CHECK(strcmp(r.rule->name, "general_greet") == 0,
          "missing fact rejects specific rules");

    // Unknown concept → silence
    CHECK(!speak("nonsense", "lobby", -1, &r), "no match = silence");

    // Range criterion
    DlgQuery q;
    dlg_query_init(&q);
    dlg_query_add_sym(&q, "who", "gibbons");
    dlg_query_add_sym(&q, "concept", "count");
    dlg_query_add(&q, "n", 4);
    CHECK(dlg_match(&q, &r), "range 4 in 3..5 matches");
    dlg_query_init(&q);
    dlg_query_add_sym(&q, "who", "gibbons");
    dlg_query_add_sym(&q, "concept", "count");
    dlg_query_add(&q, "n", 6);
    CHECK(!dlg_match(&q, &r), "range 6 outside 3..5 rejects");

    // Memory write-back + increment + memory-gated rule
    speak("greet", "lobby", -1, &r);
    dlg_commit(&r);  // greeted=1, greet_count=1
    bool found;
    CHECK(dlg_mem_get("greeted", &found) == 1 && found, "remember wrote fact");
    speak("greet", "lobby", -1, &r);
    dlg_commit(&r);  // greet_count=2
    CHECK(dlg_mem_get("greet_count", &found) == 2, "+= increments");
    CHECK(speak("greet", "lobby", -1, &r), "memory-gated rule queried");
    CHECK(strcmp(r.rule->name, "memory_gate") == 0,
          "memory facts merge into query and win on specificity");

    // Followup metadata survives parse
    dlg_mem_clear();  // keep memory_gate out of the way
    CHECK(speak("greet", "lobby", -1, &r), "lobby greet again");
    CHECK(strcmp(r.rule->name, "lobby_greet") == 0, "back to lobby_greet");
    CHECK(r.rule->then_concept >= 0 && r.rule->then_delay == 1.5f,
          "then clause parsed");

    // Variant cycling: both variants heard before any repeat
    dlg_commit(&r);
    int first = r.variant;
    dlg_mem_clear();
    CHECK(speak("greet", "lobby", -1, &r), "variant requery");
    CHECK(strcmp(r.rule->name, "lobby_greet") == 0, "still lobby_greet");
    CHECK(r.variant != first, "second variant differs before cycle resets");

    // norepeat: disabled after commit
    CHECK(speak("once", NULL, -1, &r), "once matches first time");
    dlg_commit(&r);
    CHECK(!speak("once", NULL, -1, &r), "norepeat removes rule");

    // resay cooldown honors the clock
    CHECK(speak("cool", NULL, -1, &r), "cooldown rule matches");
    dlg_commit(&r);
    CHECK(!speak("cool", NULL, -1, &r), "cooldown blocks immediate resay");
    dlg_set_time(11);
    CHECK(speak("cool", NULL, -1, &r), "cooldown expires with time");

    // Fact TTL: expires with the clock
    dlg_mem_set("fleeting", 1, 5);  // now=11, expires at 16
    CHECK(dlg_mem_get("fleeting", &found) == 1 && found, "ttl fact live");
    dlg_set_time(20);
    dlg_mem_get("fleeting", &found);
    CHECK(!found, "ttl fact expired");

    // Parse errors are reported, not crashed on
    dlg_reset();
    CHECK(!dlg_parse("rule broken\nsay \"no who\"\nend\n", "bad"),
          "rule without who rejected");
    CHECK(dlg_last_error()[0] != '\0', "error message set");

    printf("%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
