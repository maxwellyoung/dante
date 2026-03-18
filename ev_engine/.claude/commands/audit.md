Audit the codebase against the master plan and design commandments.

1. Read `MASTER_PLAN.md` sections VI (Object Inventory) and VII (Development Phases)
2. Check every object in the inventory against `src/scene.c` — report YES/NO for each
3. Run `make check` — report any static analysis warnings
4. Run `make test` — report test results
5. Check for code smells: magic numbers, dead code, missing error handling
6. Report wall counts per scene vs targets (critical scenes need 300+)
7. Check all 10 Design Commandments against current implementation

Output a scorecard with pass/fail per category.
