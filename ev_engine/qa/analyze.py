#!/usr/bin/env python3
"""
EV QA Analyzer — The Harsh Critic
Reads qa/report.json, scores every pillar, outputs brutal honest feedback.
No mercy. If it's not visible at 480x300, it doesn't exist.

Pillars scored 1-5:
  COMPOSITION  — Bold shapes, spatial balance, edge density, visual complexity
  LIGHTING     — Dynamic range, contrast, drama, no flat/unlit areas
  MATERIALS    — Surface variety, non-concrete coverage, material richness
  COLOR        — Hue diversity, warmth/coolness, accent presence, Godard palette
  INTERACTION  — Object count, reachability, diegetic consequence potential
  ANTI-PATTERNS — Horror vibes, grey-on-grey, invisible geometry, yellow-tint

Design commandments (from CLAUDE.md):
  - If it's not visible as a 3px element at 480x300, it's not visible
  - Neutral warm whites + SPECIFIC Pantone accents
  - Wonder and melancholy, not horror. Arrival, not escape.
  - Every interaction changes the world
  - Earth glow is emotional anchor — make it a landmark
  - No grey-on-grey in space — hull must pop
"""

import json
import sys
import os

def normalize_scene(scene):
    """Support both legacy flat metrics and current nested angle metrics."""
    normalized = dict(scene)
    angles = scene.get("angles") or []
    hero = next((angle for angle in angles if angle.get("name") == "hero"), None)
    if hero is None and angles:
        hero = angles[0]
    if hero:
        for key, value in hero.items():
            if key == "name":
                continue
            normalized[key] = value
    return normalized

# ── Scoring functions ──────────────────────────────────────────────

def score_composition(s):
    """Bold shapes at 480x300. Readable geometry. Visual interest."""
    score = 5.0
    notes = []

    edge = s.get("edge_density", 0)
    contrast = s.get("contrast_ratio", 1)
    center = s.get("center_luma", 0)
    lr = s.get("lr_balance", 0)
    dark = s.get("dark_by_design", False)

    cvar = s.get("color_variance", 0)

    if not dark:
        if edge < 5:
            # Low edges BUT high color variance = intentional color fields (Rothko)
            if cvar > 2000:
                score -= 0.5
                notes.append(f"edge density {edge:.0f}% — soft edges, but color fields carry it")
            else:
                score -= 2.0
                notes.append(f"edge density {edge:.0f}% — no readable geometry at 480x300")
        elif edge < 12:
            score -= 0.5
            notes.append(f"edge density {edge:.0f}% — sparse, could use more visual structure")
        elif edge > 30:
            notes.append(f"edge density {edge:.0f}% — rich visual texture")

        if contrast < 1.3:
            score -= 1.5
            notes.append(f"contrast {contrast:.1f}:1 — flat composition, no visual hierarchy")
        elif contrast > 3:
            notes.append(f"contrast {contrast:.1f}:1 — strong visual drama")

        if center < 15:
            score -= 1.0
            notes.append(f"center luma {center:.0f} — frame center is dead space")
    else:
        # Dark scenes: need SOME visible structure
        if edge < 1 and cvar < 500:
            score -= 1.5
            notes.append(f"edge density {edge:.0f}% — even dark scenes need visible form")
        elif edge < 1:
            score -= 0.5
            notes.append(f"edge density {edge:.0f}% — dark but has color presence")
        if center < 2 and s.get("luma", 0) < 2:
            score -= 1.0
            notes.append("pure void — needs at least one anchor point")

    return max(1, min(5, round(score))), notes


