#!/usr/bin/env python3
"""
EV E2E QA Analyzer — Multi-Profile Harsh Critic

Six user profiles evaluate every scene from radically different perspectives.
No mercy. No corner-cutting. Each profile has its own scoring rubric, priorities,
and failure conditions.

Profiles:
  FIRST_TIMER      — Can I find things? Is anything confusing? Am I lost?
  CINEMATOGRAPHER  — Composition, color theory, lighting craft, film language
  ART_DIRECTOR     — Design commandments, anti-patterns, material consistency
  ACCESSIBILITY    — Contrast ratios, colorblind simulation, readability
  QA_ENGINEER      — Z-fighting, geometry, performance, budget, regressions
  SPEEDRUNNER      — Flow, transitions, stuck points, movement clarity

Each profile scores 1-5 per scene. Combined into weighted composite.

Usage:
  python3 qa/analyze_e2e.py qa/e2e_report.json
  python3 qa/analyze_e2e.py qa/report.json    # works on basic reports too
"""

import json
import sys
import os
import math
from collections import defaultdict

# ── Severity levels ──
CRITICAL = "CRITICAL"
MAJOR = "MAJOR"
MINOR = "MINOR"
NOTE = "NOTE"

TRANSITIONAL_SCENES = {"hallway", "hotel_ext", "elevator", "hyperspace",
                       "taxi", "space_corridor", "return_taxi", "stars"}
CUTSCENE_SCENES = {"bed", "cleaned_suite"}
FLOOR_PROBE_EXEMPT_SCENES = {"balcony", "taxi", "return_taxi", "bed", "stars"}

# ── Design Commandments (from CLAUDE.md) ──
COMMANDMENTS = [
    "Wonder and melancholy, not horror",
    "If it's not 3px at 480x300, it's not visible",
    "Neutral warm whites + SPECIFIC accents",
    "Every interaction changes the world",
    "Earth glow is emotional anchor",
    "No grey-on-grey in space",
    "No hand-holding (no task counter, no beacon)",
    "Diegetic interaction (sounds from objects)",
    "Visible consequence (geometry/lighting change)",
    "The void is the point (waiting, not completing)",
]


def grade_letter(score):
    if score >= 4.5: return "A"
    if score >= 3.5: return "B"
    if score >= 2.5: return "C"
    if score >= 1.5: return "D"
    return "F"


def clamp(v, lo=1, hi=5):
    return max(lo, min(hi, round(v)))


def get_hero_metrics(scene):
    """Extract hero angle metrics (first angle) from scene data."""
    angles = scene.get("angles", [])
    if angles:
        return angles[0]
    # Fallback: metrics are at scene level (basic report format)
    return scene


def get_all_angle_metrics(scene):
    """Get metrics from all angles."""
    return scene.get("angles", [get_hero_metrics(scene)])


# ══════════════════════════════════════════════════════════════════════
# PROFILE 1: FIRST-TIMER
# "I just booted this game. Where do I go? What do I do?"
# ══════════════════════════════════════════════════════════════════════

def profile_first_timer(scene):
    """Evaluates navigation clarity, discoverability, and confusion potential."""
    score = 5.0
    issues = []
    name = scene["name"]
    hero = get_hero_metrics(scene)
    dark = scene.get("dark_by_design", False)

    # Can I see where I am?
    if hero.get("luma", 0) < 15 and not dark:
        score -= 2.5
        issues.append((CRITICAL, "Scene is nearly pitch black — new player will alt-F4"))
    elif hero.get("luma", 0) < 30 and not dark:
        score -= 1.0
        issues.append((MAJOR, f"Very dark (luma {hero.get('luma', 0):.0f}) — disorienting for first play"))

    # Can I find interactive objects?
    obj_count = scene.get("interact_count", 0)
    unreachable = scene.get("obj_unreachable", 0)
    transitional = name in TRANSITIONAL_SCENES
    cutscene = name in CUTSCENE_SCENES
    if not transitional and not cutscene:
        if obj_count == 0:
            score -= 2.0
            issues.append((CRITICAL, "No interactive objects — player has nothing to do"))
        elif obj_count == 1:
            score -= 0.5
            issues.append((MINOR, "Only 1 object — limited discovery"))
        if unreachable > 0:
            score -= 1.5
            issues.append((MAJOR, f"{unreachable} objects too far from spawn — player won't find them"))

    # Is there a clear path forward?
    if scene.get("has_exit", False):
        pass  # Good, there's a destination
    elif not transitional and name not in ("bed", "stars", "balcony", "paris_dream", "cleaned_suite"):
        score -= 0.5
        issues.append((MINOR, "No exit marker — player may not know where to go"))

    # Check spawn angle — what does the player see first?
    angles = get_all_angle_metrics(scene)
    if len(angles) >= 2:
        spawn = angles[1]  # spawn angle
        if spawn.get("luma", 0) < 8 and not dark:
            score -= 1.5
            issues.append((MAJOR, "Spawn view is pitch black — terrible first impression"))
        if spawn.get("edge_density", 0) < 1 and not dark:
            score -= 1.0
            issues.append((MAJOR, "Spawn view has no visible geometry — player sees void"))

    # Collision problems = stuck player
    # NOTE: The grid extends 10m beyond spawn in all directions, so many
    # "stuck" points are inside the room boundary walls — that's by design.
    # Only flag if stuck points are very high relative to the grid.
    stuck = scene.get("collision_stuck_points", 0)
    grid = scene.get("collision_grid_size", 0)
    total_grid = grid * grid if grid > 0 else 1
    stuck_pct = stuck / total_grid if total_grid > 0 else 0
    if stuck_pct > 0.3:
        score -= 1.0
        issues.append((MAJOR, f"{stuck}/{total_grid} grid points inside walls ({stuck_pct:.0%}) — dense geometry"))

    return clamp(score), issues


