#!/usr/bin/env bash
# glb_qa.sh — Automated visual QA for GLB models
#
# Usage: ./scripts/glb_qa.sh assets/bathtub.glb
#        ./scripts/glb_qa.sh assets/*.glb          # batch all models
#
# Serves a dedicated QA viewer (Three.js + ACES + shadows), loads each
# model at 4 fixed camera angles (front, side, quarter, top), captures
# screenshots via Playwright headless Chromium with SwiftShader WebGL.
#
# Output: qa/models/<name>_qa_{front,side,quarter,top}.png
#
# Requires: node, playwright (npm install --save-dev playwright)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ENGINE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
QA_DIR="$ENGINE_DIR/qa/models"
QA_VIEWER="$SCRIPT_DIR/qa_viewer.html"
SERVE_DIR="$ENGINE_DIR/.qa_serve"
PORT=8788

mkdir -p "$QA_DIR" "$SERVE_DIR"

# Copy viewer to serve directory
cp "$QA_VIEWER" "$SERVE_DIR/index.html"

# Start local server
python3 -m http.server "$PORT" -d "$SERVE_DIR" &>/dev/null &
SERVER_PID=$!
trap "kill $SERVER_PID 2>/dev/null; rm -rf $SERVE_DIR" EXIT
sleep 0.5

ANGLES="front side quarter top"
VERSION="${VERSION:-1}"  # pass VERSION=3 to increase threshold

for GLB_FILE in "$@"; do
    MODEL_NAME="$(basename "$GLB_FILE" .glb)"
    echo "═══ $MODEL_NAME (v${VERSION}) ═══"

    # Copy GLB to serve directory
    cp "$ENGINE_DIR/$GLB_FILE" "$SERVE_DIR/${MODEL_NAME}.glb"

    # Screenshot each angle + score
    node -e "
const { chromium } = require('playwright');
(async () => {
  const browser = await chromium.launch({
    args: ['--use-gl=angle', '--use-angle=swiftshader'],
  });
  const angles = '${ANGLES}'.split(' ');
  let finalTitle = '';
  for (const angle of angles) {
    const page = await browser.newPage({ viewport: { width: 1200, height: 1200 } });
    await page.goto('http://localhost:${PORT}/index.html?model=${MODEL_NAME}.glb&angle=' + angle + '&version=${VERSION}');
    try {
      await page.waitForFunction(() => document.title.startsWith('READY'), { timeout: 20000 });
    } catch (e) {
      console.log('  ✗ ' + angle + ' (timeout)');
      await page.close();
      continue;
    }
    await page.waitForTimeout(200);
    const outPath = '${QA_DIR}/${MODEL_NAME}_qa_' + angle + '.png';
    await page.screenshot({ path: outPath });
    finalTitle = await page.title();
    console.log('  ✓ ' + angle);
    await page.close();
  }
  // Parse and display score
  const match = finalTitle.match(/score:(\d+)\s+(PASS|FAIL)/);
  if (match) {
    const [, score, result] = match;
    const icon = result === 'PASS' ? '✅' : '❌';
    console.log('  ' + icon + ' Score: ' + score + '/100 (' + result + ')');
  }
  console.log('  ' + finalTitle);
  await browser.close();
})();
" 2>&1

    # Cleanup model
    rm -f "$SERVE_DIR/${MODEL_NAME}.glb"
    echo ""
done

echo "═══ Screenshots: qa/models/ ═══"