def score_lighting(s):
    """Every scene needs drama. Key/fill must create depth."""
    score = 5.0
    notes = []
    dark = s.get("dark_by_design", False)
    luma = s.get("luma", 0)
    black = s.get("black_pct", 0)
    bright = s.get("bright_pct", 0)
    mid = s.get("mid_tone_pct", 0)
    quads = s.get("quadrant_luma", [0,0,0,0])

    if not dark:
        if luma < 10:
            score -= 3
            notes.append(f"luma {luma:.0f} — scene is nearly invisible")
        elif luma < 40:
            score -= 1.5
            notes.append(f"luma {luma:.0f} — underlit, key light not reaching surfaces")
        elif luma > 200:
            score -= 1
            notes.append(f"luma {luma:.0f} — over-bright, losing shadow detail")

        if black > 60:
            score -= 2
            notes.append(f"{black:.0f}% black — massive unlit regions")
        elif black > 30:
            score -= 0.5
            notes.append(f"{black:.0f}% black — significant shadow areas")

        if bright > 40:
            score -= 1.5
            notes.append(f"{bright:.0f}% blown out — exposure too high")

        if mid < 20:
            score -= 1
            notes.append(f"only {mid:.0f}% mid-tones — harsh binary lighting")

        # Check if all quadrants are similar (no directional light)
        if len(quads) == 4 and max(quads) > 0:
            q_range = max(quads) - min(quads)
            if q_range < 10:
                score -= 1
                notes.append(f"quadrant range {q_range:.0f} — ambient wash, no key/fill direction")
    else:
        # Dark scenes still need readable elements
        if mid < 3:
            score -= 1.5
            notes.append(f"only {mid:.0f}% mid-tones — even dark scenes need readable elements")
        # But reward good contrast
        if s.get("contrast_ratio", 1) > 3:
            notes.append("good dark-scene contrast — light pools work")

    return max(1, min(5, round(score))), notes


def score_materials(s):
    """Surface variety. Procedural detail. No monotone concrete."""
    score = 5.0
    notes = []
    types = s.get("material_types", 1)
    cov = s.get("material_coverage", 0)
    walls = s.get("walls", 0)
    breakdown = s.get("material_breakdown", {})

    if walls < 10:
        return 3, ["too few walls to judge materials"]

    if types <= 2:
        score -= 2.5
        notes.append(f"only {types} material types — monotone surfaces")
    elif types <= 4:
        score -= 1
        notes.append(f"{types} material types — adequate but could be richer")
    elif types >= 6:
        notes.append(f"{types} material types — rich surface vocabulary")

    if cov < 20:
        score -= 2
        notes.append(f"{cov:.0f}% non-concrete — brutalist by accident, not by design")
    elif cov < 40:
        score -= 1
        notes.append(f"{cov:.0f}% non-concrete — needs more surface assignment")
    elif cov > 70:
        notes.append(f"{cov:.0f}% coverage — well-tagged surfaces")

    # Check concrete dominance
    concrete = breakdown.get("concrete", 0)
    if concrete > walls * 0.7 and walls > 20:
        score -= 1
        notes.append(f"{concrete}/{walls} walls are raw concrete — tag your surfaces")

    return max(1, min(5, round(score))), notes


def score_color(s):
    """Godard palette. Neutral warm whites + specific Pantone accents."""
    score = 5.0
    notes = []
    hues = s.get("hue_buckets", 0)
    warmth = s.get("warmth", 0)
    cvar = s.get("color_variance", 0)
    dark = s.get("dark_by_design", False)
    rgb = s.get("avg_rgb", [0,0,0])
    name = s.get("name", "")

    if dark:
        if hues < 2:
            score -= 1
            notes.append(f"only {hues} hue — dark scenes still benefit from colored accents")
        return max(1, min(5, round(score))), notes

    if hues < 2:
        score -= 2.5
        notes.append(f"only {hues} hue bucket — completely monotone")
    elif hues < 4:
        score -= 1.5
        notes.append(f"only {hues} hue buckets — needs accent colors")
    elif hues >= 8:
        notes.append(f"{hues} hue buckets — good color richness")

    if cvar < 200:
        score -= 2
        notes.append(f"variance {cvar:.0f} — dead flat, no color separation")
    elif cvar < 1000:
        score -= 1
        notes.append(f"variance {cvar:.0f} — low color energy")

    # Temperature check per scene type
    if "space" in name:
        # Space should have cool tones (blue) but with warm accents
        if warmth > 30:
            score -= 0.5
            notes.append(f"warmth +{warmth:.0f} — space scenes should read cooler")
    elif name in ("room", "hallway", "lobby"):
        # Paris hotel should be warm (golden)
        if warmth < -10:
            score -= 1
            notes.append(f"warmth {warmth:.0f} — Hotel Chevalier needs golden warmth")
        elif warmth > 5:
            notes.append(f"warmth +{warmth:.0f} — good golden tone")

    # Grey check: if all RGB within ±5 of each other = pure grey
    if len(rgb) == 3 and max(rgb) - min(rgb) < 8 and max(rgb) > 50:
        score -= 1.5
        notes.append(f"avg RGB [{rgb[0]:.0f},{rgb[1]:.0f},{rgb[2]:.0f}] — pure grey, needs color identity")

    return max(1, min(5, round(score))), notes