# ══════════════════════════════════════════════════════════════════════
# PROFILE 2: CINEMATOGRAPHER
# "I'm framing every shot. Light, color, composition, drama."
# ══════════════════════════════════════════════════════════════════════

def profile_cinematographer(scene):
    """Evaluates visual craft: composition, color theory, lighting mastery."""
    score = 5.0
    issues = []
    hero = get_hero_metrics(scene)
    dark = scene.get("dark_by_design", False)
    name = scene["name"]

    # ── Composition ──
    edge_density = hero.get("edge_density", 0)
    cvar = hero.get("color_variance", 0)
    if not dark:
        if edge_density < 3:
            if cvar > 2000:
                score -= 0.5
                issues.append((MINOR, f"Soft edges ({edge_density:.0f}%) but strong color fields — Rothko territory"))
            else:
                score -= 2.0
                issues.append((CRITICAL, f"Edge density {edge_density:.0f}% — no readable geometry at any distance"))
        elif edge_density < 8:
            score -= 0.5
            issues.append((MINOR, f"Edge density {edge_density:.0f}% — sparse visual structure"))

    # Rule of thirds
    thirds = hero.get("rule_of_thirds_energy", 0)
    if thirds > 15 and not dark:
        issues.append((NOTE, f"Strong thirds energy ({thirds:.0f}) — good classical composition"))
    elif thirds < 3 and not dark:
        score -= 0.5
        issues.append((MINOR, "Weak rule-of-thirds energy — consider reframing hero shot"))

    # ── Contrast / Drama ──
    contrast = hero.get("contrast_ratio", 1)
    if not dark:
        if contrast < 1.3:
            score -= 1.5
            issues.append((MAJOR, f"Contrast {contrast:.1f}:1 — completely flat, no visual hierarchy"))
        elif contrast > 4:
            issues.append((NOTE, f"Contrast {contrast:.1f}:1 — strong chiaroscuro"))
    else:
        if contrast > 5:
            issues.append((NOTE, "Excellent dark-scene contrast — pools of light work"))

    # ── Color ──
    hues = hero.get("hue_buckets", 0)
    warmth = hero.get("warmth", 0)
    sat = hero.get("saturation_avg", 0)

    if not dark:
        if hues < 2:
            score -= 1.5
            issues.append((MAJOR, f"Only {hues} hue — completely monotone"))
        elif hues < 4:
            score -= 0.5
            issues.append((MINOR, f"Only {hues} hues — could use accent colors"))
        elif hues >= 8:
            issues.append((NOTE, f"{hues} hues — rich color vocabulary"))

        # Godard palette check — specific accents, not random color
        if sat > 0.3:
            issues.append((NOTE, f"Saturation {sat:.2f} — bold Godard-esque color"))
        elif sat < 0.05 and not dark:
            score -= 0.5
            issues.append((MINOR, f"Saturation {sat:.2f} — desaturated to the point of grey"))

    # Scene-specific color checks
    if "space" in name and warmth > 30:
        score -= 0.5
        issues.append((MINOR, f"Warmth +{warmth:.0f} — space should read cooler"))
    if name in ("room", "hallway", "lobby") and warmth < -10:
        score -= 1.0
        issues.append((MAJOR, f"Warmth {warmth:.0f} — Hotel Chevalier needs golden tone"))

    # ── Lighting craft ──
    luma = hero.get("luma", 0)
    black_pct = hero.get("black_pct", 0)
    mid_pct = hero.get("mid_tone_pct", 0)
    if not dark:
        if luma < 10:
            score -= 2.0
            issues.append((CRITICAL, f"Luma {luma:.0f} — scene is invisible"))
        if black_pct > 60:
            score -= 1.0
            issues.append((MAJOR, f"{black_pct:.0f}% black — massive unlit regions"))
        if mid_pct < 15:
            score -= 0.5
            issues.append((MINOR, f"Only {mid_pct:.0f}% mid-tones — harsh binary lighting"))

    # ── Multi-angle consistency ──
    angles = get_all_angle_metrics(scene)
    if len(angles) >= 4:
        lumas = [a.get("luma", 0) for a in angles]
        luma_range = max(lumas) - min(lumas)
        if not dark and luma_range > 80:
            score -= 0.5
            issues.append((MINOR, f"Luma varies {min(lumas):.0f}-{max(lumas):.0f} across angles — uneven lighting"))

    return clamp(score), issues


