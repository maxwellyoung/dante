#!/usr/bin/env bash
# glb_qa.sh — Visual QA for GLB models using Three.js viewer + Playwright
#
# Usage: ./scripts/glb_qa.sh assets/bathtub.glb
#
# Serves the viewer locally, auto-loads the GLB, captures 4 screenshots
# via Playwright headless browser, saves to qa/models/
#
# Requires: npx playwright, python3

set -euo pipefail

GLB_FILE="${1:?Usage: $0 <path-to-glb>}"
MODEL_NAME="$(basename "$GLB_FILE" .glb)"
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
QA_DIR="$ENGINE_DIR/qa/models"
VIEWER_DIR="/Users/maxwellyoung/Development/glb-viewer"
PORT=8787

mkdir -p "$QA_DIR"

# Copy GLB to viewer dir
cp "$ENGINE_DIR/$GLB_FILE" "$VIEWER_DIR/_qa_model.glb"

# Auto-loading viewer page
cat > "$VIEWER_DIR/_qa_loader.html" << 'EOF'
<!DOCTYPE html>
<html><head>
<style>* { margin: 0; } body { background: #111; } canvas { display: block; }</style>
</head><body>
<div id="status" style="position:fixed;top:10px;left:10px;color:#fff;font:12px monospace;z-index:99">Loading...</div>
<script type="importmap">
{ "imports": {
    "three": "https://cdn.jsdelivr.net/npm/three@0.170.0/build/three.module.js",
    "three/addons/": "https://cdn.jsdelivr.net/npm/three@0.170.0/examples/jsm/"
} }
</script>
<script type="module">
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

const scene = new THREE.Scene();
scene.background = new THREE.Color(0x111111);
const camera = new THREE.PerspectiveCamera(40, 1, 0.01, 100);
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(1200, 1200);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.3;
document.body.appendChild(renderer.domElement);

const controls = new OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.autoRotate = true;
controls.autoRotateSpeed = 12.0;

scene.add(new THREE.AmbientLight(0xffffff, 0.5));
const key = new THREE.DirectionalLight(0xffeedd, 1.2);
key.position.set(3, 5, 4); scene.add(key);
const fill = new THREE.DirectionalLight(0xddeeff, 0.4);
fill.position.set(-3, 2, -4); scene.add(fill);

const loader = new GLTFLoader();
loader.load('./_qa_model.glb', (gltf) => {
  const model = gltf.scene;
  scene.add(model);
  const box = new THREE.Box3().setFromObject(model);
  const center = box.getCenter(new THREE.Vector3());
  const size = box.getSize(new THREE.Vector3());
  const maxDim = Math.max(size.x, size.y, size.z);
  model.position.sub(center);
  camera.position.set(maxDim*1.5, maxDim*0.8, maxDim*1.5);
  controls.target.set(0, 0, 0);
  controls.update();
  // Count tris
  let tris = 0;
  model.traverse(c => { if (c.isMesh && c.geometry) tris += (c.geometry.index ? c.geometry.index.count/3 : c.geometry.attributes.position.count/3); });
  document.getElementById('status').textContent = `READY ${Math.round(tris)} tris`;
  document.title = 'READY';
});

function animate() {
  requestAnimationFrame(animate);
  controls.update();
  renderer.render(scene, camera);
}
animate();
</script>
</body></html>
EOF

# Start server
python3 -m http.server "$PORT" -d "$VIEWER_DIR" &>/dev/null &
SERVER_PID=$!
sleep 1

echo "═══ GLB Visual QA: $MODEL_NAME ═══"

# Playwright screenshot script
cat > "$ENGINE_DIR/scripts/_glb_qa_pw.js" << PWEOF
const { chromium } = require('playwright');
(async () => {
  const browser = await chromium.launch({
    args: ['--use-gl=angle', '--use-angle=swiftshader'],
  });
  const page = await browser.newPage({ viewport: { width: 1200, height: 1200 } });
  await page.goto('http://localhost:${PORT}/_qa_loader.html');
  // Wait for model to load
  try {
    await page.waitForFunction(() => document.title === 'READY', { timeout: 20000 });
  } catch (e) {
    // Dump console errors for debugging
    const logs = [];
    page.on('console', m => logs.push(m.text()));
    page.on('pageerror', e => logs.push('ERROR: ' + e.message));
    await page.waitForTimeout(2000);
    console.log('Page did not reach READY state. Logs:', logs.join('\\n'));
    // Take screenshot anyway to see what's there
    await page.screenshot({ path: '${QA_DIR}/${MODEL_NAME}_qa_debug.png' });
    console.log('Debug screenshot saved');
  }
  await page.waitForTimeout(500);
  // Get stats
  const stats = await page.textContent('#status');
  console.log('Stats: ' + stats);
  // Take 4 screenshots as model auto-rotates
  for (let i = 1; i <= 4; i++) {
    const path = '${QA_DIR}/${MODEL_NAME}_qa_' + i + '.png';
    await page.screenshot({ path, omitBackground: false });
    console.log('  ✓ ${MODEL_NAME}_qa_' + i + '.png');
    await page.waitForTimeout(1500); // rotate between shots
  }
  await browser.close();
})();
PWEOF

npx playwright test --version >/dev/null 2>&1  # ensure installed
node -e "require('playwright')" 2>/dev/null || npm install --no-save playwright 2>/dev/null

cd "$ENGINE_DIR" && node scripts/_glb_qa_pw.js 2>&1

# Cleanup
kill "$SERVER_PID" 2>/dev/null || true
rm -f "$VIEWER_DIR/_qa_model.glb" "$VIEWER_DIR/_qa_loader.html" "$ENGINE_DIR/scripts/_glb_qa_pw.js"

echo ""
echo "═══ Done: qa/models/${MODEL_NAME}_qa_*.png ═══"
