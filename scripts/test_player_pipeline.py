from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from PIL import Image, ImageDraw

from scripts.build_player_atlas import build_player_atlas
from scripts.make_reference_canvas import build_reference_canvas
from scripts.normalize_strip import normalize_strip


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


if __name__ == "__main__":
    unittest.main()