# ══════════════════════════════════════════════════════════════════════
# PROFILE 3: ART DIRECTOR
# "Does this match the design vision? Anti-patterns? Commandments?"
# ══════════════════════════════════════════════════════════════════════

def profile_art_director(scene):
    """Evaluates against the 10 Design Commandments and anti-pattern list."""
    score = 5.0
    issues = []
    hero = get_hero_metrics(scene)
    name = scene["name"]
    dark = scene.get("dark_by_design", False)
    warmth = hero.get("warmth", 0)
    luma = hero.get("luma", 0)
    hues = hero.get("hue_buckets", 0)
    contrast = hero.get("contrast_ratio", 1)
    rgb = hero.get("avg_rgb", [0, 0, 0])

    # ── Anti-pattern: Horror vibes ──
    if warmth < -15 and luma < 40 and not dark and contrast > 3:
        score -= 2.0
        issues.append((CRITICAL, f"HORROR VIBES — cold ({warmth:.0f}) + dark (luma {luma:.0f}). Wonder, not dread."))

    # ── Anti-pattern: Grey-on-grey in space ──
    if "space" in name and hues < 3 and not dark:
        score -= 2.0
        issues.append((CRITICAL, f"GREY-ON-GREY — {hues} hues in space. Hull must pop against void."))

    # ── Anti-pattern: Yellow-tinting everything ──
    if len(rgb) == 3 and rgb[0] > 150 and rgb[1] > 130 and rgb[2] < rgb[1] - 30:
        if name not in ("room", "hallway"):
            score -= 1.0
            issues.append((MAJOR, f"YELLOW TINT — RGB [{rgb[0]:.0f},{rgb[1]:.0f},{rgb[2]:.0f}]. Neutral base, SPECIFIC accents."))

    # ── Anti-pattern: Earth glow missing ──
    if name == "balcony":
        quads = hero.get("quadrant_luma", [0, 0, 0, 0])
        if len(quads) == 4:
            bottom = (quads[2] + quads[3]) / 2
            if bottom < 5:
                score -= 2.0
                issues.append((CRITICAL, f"EARTH GLOW MISSING — bottom luma {bottom:.0f}. Earth is the emotional anchor."))

    if name == "space_lobby":
        quads = hero.get("quadrant_luma", [0, 0, 0, 0])
        if len(quads) == 4 and all(q < 8 for q in quads):
            score -= 1.5
            issues.append((MAJOR, "Earth not visible from observation window"))

    # ── Material consistency ──
    mat_types = scene.get("material_types", 0)
    mat_cov = scene.get("material_coverage", 0)
    walls = scene.get("walls", 0)
    breakdown = scene.get("material_breakdown", {})

    if walls > 20:
        if mat_types <= 2:
            score -= 1.5
            issues.append((MAJOR, f"Only {mat_types} material types — brutalist by accident"))
        if mat_cov < 30:
            score -= 1.0
            issues.append((MAJOR, f"{mat_cov:.0f}% non-concrete — tag your surfaces"))

        # Concrete dominance
        concrete = breakdown.get("concrete", 0)
        if walls > 0 and concrete > walls * 0.75:
            score -= 0.5
            issues.append((MINOR, f"{concrete}/{walls} walls raw concrete"))

    # ── 3px visibility rule ──
    edge = hero.get("edge_density", 0)
    if not dark and edge < 2 and hero.get("color_variance", 0) < 500:
        score -= 1.5
        issues.append((MAJOR, "Nothing visible at 480x300 — fails 3px rule"))

    # ── Warmth progression (space suite should feel warmer as tasks complete) ──
    if name == "space_suite":
        if warmth < -5:
            score -= 0.5
            issues.append((MINOR, f"Suite is cold (warmth {warmth:.0f}) — should feel inviting"))

    # ── Model count as richness proxy ──
    model_count = scene.get("model_count", 0)
    if not dark and walls > 30 and model_count == 0:
        score -= 0.5
        issues.append((MINOR, "No 3D model assets in scene — all procedural boxes"))

    return clamp(score), issues


