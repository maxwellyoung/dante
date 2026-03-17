// scale.h — Canonical dimensions for 480x300 visibility
// Everything is tuned so the SMALLEST element renders at >=3 pixels.
// At 480x300, ~1m of world space = 4-8 pixels at typical viewing distance.
// Minimum visible element: 0.25m (about 1-2px). Comfortably readable: 0.4m+.
//
// Rule: if a human wouldn't notice it from 3 meters away, don't model it.
// Legs, trim, handles — all scaled UP to read as silhouettes.

#ifndef EV_SCALE_H
#define EV_SCALE_H

// ============================================================
// PLAYER
// ============================================================
#define S_EYE_HEIGHT     1.6f    // camera Y when standing
#define S_PLAYER_RADIUS  0.45f   // collision capsule

// ============================================================
// ARCHITECTURE — rooms, walls, doors
// ============================================================
#define S_WALL_THICK     0.3f    // visible wall thickness (was 0.2-0.3)
#define S_DOOR_W         1.2f    // door opening width
#define S_DOOR_H         2.6f    // door opening height
#define S_DOOR_FRAME     0.14f   // frame thickness — bold enough to read
#define S_CEILING_STD    3.8f    // standard ceiling (Paris room)
#define S_CEILING_GRAND  7.0f    // double-height (lobby, space suite)
#define S_BASEBOARD_H    0.15f   // taller than real — must read at 480x300
#define S_CROWN_H        0.12f   // crown molding height
#define S_TRIM_DEPTH     0.08f   // protruding trim depth

// ============================================================
// FURNITURE — all dimensions tuned for 480x300 silhouettes
// Principle: ~1.5x realistic scale on thin elements
// ============================================================

// Tables
#define S_TABLE_H        0.78f   // dining/desk surface height
#define S_TABLE_TOP_T    0.08f   // tabletop thickness (was 0.04-0.06, invisible)
#define S_TABLE_LEG_D    0.12f   // leg diameter (was 0.06, invisible)
#define S_TABLE_LEG_INS  0.12f   // leg inset from edge

// Chairs
#define S_CHAIR_SEAT_H   0.48f   // seat height
#define S_CHAIR_SEAT_T   0.08f   // seat thickness (was 0.04)
#define S_CHAIR_SEAT_W   0.50f   // seat width (was 0.4)
#define S_CHAIR_SEAT_D   0.50f   // seat depth (was 0.4)
#define S_CHAIR_BACK_H   0.55f   // backrest height
#define S_CHAIR_BACK_T   0.08f   // backrest thickness (was 0.04)
#define S_CHAIR_LEG_D    0.08f   // leg diameter (was 0.03!)
#define S_CHAIR_LEG_INS  0.18f   // leg inset from center

// Desks
#define S_DESK_H         0.78f   // surface height
#define S_DESK_W         1.4f    // width (was 1.2)
#define S_DESK_D         0.7f    // depth (was 0.6)
#define S_DESK_TOP_T     0.08f   // top thickness (was 0.04)
#define S_DESK_PANEL_T   0.08f   // side panel thickness (was 0.03)

// Sofas
#define S_SOFA_W         2.0f    // total width (was 1.8)
#define S_SOFA_D         0.8f    // seat depth (was 0.7)
#define S_SOFA_SEAT_H    0.40f   // seat height (was 0.38)
#define S_SOFA_SEAT_T    0.18f   // cushion thickness (was 0.15)
#define S_SOFA_BACK_H    0.50f   // backrest height (was 0.45)
#define S_SOFA_BACK_T    0.20f   // backrest thickness (was 0.15)
#define S_SOFA_ARM_W     0.18f   // armrest width (was 0.1!)
#define S_SOFA_ARM_H     0.40f   // armrest height (was 0.35)

// Beds
#define S_BED_FRAME_H    0.25f   // frame height
#define S_BED_MATTRESS_T 0.30f   // mattress thickness (was 0.25)
#define S_BED_HEADBOARD_H 1.2f   // headboard total height

// Bars / Counters
#define S_BAR_H          1.1f    // counter height
#define S_BAR_D          0.7f    // counter depth (was 0.6)
#define S_BAR_TOP_T      0.10f   // counter top thickness (was 0.06)

// Fireplaces
#define S_FIRE_MANTEL_H  1.3f    // mantel height
#define S_FIRE_W         1.8f    // total width (was 1.6)
#define S_FIRE_PILLAR_W  0.25f   // pillar width (was 0.2)

// Rugs
#define S_RUG_T          0.03f   // rug thickness (raised from floor)
#define S_RUG_BORDER     0.15f   // border width (was 0.08!)

// Chandeliers
#define S_CHAN_ROD_D      0.08f   // central rod diameter (was 0.04)
#define S_CHAN_ARM_T      0.06f   // arm thickness (was 0.03)
#define S_CHAN_GLOBE_D    0.20f   // light globe diameter (was 0.12)

// Columns
#define S_COL_CAP_MULT   1.4f    // capital width = radius * this (was 1.25)
#define S_COL_BASE_MULT  1.3f    // base width = radius * this (was 1.1)
#define S_COL_CAP_H      0.25f   // capital height (was 0.2)
#define S_COL_BASE_H     0.25f   // base height (was 0.2)

// ============================================================
// PROPS — minimum size for 480x300 readability
// ============================================================
#define S_PROP_MIN       0.20f   // absolute minimum visible dimension
#define S_PROP_BOOK      0.35f   // book width (2x real)
#define S_PROP_CUP       0.20f   // cup diameter (2x real)
#define S_PROP_PHONE     0.25f   // phone screen (2x real)
#define S_PROP_BOTTLE    0.14f   // bottle diameter
#define S_PROP_GLASS_D   0.10f   // wine glass stem diameter
#define S_PROP_GLASS_H   0.25f   // wine glass total height

// ============================================================
// WAINSCOTING / TRIM
// ============================================================
#define S_WAINSCOT_D     0.08f   // panel depth from wall (was 0.04)
#define S_WAINSCOT_RAIL  0.06f   // rail thickness (was 0.03-0.04)

#endif
