from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageDraw

from scripts.build_player_atlas import build_player_atlas
from scripts.build_all_sprite_prompts import PROMPTS
from scripts.build_sprite_prompt import build_prompt
from scripts.make_reference_canvas import build_reference_canvas
from scripts.audit_player_animation import summarize_animation, diagnose_animation, strict_violations
from scripts.cleanup_player_frames import align_to_center, fill_small_holes, remove_small_components
from scripts.normalize_strip import normalize_strip
from scripts.validate_slice_content import validate_player_assets, validate_player_prompts


class PlayerPipelineTests(unittest.TestCase):
    def test_reference_canvas_places_seed_in_leftmost_slot(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            seed_path = temp_path / "seed.png"
            output_path = temp_path / "canvas.png"

            seed = Image.new("RGBA", (4, 4), (0, 0, 0, 0))
            ImageDraw.Draw(seed).rectangle((0, 0, 3, 3), fill=(255, 0, 0, 255))
            seed.save(seed_path)

            build_reference_canvas(seed_path, output_path, canvas_size=64, slot_count=4, slot_size=16)

            canvas = Image.open(output_path).convert("RGBA")
            self.assertEqual(canvas.size, (64, 64))
            self.assertIsNotNone(canvas.getchannel("A").getbbox())

    def test_normalize_strip_preserves_shared_scale(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            strip_path = temp_path / "strip.png"
            output_dir = temp_path / "out"

            strip = Image.new("RGBA", (64, 32), (0, 0, 0, 0))
            draw = ImageDraw.Draw(strip)
            draw.rectangle((8, 5, 18, 25), fill=(255, 255, 255, 255))
            draw.rectangle((40, 13, 50, 23), fill=(255, 255, 255, 255))
            strip.save(strip_path)

            frames = normalize_strip(strip_path, output_dir, slot_count=2, slot_size=16, frame_size=16, padding=2)
            self.assertEqual(len(frames), 2)

            with Image.open(frames[0]) as first_frame:
                first_bbox = first_frame.getchannel("A").getbbox()
                self.assertEqual(first_frame.size, (16, 16))

            with Image.open(frames[1]) as second_frame:
                second_bbox = second_frame.getchannel("A").getbbox()
                self.assertEqual(second_frame.size, (16, 16))
            self.assertGreater(first_bbox[3] - first_bbox[1], second_bbox[3] - second_bbox[1])

    def test_lock_frame_01_uses_exact_seed(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            strip_path = temp_path / "strip.png"
            seed_path = temp_path / "seed.png"
            output_dir = temp_path / "out"

            seed = Image.new("RGBA", (16, 16), (10, 20, 30, 255))
            seed.save(seed_path)

            strip = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
            ImageDraw.Draw(strip).rectangle((2, 2, 10, 14), fill=(255, 255, 255, 255))
            ImageDraw.Draw(strip).rectangle((18, 2, 28, 14), fill=(255, 255, 255, 255))
            strip.save(strip_path)

            normalize_strip(
                strip_path,
                output_dir,
                slot_count=2,
                slot_size=16,
                frame_size=16,
                padding=2,
                seed_path=seed_path,
                lock_frame_01=True,
            )

            self.assertEqual((output_dir / "frame-01.png").read_bytes(), seed_path.read_bytes())

    def test_normalize_strip_uses_seed_anchor_by_default(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            strip_path = temp_path / "strip.png"
            seed_path = temp_path / "seed.png"
            output_dir = temp_path / "out"

            seed = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
            ImageDraw.Draw(seed).rectangle((1, 4, 6, 13), fill=(255, 0, 0, 255))
            seed.save(seed_path)

            strip = Image.new("RGBA", (32, 16), (0, 0, 0, 0))
            draw = ImageDraw.Draw(strip)
            draw.rectangle((2, 1, 11, 15), fill=(255, 255, 255, 255))
            draw.rectangle((19, 3, 26, 15), fill=(255, 255, 255, 255))
            strip.save(strip_path)

            frames = normalize_strip(
                strip_path,
                output_dir,
                slot_count=2,
                slot_size=16,
                frame_size=16,
                padding=1,
                seed_path=seed_path,
            )

            with Image.open(frames[1]) as frame:
                bbox = frame.getchannel("A").getbbox()

            self.assertEqual(bbox[0], 1)
            self.assertEqual(bbox[3], 14)

    def test_build_player_atlas_packs_frames_and_emits_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            assets_root = temp_path / "assets" / "player"
            (assets_root / "idle").mkdir(parents=True)
            (assets_root / "run").mkdir(parents=True)

            for animation, count in {"idle": 2, "run": 2}.items():
                for index in range(1, count + 1):
                    frame = Image.new("RGBA", (16, 16), (index * 40, 0, 0, 255))
                    frame.save(assets_root / animation / f"frame-{index:02d}.png")

            source_manifest = {
                "frame_size": 16,
                "animation_order": ["idle", "run"],
                "animations": {
                    "idle": {"fps": 4, "loop": True, "frames": ["frame-01.png", "frame-02.png"]},
                    "run": {"fps": 8, "loop": True, "frames": ["frame-01.png", "frame-02.png"]},
                },
            }
            source_manifest_path = assets_root / "player_manifest.json"
            source_manifest_path.write_text(json.dumps(source_manifest), encoding="utf-8")

            atlas_path = assets_root / "player_atlas.png"
            runtime_manifest_path = assets_root / "player_manifest.lua"
            runtime_manifest = build_player_atlas(source_manifest_path, atlas_path, runtime_manifest_path)

            self.assertTrue(atlas_path.exists())
            self.assertTrue(runtime_manifest_path.exists())
            self.assertEqual(runtime_manifest["frame_size"], 16)
            self.assertEqual(len(runtime_manifest["animations"]["idle"]["frames"]), 2)

    def test_build_player_atlas_rejects_missing_frame(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            assets_root = temp_path / "assets" / "player"
            (assets_root / "idle").mkdir(parents=True)
            Image.new("RGBA", (16, 16), (255, 0, 0, 255)).save(assets_root / "idle" / "frame-01.png")

            source_manifest = {
                "frame_size": 16,
                "animation_order": ["idle"],
                "animations": {
                    "idle": {"fps": 4, "loop": True, "frames": ["frame-01.png", "frame-02.png"]},
                },
            }
            source_manifest_path = assets_root / "player_manifest.json"
            source_manifest_path.write_text(json.dumps(source_manifest), encoding="utf-8")

            with self.assertRaises(ValueError):
                build_player_atlas(
                    source_manifest_path,
                    assets_root / "player_atlas.png",
                    assets_root / "player_manifest.lua",
                )

    def test_build_prompt_mentions_seed_slot_and_action(self) -> None:
        prompt = build_prompt(
            animation_name="hurt",
            slot_count=4,
            slot_size=256,
            canvas_size=1024,
            seed_slot=0,
            facing="right-facing",
            action="Frames 2 through 4 show a short hurt recoil, then a slight recovery.",
            constraints="no weapon, no scenery",
        )

        self.assertIn("leftmost slot", prompt)
        self.assertIn("Frames 2 through 4 show a short hurt recoil", prompt)
        self.assertIn("no weapon, no scenery", prompt)

    def test_validate_player_assets_requires_all_states(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            temp_path = Path(temp_dir)
            assets_root = temp_path / "assets" / "player"
            (assets_root / "idle").mkdir(parents=True)
            Image.new("RGBA", (16, 16), (255, 0, 0, 255)).save(assets_root / "idle" / "frame-01.png")

            manifest = {
                "frame_size": 16,
                "animations": {
                    "idle": {
                        "fps": 4,
                        "loop": True,
                        "frames": ["frame-01.png"],
                    }
                },
            }
            errors = validate_player_assets(manifest, assets_root / "player_manifest.json")
            self.assertTrue(any("Missing required player states" in error for error in errors))

    def test_validate_player_prompts_checks_full_set(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            prompts_dir = Path(temp_dir)
            (prompts_dir / "idle_edit_prompt.txt").write_text("idle", encoding="utf-8")
            errors = validate_player_prompts(prompts_dir)
            self.assertEqual(len(errors), (len(PROMPTS) - 1) + 5)
            self.assertTrue(any("shoot_compact_edit_prompt.txt" in error for error in errors))

    def test_animation_audit_flags_large_drift(self) -> None:
        summary = summarize_animation(
            [
                {"center_drift": -1, "foot_drift": 0, "width": 12, "height": 24, "components": 1, "small_components": 0, "holes": 0, "hole_pixels": 0},
                {"center_drift": 5, "foot_drift": 3, "width": 17, "height": 31, "components": 2, "small_components": 1, "holes": 2, "hole_pixels": 3},
            ]
        )
        findings = diagnose_animation(summary)
        self.assertTrue(any("foot drift range" in finding for finding in findings))
        self.assertTrue(any("center drift range" in finding for finding in findings))

    def test_cleanup_fills_single_pixel_hole_and_removes_small_island(self) -> None:
        image = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        draw.rectangle((1, 1, 5, 5), fill=(255, 0, 0, 255))
        image.putpixel((3, 3), (0, 0, 0, 0))
        image.putpixel((7, 7), (255, 0, 0, 255))

        filled = fill_small_holes(image, max_hole_pixels=1)
        removed = remove_small_components(image, max_component_pixels=1)

        self.assertEqual(filled, 1)
        self.assertEqual(removed, 1)
        self.assertEqual(image.getpixel((3, 3))[3], 255)
        self.assertEqual(image.getpixel((7, 7))[3], 0)

    def test_align_to_center_recenters_visible_bounds(self) -> None:
        image = Image.new("RGBA", (8, 8), (0, 0, 0, 0))
        draw = ImageDraw.Draw(image)
        draw.rectangle((0, 2, 2, 5), fill=(255, 0, 0, 255))

        shift = align_to_center(image, center_x=4)

        self.assertGreater(shift, 0)
        bbox = image.getchannel("A").getbbox()
        self.assertIsNotNone(bbox)
        self.assertLessEqual(abs((bbox[0] + ((bbox[2] - bbox[0]) / 2)) - 4), 0.5)

    def test_strict_violations_require_idle_seed_identity(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            idle_dir = root / "idle"
            hurt_dir = root / "hurt"
            idle_dir.mkdir()
            hurt_dir.mkdir()
            Image.new("RGBA", (4, 4), (255, 0, 0, 255)).save(idle_dir / "frame-01.png")
            Image.new("RGBA", (4, 4), (0, 255, 0, 255)).save(hurt_dir / "frame-01.png")

            violations = strict_violations(
                "hurt",
                {
                    "frames": [
                        {
                            "source": str(hurt_dir / "frame-01.png"),
                            "foot_drift": 0,
                            "width": 20,
                            "height": 20,
                            "components": 1,
                            "holes": 0,
                        }
                    ],
                    "summary": {
                        "foot_drift_range": 0,
                        "center_drift_range": 0,
                        "height_range": 0,
                        "max_width": 20,
                        "max_components": 1,
                        "max_holes": 0,
                        "max_hole_pixels": 0,
                    },
                },
                root,
            )

            self.assertTrue(any("byte-identical" in violation for violation in violations))


if __name__ == "__main__":
    unittest.main()