# ══════════════════════════════════════════════════════════════════════
# PROFILE 4: ACCESSIBILITY
# "Can everyone experience this? Color blindness? Low vision? Contrast?"
# ══════════════════════════════════════════════════════════════════════

def profile_accessibility(scene):
    """Evaluates contrast ratios, color-blind safety, and visual clarity."""
    score = 5.0
    issues = []
    hero = get_hero_metrics(scene)
    name = scene["name"]
    dark = scene.get("dark_by_design", False)

    # ── WCAG-ish contrast ──
    # We don't have text-on-background, but we can check if important elements
    # are distinguishable from their surroundings
    brightness_std = hero.get("brightness_std", 0)
    if not dark:
        if brightness_std < 15:
            score -= 2.0
            issues.append((CRITICAL, f"Brightness stddev {brightness_std:.0f} — everything is same brightness, nothing stands out"))
        elif brightness_std < 30:
            score -= 0.5
            issues.append((MINOR, f"Low brightness variation ({brightness_std:.0f}) — subtle differences may be lost"))

    # ── Color blindness simulation ──
    # Check if scene relies solely on red-green distinction
    rgb = hero.get("avg_rgb", [0, 0, 0])
    hues = hero.get("hue_buckets", 0)
    sat = hero.get("saturation_avg", 0)
    if len(rgb) == 3 and sat > 0.15:
        # Protanopia/Deuteranopia: red and green look similar
        # Only flag when the palette is actually leaning on a red-vs-green split.
        # Warm amber / taupe scenes are not the same thing and were being misclassified.
        r, g, b = rgb
        if abs(r - g) > 45 and b < min(r, g) * 0.45 and sat > 0.2:
            score -= 0.5
            issues.append((MINOR, "Color palette relies on red-green distinction — may be invisible to ~8% of players"))

    # ── Dark scene accessibility ──
    luma = hero.get("luma", 0)
    if dark:
        if luma < 5:
            score -= 1.5
            issues.append((MAJOR, f"Luma {luma:.0f} — players on bright screens will see nothing"))
        elif luma < 15:
            score -= 0.5
            issues.append((MINOR, f"Very dark (luma {luma:.0f}) — may be invisible on non-calibrated displays"))

    # ── Quadrant uniformity (is important content only in one spot?) ──
    quads = hero.get("quadrant_luma", [0, 0, 0, 0])
    dark_quads = hero.get("dark_quadrant_count", 0)
    if not dark and dark_quads >= 3:
        score -= 1.0
        issues.append((MAJOR, f"{dark_quads}/4 quadrants are near-black — content concentrated in tiny area"))

    # ── Interactive object visibility ──
    # Objects far from center may be missed by players with tunnel vision
    obj_max_dist = scene.get("obj_max_dist", 0)
    if obj_max_dist > 15 and scene.get("interact_count", 0) > 0:
        score -= 0.5
        issues.append((MINOR, f"Objects up to {obj_max_dist:.0f}m from spawn — peripheral vision required"))

    # ── Flashing/strobe check ──
    if name == "hyperspace":
        issues.append((NOTE, "Hyperspace tunnel may cause issues for photosensitive players — consider reduced motion option"))

    # ── Dominant color flood ──
    dom_pct = hero.get("dominant_color_pct", 0)
    if dom_pct > 80 and not dark:
        score -= 1.0
        issues.append((MAJOR, f"{dom_pct:.0f}% single color — visually overwhelming, no landmarks"))

    return clamp(score), issues


# ══════════════════════════════════════════════════════════════════════
# PROFILE 5: QA ENGINEER
# "Find the bugs. Z-fighting. Geometry leaks. Budget. Performance."
# ══════════════════════════════════════════════════════════════════════