TRANSITIONAL_SCENES = {"hallway", "hotel_ext", "elevator", "hyperspace", "taxi", "space_corridor", "return_taxi"}
CUTSCENE_SCENES = {"bed", "stars", "cleaned_suite"}


def score_interaction(s):
    """Diegetic, visible consequence. Every interaction changes the world."""
    score = 5.0
    notes = []
    count = s.get("interact_count", 0)
    unreachable = s.get("obj_unreachable", 0)
    name = s.get("name", "")

    if name in CUTSCENE_SCENES:
        return 5, ["non-playable scene — object density not applicable"]

    # Some scenes are transitional — no objects expected
    transitional = name in TRANSITIONAL_SCENES
    if transitional:
        if count == 0:
            return 3, ["transitional scene — no objects expected"]
        else:
            notes.append(f"{count} objects in a transitional scene — nice density")
            return min(5, 3 + count), notes

    # Interactive scenes should have objects
    if count == 0:
        score -= 3
        notes.append("no interactive objects — the player has nothing to DO")
    elif count == 1:
        score -= 1
        notes.append("only 1 object — needs more diegetic interaction")
    elif count >= 4:
        notes.append(f"{count} objects — good interaction density")

    if unreachable > 0:
        score -= 2
        notes.append(f"{unreachable} object(s) unreachable from spawn — player can't find them")

    return max(1, min(5, round(score))), notes


def score_antipatterns(s):
    """Horror vibes, grey monotone, invisible geometry, yellow-tint."""
    score = 5.0
    notes = []
    warmth = s.get("warmth", 0)
    luma = s.get("luma", 0)
    contrast = s.get("contrast_ratio", 1)
    hues = s.get("hue_buckets", 0)
    name = s.get("name", "")
    dark = s.get("dark_by_design", False)
    rgb = s.get("avg_rgb", [0,0,0])

    # Horror check: blue-green dominance + low luma + high contrast
    if warmth < -15 and luma < 40 and not dark:
        score -= 2
        notes.append(f"HORROR VIBES — cold ({warmth:.0f}) + dark (luma {luma:.0f}). This is wonder, not dread.")

    # Grey-on-grey in space
    if "space" in name and hues < 3 and not dark:
        score -= 2
        notes.append(f"GREY-ON-GREY — {hues} hues in space. Hull must pop against void, not blend into fog.")

    # Yellow-tinting everything
    if len(rgb) == 3 and rgb[0] > 150 and rgb[1] > 130 and rgb[2] < rgb[1] - 30:
        if name not in ("room", "hallway"):  # room/hallway SHOULD be warm
            score -= 1
            notes.append(f"YELLOW TINT — [{rgb[0]:.0f},{rgb[1]:.0f},{rgb[2]:.0f}]. Neutral base, SPECIFIC accents.")

    # Earth glow check for balcony/space scenes
    if name == "balcony":
        # Bottom half should have blue glow (Earth)
        quads = s.get("quadrant_luma", [0,0,0,0])
        if len(quads) == 4:
            bottom_avg = (quads[2] + quads[3]) / 2
            if bottom_avg < 5:
                score -= 1.5
                notes.append(f"EARTH GLOW MISSING — bottom luma {bottom_avg:.0f}. Earth is the emotional anchor. Make it a landmark.")

    # Space lobby must have Earth visible
    if name == "space_lobby":
        quads = s.get("quadrant_luma", [0,0,0,0])
        if len(quads) == 4 and all(q < 8 for q in quads):
            score -= 1
            notes.append("EARTH NOT VISIBLE — observation window should show Earth glow")

    return max(1, min(5, round(score))), notes


