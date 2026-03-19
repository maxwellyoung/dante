#!/usr/bin/env node
/**
 * glb_qa.js — Automated visual QA for GLB models using the viewer
 *
 * Usage: node scripts/glb_qa.js assets/bathtub.glb
 *        node scripts/glb_qa.js assets/bathtub.glb --angles 6
 *
 * Opens the GLB viewer in headless Chromium, loads the model,
 * screenshots from multiple angles, saves to qa/models/<name>_qa_<angle>.png
 *
 * Requires: npx playwright (already installed)
 */
const { chromium } = require('playwright');
const path = require('path');
const fs = require('fs');

const glbPath = process.argv[2];
if (!glbPath) {
  console.error('Usage: node scripts/glb_qa.js <path-to-glb> [--angles N]');
  process.exit(1);
}

const numAngles = process.argv.includes('--angles')
  ? parseInt(process.argv[process.argv.indexOf('--angles') + 1])
  : 4;

const absGlb = path.resolve(glbPath);
const modelName = path.basename(glbPath, '.glb');
const viewerPath = path.resolve('/Users/maxwellyoung/Development/glb-viewer/index.html');
const qaDir = path.resolve(__dirname, '..', 'qa', 'models');

if (!fs.existsSync(absGlb)) {
  console.error(`File not found: ${absGlb}`);
  process.exit(1);
}
fs.mkdirSync(qaDir, { recursive: true });

(async () => {
  const browser = await chromium.launch({ headless: true });
  const page = await browser.newPage({ viewport: { width: 1200, height: 1200 } });

  // Load the viewer
  await page.goto(`file://${viewerPath}`);
  await page.waitForTimeout(1000);

  // Inject the GLB file via drag-and-drop simulation
  const glbBuffer = fs.readFileSync(absGlb);

  await page.evaluate(async (glbBase64, fileName) => {
    // Convert base64 back to ArrayBuffer
    const binary = atob(glbBase64);
    const bytes = new Uint8Array(binary.length);
    for (let i = 0; i < binary.length; i++) bytes[i] = binary.charCodeAt(i);

    // Create a File object and trigger the viewer's file loading
    const file = new File([bytes.buffer], fileName, { type: 'model/gltf-binary' });

    // Find the file input and set it
    const input = document.getElementById('file-input');
    const dt = new DataTransfer();
    dt.items.add(file);
    input.files = dt.files;
    input.dispatchEvent(new Event('change', { bubbles: true }));
  }, glbBuffer.toString('base64'), path.basename(absGlb));

  // Wait for model to load and render
  await page.waitForTimeout(3000);

  // Disable auto-rotate and take screenshots from multiple angles
  const angles = [];
  for (let i = 0; i < numAngles; i++) {
    const theta = (i / numAngles) * Math.PI * 2;
    const label = ['front', 'quarter', 'side', 'back', 'quarter2', 'side2'][i] || `angle_${i}`;
    angles.push({ theta, label });
  }

  // Set camera angles via OrbitControls
  for (const { theta, label } of angles) {
    await page.evaluate((angle) => {
      // Access Three.js scene via module scope — try global references
      // The viewer uses module scope, so we need to dispatch custom events
      // or access controls directly if exposed

      // Fallback: just rotate the auto-rotate and screenshot at intervals
      const event = new CustomEvent('set-camera-angle', { detail: { theta: angle } });
      window.dispatchEvent(event);
    }, theta);

    // Auto-rotate will move the model — wait for it to reach position
    await page.waitForTimeout(800);

    const outPath = path.join(qaDir, `${modelName}_qa_${label}.png`);
    await page.screenshot({ path: outPath, omitBackground: true });
    console.log(`  ✓ ${outPath}`);
  }

  // Get model stats from HUD
  const stats = await page.evaluate(() => {
    const hud = document.getElementById('hud');
    return hud ? hud.textContent : 'no stats';
  });
  console.log(`\nModel: ${modelName}`);
  console.log(`Stats: ${stats}`);
  console.log(`Screenshots: ${numAngles} angles → qa/models/`);

  await browser.close();
})();