def profile_qa_engineer(scene):
    """Evaluates technical quality: geometry, collisions, budgets, regressions."""
    score = 5.0
    issues = []
    name = scene["name"]

    # ── Wall budget ──
    walls = scene.get("walls", 0)
    wall_pct = scene.get("wall_pct", 0)
    if wall_pct > 90:
        score -= 2.0
        issues.append((CRITICAL, f"Wall budget {wall_pct:.0f}% ({walls}/{2048}) — overflow imminent"))
    elif wall_pct > 75:
        score -= 1.0
        issues.append((MAJOR, f"Wall budget {wall_pct:.0f}% ({walls}/2048) — getting tight"))
    elif wall_pct > 60:
        score -= 0.5
        issues.append((MINOR, f"Wall budget {wall_pct:.0f}% — moderate usage"))

    # ── Wall overlaps (z-fighting risk) ──
    # NOTE: The engine intentionally overlaps walls for furniture composition
    # (chandelier arms, desk legs, etc). Only flag extreme counts as those
    # suggest structural walls overlapping, not just furniture pieces.
    overlaps = scene.get("wall_overlaps", 0)
    if overlaps > 200:
        score -= 1.5
        issues.append((MAJOR, f"{overlaps} wall overlaps — likely structural z-fighting"))
    elif overlaps > 100:
        score -= 0.5
        issues.append((MINOR, f"{overlaps} wall overlaps (most are intentional furniture composition)"))
    elif overlaps > 50:
        issues.append((NOTE, f"{overlaps} wall overlaps (furniture composition — expected)"))

    # ── Collision issues ──
    # Grid probes extend 10m beyond spawn — being inside boundary walls is expected.
    # Only flag if stuck-point ratio is very high (cramped interior).
    stuck = scene.get("collision_stuck_points", 0)
    grid_size = scene.get("collision_grid_size", 0)
    total_grid = grid_size * grid_size if grid_size > 0 else 1
    stuck_pct = stuck / total_grid if total_grid > 0 else 0
    if stuck_pct > 0.4:
        score -= 1.5
        issues.append((MAJOR, f"Collision: {stuck_pct:.0%} of probe grid is inside walls — very dense"))
    elif stuck_pct > 0.25:
        score -= 0.5
        issues.append((MINOR, f"Collision: {stuck_pct:.0%} inside walls — moderately dense"))

    floor_hits = scene.get("collision_floor_hits", 0)
    if grid_size > 0 and name not in FLOOR_PROBE_EXEMPT_SCENES:
        coverage = floor_hits / total_grid
        if coverage < 0.1:
            score -= 1.0
            issues.append((MAJOR, f"Floor coverage {coverage:.0%} — very little walkable area detected"))
        elif coverage < 0.2:
            score -= 0.5
            issues.append((MINOR, f"Floor coverage {coverage:.0%} — sparse floor"))

    # ── Load time ──
    load_ms = scene.get("load_time_ms", 0)
    if load_ms > 500:
        score -= 1.0
        issues.append((MAJOR, f"Load time {load_ms:.0f}ms — noticeable delay"))
    elif load_ms > 200:
        score -= 0.5
        issues.append((MINOR, f"Load time {load_ms:.0f}ms"))

    # ── Regression ──
    reg_pct = scene.get("regression_pct", 0)
    if reg_pct > 50:
        score -= 2.0
        issues.append((CRITICAL, f"REGRESSION: {reg_pct:.0f}% pixels changed vs baseline — visual breakage"))
    elif reg_pct > 25:
        score -= 1.0
        issues.append((MAJOR, f"Regression: {reg_pct:.0f}% pixels changed"))
    elif reg_pct > 10:
        score -= 0.5
        issues.append((MINOR, f"Minor regression: {reg_pct:.0f}% pixels changed"))

    # ── Anomaly: dominant color flood ──
    hero = get_hero_metrics(scene)
    dom_pct = hero.get("dominant_color_pct", 0)
    if dom_pct > 85 and not scene.get("dark_by_design", False):
        score -= 1.5
        issues.append((MAJOR, f"Dominant color {dom_pct:.0f}% — possible rogue geometry blocking view"))

    # ── Object placement sanity ──
    objects = scene.get("objects", [])
    for obj in objects:
        if obj.get("dist", 0) > 30:
            score -= 0.5
            issues.append((MINOR, f"Object '{obj.get('name', '?')}' is {obj['dist']:.0f}m from spawn"))
            break  # Only flag once

    return clamp(score), issues


# ══════════════════════════════════════════════════════════════════════
# PROFILE 6: SPEEDRUNNER
# "How fast can I move through this? Where will I get stuck?"
# ══════════════════════════════════════════════════════════════════════