# ── Report generation ──────────────────────────────────────────────

def grade_letter(score):
    if score >= 4.5: return "A"
    if score >= 3.5: return "B"
    if score >= 2.5: return "C"
    if score >= 1.5: return "D"
    return "F"


def analyze(report_path):
    with open(report_path) as f:
        report = json.load(f)

    scenes = report["scenes"]
    total_score = 0
    total_possible = 0
    pillar_totals = {p: 0 for p in ["composition", "lighting", "materials", "color", "interaction", "antipatterns"]}
    pillar_counts = {p: 0 for p in pillar_totals}

    print()
    print("=" * 72)
    print("  ENDEARING VOID — QA ANALYSIS REPORT")
    print("  \"If it's not visible as a 3px element, it's not visible.\"")
    print("=" * 72)
    print()

    normalized_scenes = [normalize_scene(scene) for scene in scenes]

    for s in normalized_scenes:
        name = s["name"]
        comp_score, comp_notes = score_composition(s)
        light_score, light_notes = score_lighting(s)
        mat_score, mat_notes = score_materials(s)
        color_score, color_notes = score_color(s)
        inter_score, inter_notes = score_interaction(s)
        anti_score, anti_notes = score_antipatterns(s)

        scores = {
            "composition": comp_score,
            "lighting": light_score,
            "materials": mat_score,
            "color": color_score,
            "interaction": inter_score,
            "antipatterns": anti_score,
        }
        all_notes = {
            "composition": comp_notes,
            "lighting": light_notes,
            "materials": mat_notes,
            "color": color_notes,
            "interaction": inter_notes,
            "antipatterns": anti_notes,
        }

        scene_total = sum(scores.values())
        scene_max = len(scores) * 5
        scene_pct = 100 * scene_total / scene_max
        total_score += scene_total
        total_possible += scene_max

        for p in pillar_totals:
            pillar_totals[p] += scores[p]
            pillar_counts[p] += 1

        grade = grade_letter(scene_total / len(scores))
        bar = "█" * int(scene_pct / 5) + "░" * (20 - int(scene_pct / 5))

        print(f"┌─ {name.upper()} {'─' * (50 - len(name))} {grade} ({scene_total}/{scene_max})")
        print(f"│  {bar}  {scene_pct:.0f}%")
        print(f"│")

        for pillar, sc in scores.items():
            label = pillar.upper()[:11].ljust(11)
            stars = "★" * sc + "☆" * (5 - sc)
            print(f"│  {label}  {stars}  ", end="")
            notes = all_notes[pillar]
            if notes:
                print(notes[0])
                for n in notes[1:]:
                    print(f"│  {'':11}         {n}")
            else:
                print("—")

        print(f"└{'─' * 71}")
        print()

    # ── Overall summary ──
    overall_pct = 100 * total_score / total_possible if total_possible > 0 else 0
    overall_grade = grade_letter(total_score / total_possible * 5) if total_possible > 0 else "F"

    print("=" * 72)
    print(f"  OVERALL: {overall_grade}  ({total_score}/{total_possible} = {overall_pct:.0f}%)")
    print("=" * 72)
    print()

    # Per-pillar averages
    print("  PILLAR AVERAGES:")
    worst_pillar = None
    worst_avg = 6
    for p in ["composition", "lighting", "materials", "color", "interaction", "antipatterns"]:
        avg = pillar_totals[p] / pillar_counts[p] if pillar_counts[p] > 0 else 0
        label = p.upper()[:14].ljust(14)
        stars = "★" * round(avg) + "☆" * (5 - round(avg))
        grade = grade_letter(avg)
        print(f"    {label}  {stars}  {avg:.1f}/5  ({grade})")
        if avg < worst_avg:
            worst_avg = avg
            worst_pillar = p

    print()

    # ── Urgent action items ──
    print("  URGENT FIXES:")
    urgent_count = 0
    for s in normalized_scenes:
        issues = []
        if s.get("edge_density", 99) < 3 and not s.get("dark_by_design"):
            issues.append("no visible geometry in hero shot — FIX CAMERA ANGLE or scene geometry")
        if s.get("luma", 99) < 10 and not s.get("dark_by_design"):
            issues.append("scene is invisible — FIX LIGHTING")
        if s.get("interact_count", 0) == 0 and s["name"] in ("lobby", "room", "bathroom", "space_lobby", "space_suite"):
            issues.append("interactive scene has NO objects — players need something to DO")
        if "space" in s["name"] and s.get("hue_buckets", 99) < 3 and not s.get("dark_by_design"):
            issues.append("grey-on-grey in space — hull walls need color separation")
        for issue in issues:
            print(f"    [{s['name'].upper()}] {issue}")
            urgent_count += 1

    if urgent_count == 0:
        print("    (none — nice work)")

    print()

    if worst_pillar:
        print(f"  WEAKEST PILLAR: {worst_pillar.upper()} ({worst_avg:.1f}/5)")
        print(f"  Focus your next iteration here.")
    print()

    # Write machine-readable grades
    grades = {
        "overall": {"grade": overall_grade, "score": total_score, "max": total_possible, "pct": round(overall_pct)},
        "pillars": {},
        "scenes": {}
    }
    for p in pillar_totals:
        avg = pillar_totals[p] / pillar_counts[p] if pillar_counts[p] > 0 else 0
        grades["pillars"][p] = {"avg": round(avg, 1), "grade": grade_letter(avg)}
    for s in normalized_scenes:
        cs, _ = score_composition(s)
        ls, _ = score_lighting(s)
        ms, _ = score_materials(s)
        cos, _ = score_color(s)
        ins, _ = score_interaction(s)
        ans, _ = score_antipatterns(s)
        total = cs + ls + ms + cos + ins + ans
        grades["scenes"][s["name"]] = {
            "grade": grade_letter(total / 6),
            "total": total,
            "composition": cs, "lighting": ls, "materials": ms,
            "color": cos, "interaction": ins, "antipatterns": ans
        }

    grades_path = os.path.join(os.path.dirname(report_path), "grades.json")
    with open(grades_path, "w") as f:
        json.dump(grades, f, indent=2)

    return 0 if overall_pct >= 70 else 1