def profile_speedrunner(scene):
    """Evaluates flow, movement clarity, and potential stuck points."""
    score = 5.0
    issues = []
    name = scene["name"]

    # ── Exit clarity ──
    has_exit = scene.get("has_exit", False)
    transitional = name in ("hyperspace", "taxi", "return_taxi", "elevator", "stars", "bed")
    if not has_exit and not transitional:
        score -= 0.5
        issues.append((MINOR, "No exit marker — speedrunner has to guess where to go"))

    # ── Spawn to exit distance ──
    if has_exit:
        spawn = scene.get("spawn", [0, 0, 0])
        exit_p = scene.get("exit_pos", [0, 0, 0])
        dx = exit_p[0] - spawn[0]
        dz = exit_p[2] - spawn[2]
        dist = math.sqrt(dx*dx + dz*dz)
        if dist < 2:
            issues.append((NOTE, f"Exit very close to spawn ({dist:.1f}m) — fast transit"))
        elif dist > 20:
            score -= 0.5
            issues.append((MINOR, f"Exit is {dist:.0f}m from spawn — long traverse"))

    # ── Stuck potential (collision) ──
    # Grid extends beyond room, so inside-wall points are expected at edges
    stuck = scene.get("collision_stuck_points", 0)
    grid = scene.get("collision_grid_size", 0)
    total_grid = grid * grid if grid > 0 else 1
    stuck_pct = stuck / total_grid if total_grid > 0 else 0
    if stuck_pct > 0.3:
        score -= 1.0
        issues.append((MAJOR, f"Dense collision geometry ({stuck_pct:.0%} stuck) — movement may be tight"))

    # ── Load time ──
    load_ms = scene.get("load_time_ms", 0)
    if load_ms > 100:
        score -= 0.5
        issues.append((MINOR, f"Load time {load_ms:.0f}ms — adds up over multiple runs"))

    # ── Scene complexity (more walls = more collision checks = slower physics) ──
    collidable_walls = scene.get("collidable_wall_count")
    if collidable_walls is None:
        collidable_walls = scene.get("walls", 0) - scene.get("no_collide_count", 0)
    if collidable_walls > 500:
        score -= 0.5
        issues.append((MINOR, f"{collidable_walls} collidable walls — dense collision geometry may slow physics"))

    # ── Multi-step objects (take time) ──
    objects = scene.get("objects", [])
    total_steps = sum(obj.get("max_steps", 1) for obj in objects if obj.get("max_steps", 1) > 1)
    if total_steps > 6:
        score -= 0.5
        issues.append((MINOR, f"{total_steps} total interaction steps — forces slow play"))

    # ── Hinge doors (potential blockers) ──
    hinges = scene.get("hinge_count", 0)
    if hinges > 2:
        issues.append((NOTE, f"{hinges} hinge doors — potential pathing blocks on tight runs"))

    return clamp(score), issues


# ══════════════════════════════════════════════════════════════════════
# ANALYSIS ENGINE
# ══════════════════════════════════════════════════════════════════════

PROFILES = {
    "first_timer": {
        "name": "First-Timer",
        "icon": "👤",
        "weight": 1.3,
        "desc": "Navigation, discoverability, confusion",
        "fn": profile_first_timer,
    },
    "cinematographer": {
        "name": "Cinematographer",
        "icon": "🎬",
        "weight": 1.2,
        "desc": "Composition, color theory, lighting craft",
        "fn": profile_cinematographer,
    },
    "art_director": {
        "name": "Art Director",
        "icon": "🎨",
        "weight": 1.2,
        "desc": "Design commandments, anti-patterns, materials",
        "fn": profile_art_director,
    },
    "accessibility": {
        "name": "Accessibility",
        "icon": "♿",
        "weight": 1.0,
        "desc": "Contrast, color-blindness, visual clarity",
        "fn": profile_accessibility,
    },
    "qa_engineer": {
        "name": "QA Engineer",
        "icon": "🔧",
        "weight": 1.1,
        "desc": "Z-fighting, geometry, budgets, regressions",
        "fn": profile_qa_engineer,
    },
    "speedrunner": {
        "name": "Speedrunner",
        "icon": "⚡",
        "weight": 0.8,
        "desc": "Flow, transitions, stuck points",
        "fn": profile_speedrunner,
    },
}


def analyze_e2e(report_path):
    with open(report_path) as f:
        report = json.load(f)

    scenes = report.get("scenes", [])
    mode = report.get("mode", "standard")
    is_e2e = mode == "e2e"

    # Results storage
    all_results = {}  # {scene_name: {profile_name: (score, issues)}}
    profile_totals = defaultdict(lambda: {"sum": 0, "count": 0})
    scene_totals = {}

    # ── Score every scene from every profile ──
    for scene in scenes:
        name = scene["name"]
        all_results[name] = {}
        scene_sum = 0

        for pid, profile in PROFILES.items():
            score, issues = profile["fn"](scene)
            all_results[name][pid] = (score, issues)
            profile_totals[pid]["sum"] += score
            profile_totals[pid]["count"] += 1
            scene_sum += score * profile["weight"]

        total_weight = sum(p["weight"] for p in PROFILES.values())
        scene_totals[name] = scene_sum / total_weight

    # ══════════════════════════════════════════════════════════════
    # CONSOLE REPORT
    # ══════════════════════════════════════════════════════════════

    print()
    print("=" * 80)
    print("  ENDEARING VOID — E2E QA ANALYSIS")
    print(f"  Mode: {mode} | Scenes: {len(scenes)} | Profiles: {len(PROFILES)}")
    print("  \"Three hours to kill.\"")
    print("=" * 80)

    # ── Per-scene breakdown ──
    for scene in scenes:
        name = scene["name"]
        avg = scene_totals[name]
        grade = grade_letter(avg)

        # Scene header
        print()
        bar = "█" * int(avg * 4) + "░" * (20 - int(avg * 4))
        print(f"┌─ {name.upper()} {'─' * (58 - len(name))} {grade} ({avg:.1f}/5)")
        print(f"│  {bar}  {avg/5*100:.0f}%")

        # Per-profile scores
        for pid, profile in PROFILES.items():
            score, issues = all_results[name][pid]
            stars = "★" * score + "☆" * (5 - score)
            print(f"│  {profile['icon']} {profile['name'][:14]:14s} {stars}", end="")

            # Show worst issue
            criticals = [i for i in issues if i[0] == CRITICAL]
            majors = [i for i in issues if i[0] == MAJOR]
            if criticals:
                print(f"  !! {criticals[0][1]}")
            elif majors:
                print(f"   ! {majors[0][1]}")
            elif issues:
                print(f"     {issues[0][1]}")
            else:
                print("     —")

            # Show additional critical/major issues
            all_important = criticals[1:] + majors[1:]
            for severity, msg in all_important[:2]:
                prefix = "!!" if severity == CRITICAL else " !"
                print(f"│  {'':17s}          {prefix} {msg}")

        print(f"└{'─' * 79}")

    # ── Profile averages ──
    print()
    print("=" * 80)
    print("  PROFILE AVERAGES")
    print("=" * 80)
    print()

    worst_profile = None
    worst_avg = 6
    for pid in PROFILES:
        t = profile_totals[pid]
        avg = t["sum"] / t["count"] if t["count"] > 0 else 0
        grade = grade_letter(avg)
        profile = PROFILES[pid]
        stars = "★" * round(avg) + "☆" * (5 - round(avg))
        print(f"  {profile['icon']} {profile['name']:16s}  {stars}  {avg:.1f}/5  ({grade})  — {profile['desc']}")
        if avg < worst_avg:
            worst_avg = avg
            worst_profile = pid

    # ── Overall composite ──
    total_weighted = sum(scene_totals.values())
    overall_avg = total_weighted / len(scenes) if scenes else 0
    overall_grade = grade_letter(overall_avg)

    print()
    print("=" * 80)
    print(f"  OVERALL: {overall_grade}  ({overall_avg:.1f}/5 = {overall_avg/5*100:.0f}%)")
    print("=" * 80)

    # ── Cross-profile consensus: issues flagged by 3+ profiles ──
    print()
    print("  CONSENSUS ISSUES (flagged by 3+ profiles):")
    consensus_count = 0
    for scene in scenes:
        name = scene["name"]
        critical_count = 0
        major_count = 0
        for pid in PROFILES:
            score, issues = all_results[name][pid]
            critical_count += len([i for i in issues if i[0] == CRITICAL])
            major_count += len([i for i in issues if i[0] == MAJOR])
        profiles_with_issues = sum(1 for pid in PROFILES if all_results[name][pid][0] < 4)
        if profiles_with_issues >= 3:
            print(f"    [{name.upper()}] {profiles_with_issues}/6 profiles flagged issues "
                  f"({critical_count} critical, {major_count} major)")
            consensus_count += 1
    if consensus_count == 0:
        print("    (none — scenes are in good shape)")

    # ── Urgent fixes ──
    print()
    print("  URGENT FIXES (critical issues):")
    urgent_count = 0
    for scene in scenes:
        name = scene["name"]
        for pid in PROFILES:
            score, issues = all_results[name][pid]
            for severity, msg in issues:
                if severity == CRITICAL:
                    print(f"    [{name.upper()}] ({PROFILES[pid]['name']}) {msg}")
                    urgent_count += 1
    if urgent_count == 0:
        print("    (none — nice work)")

    # ── Weakest profile ──
    print()
    if worst_profile:
        print(f"  WEAKEST PROFILE: {PROFILES[worst_profile]['icon']} "
              f"{PROFILES[worst_profile]['name'].upper()} ({worst_avg:.1f}/5)")
        print(f"  Focus your next iteration here.")

    # ── Worst scenes ──
    sorted_scenes = sorted(scene_totals.items(), key=lambda x: x[1])
    if sorted_scenes:
        print()
        print(f"  WORST SCENE: {sorted_scenes[0][0].upper()} ({sorted_scenes[0][1]:.1f}/5)")
        if len(sorted_scenes) > 1 and sorted_scenes[1][1] < 4:
            print(f"  RUNNER-UP:   {sorted_scenes[1][0].upper()} ({sorted_scenes[1][1]:.1f}/5)")

    # ── Flow test results ──
    transitions = report.get("transitions", [])
    if transitions:
        print()
        print("  FLOW TRANSITIONS:")
        failed = [t for t in transitions if not t.get("ok", True)]
        slow = [t for t in transitions if t.get("to_ms", 0) > 200]
        if failed:
            for t in failed:
                print(f"    FAIL: {t['from']} -> {t['to']}")
        if slow:
            for t in slow:
                print(f"    SLOW: {t['from']} -> {t['to']} ({t['to_ms']:.0f}ms)")
        if not failed and not slow:
            total_ms = sum(t.get("from_ms", 0) + t.get("to_ms", 0) for t in transitions)
            print(f"    All {len(transitions)} transitions OK ({total_ms:.0f}ms total)")

    print()

    # ══════════════════════════════════════════════════════════════
    # JSON OUTPUT
    # ══════════════════════════════════════════════════════════════

    grades = {
        "mode": mode,
        "overall": {
            "grade": overall_grade,
            "score": round(overall_avg, 2),
            "pct": round(overall_avg / 5 * 100),
        },
        "profiles": {},
        "scenes": {},
    }

    for pid in PROFILES:
        t = profile_totals[pid]
        avg = t["sum"] / t["count"] if t["count"] > 0 else 0
        grades["profiles"][pid] = {
            "name": PROFILES[pid]["name"],
            "avg": round(avg, 1),
            "grade": grade_letter(avg),
            "weight": PROFILES[pid]["weight"],
        }

    for scene in scenes:
        name = scene["name"]
        scene_grade = {
            "grade": grade_letter(scene_totals[name]),
            "composite": round(scene_totals[name], 2),
        }
        for pid in PROFILES:
            score, issues = all_results[name][pid]
            scene_grade[pid] = {
                "score": score,
                "issues": [{"severity": s, "message": m} for s, m in issues],
            }
        grades["scenes"][name] = scene_grade

    # Backward compat: also output pillar-style data
    # Map profiles to old pillar names
    pillar_map = {
        "composition": "cinematographer",
        "lighting": "cinematographer",
        "materials": "art_director",
        "color": "cinematographer",
        "interaction": "first_timer",
        "antipatterns": "art_director",
    }
    grades["pillars"] = {}
    for pillar, pid in pillar_map.items():
        t = profile_totals[pid]
        avg = t["sum"] / t["count"] if t["count"] > 0 else 0
        grades["pillars"][pillar] = {"avg": round(avg, 1), "grade": grade_letter(avg)}

    grades_path = os.path.join(os.path.dirname(report_path), "e2e_grades.json")
    with open(grades_path, "w") as f:
        json.dump(grades, f, indent=2)

    # Also write backward-compat grades.json
    compat_grades = {
        "overall": grades["overall"],
        "pillars": grades["pillars"],
        "scenes": {},
    }
    for name, sg in grades["scenes"].items():
        compat_grades["scenes"][name] = {
            "grade": sg["grade"],
            "total": round(sg["composite"] * 6),
            "composition": sg.get("cinematographer", {}).get("score", 3),
            "lighting": sg.get("cinematographer", {}).get("score", 3),
            "materials": sg.get("art_director", {}).get("score", 3),
            "color": sg.get("cinematographer", {}).get("score", 3),
            "interaction": sg.get("first_timer", {}).get("score", 3),
            "antipatterns": sg.get("art_director", {}).get("score", 3),
        }
    compat_path = os.path.join(os.path.dirname(report_path), "grades.json")
    with open(compat_path, "w") as f:
        json.dump(compat_grades, f, indent=2)

    return 0 if overall_avg >= 3.5 else 1


if __name__ == "__main__":
    path = sys.argv[1] if len(sys.argv) > 1 else "qa/e2e_report.json"
    if not os.path.exists(path):
        # Fallback to standard report
        path = "qa/report.json"
    if not os.path.exists(path):
        print("No report found. Run 'make qa-e2e' or 'make qa' first.")
        sys.exit(1)
    sys.exit(analyze_e2e(path))