def analyze_health(health_path):
    """Read health-latest.json and print a human-readable report."""
    try:
        with open(health_path) as f:
            h = json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"  (health report not available: {e})")
        return

    print()
    print("=" * 56)
    print("  HEALTH CHECK REPORT")
    print("=" * 56)
    print()

    ts = h.get("timestamp", "unknown")
    print(f"  Timestamp:    {ts}")

    compile_ok = h.get("compile", {}).get("ok", False)
    print(f"  Compile:      {'OK' if compile_ok else 'FAILED'}")
    if not compile_ok:
        errors = h.get("compile", {}).get("errors", "")
        if errors:
            for line in str(errors).split("\n")[:10]:
                print(f"    {line}")

    print(f"  Screenshots:  {h.get('screenshots', 0)}")
    print(f"  Report:       {'generated' if h.get('report_generated') else 'missing'}")
    print(f"  QA exit code: {h.get('qa_exit_code', '?')}")
    print()

    scenes = h.get("scenes", [])
    if scenes:
        print("  Per-scene status:")
        for s in scenes:
            status = s.get("status", "?")
            icon = "OK" if status == "PASS" else "!!"
            print(f"    [{icon}] {s.get('name', '?')}")
    print()

    total_issues = h.get("total_issues", 0)
    if total_issues == 0 and compile_ok:
        print("  ALL CLEAR")
    else:
        print(f"  {total_issues} issue(s) detected")
    print()


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "qa/report.json"

    # If --health flag or health JSON path passed, analyze health
    if len(sys.argv) > 1 and sys.argv[1] == "--health":
        health_path = sys.argv[2] if len(sys.argv) > 2 else "qa/health-latest.json"
        analyze_health(health_path)
        sys.exit(0)

    # If path ends with health-latest.json, analyze that instead
    if path.endswith("health-latest.json"):
        analyze_health(path)
        sys.exit(0)

    sys.exit(analyze(path))
