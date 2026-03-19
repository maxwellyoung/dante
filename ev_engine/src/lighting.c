// lighting.c — Custom shader lighting for EV engine
// Per-scene presets. Warm key, cool fill, point light for practicals.
// Tadao Ando shadows. Hotel Chevalier golden hour. Godard moonlight.

#include "lighting.h"
#include "raymath.h"
#include "rlgl.h"
#ifdef __APPLE__
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#endif
#include <stdio.h>

static const char *vs_source =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "in vec2 vertexTexCoord;\n"
    "in vec3 vertexNormal;\n"
    "in vec4 vertexColor;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "uniform mat4 lightSpaceMatrix;\n"
    "out vec3 fragPosition;\n"
    "out vec3 fragNormal;\n"
    "out vec4 fragColor;\n"
    "out vec2 fragTexCoord;\n"
    "out vec4 fragPosLightSpace;\n"
    "void main() {\n"
    "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragColor = vertexColor;\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragPosLightSpace = lightSpaceMatrix * vec4(fragPosition, 1.0);\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *fs_source =
    "#version 330\n"
    "in vec3 fragPosition;\n"
    "in vec3 fragNormal;\n"
    "in vec4 fragColor;\n"
    "in vec2 fragTexCoord;\n"
    "in vec4 fragPosLightSpace;\n"
    "uniform sampler2D shadowMap;\n"
    "uniform vec3 viewPos;\n"
    "uniform vec3 ambient;\n"
    "uniform vec3 fogColor;\n"
    "uniform float fogDensity;\n"
    "uniform vec3 lightDir;\n"
    "uniform vec3 lightColor;\n"
    "uniform vec3 fillDir;\n"
    "uniform vec3 fillColor;\n"
    "uniform vec3 pointPos[8];\n"
    "uniform vec3 pointColor[8];\n"
    "uniform float pointRadius[8];\n"
    "uniform int materialId;\n"
    "uniform float time;\n"
    "uniform sampler2D texture0;\n"
    "uniform vec4 colDiffuse;\n"
    "out vec4 finalColor;\n"
    "\n"
    // === Noise functions for procedural textures ===
    "float hash(vec2 p) {\n"
    "    vec3 p3 = fract(vec3(p.xyx) * 0.1031);\n"
    "    p3 += dot(p3, p3.yzx + 33.33);\n"
    "    return fract((p3.x + p3.y) * p3.z);\n"
    "}\n"
    "float noise(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    f = f * f * (3.0 - 2.0 * f);\n"  // smoothstep
    "    float a = hash(i);\n"
    "    float b = hash(i + vec2(1.0, 0.0));\n"
    "    float c = hash(i + vec2(0.0, 1.0));\n"
    "    float d = hash(i + vec2(1.0, 1.0));\n"
    "    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);\n"
    "}\n"
    "float fbm(vec2 p) {\n"
    "    float v = 0.0;\n"
    "    v += noise(p) * 0.5;\n"
    "    v += noise(p * 2.0) * 0.25;\n"
    "    v += noise(p * 4.0) * 0.125;\n"
    "    return v;\n"
    "}\n"
    "// Voronoi — cellular noise for organic patterns\n"
    "float voronoi(vec2 p) {\n"
    "    vec2 i = floor(p);\n"
    "    vec2 f = fract(p);\n"
    "    float minDist = 1.0;\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            vec2 neighbor = vec2(float(x), float(y));\n"
    "            vec2 point = vec2(hash(i + neighbor), hash(i + neighbor + 17.3));\n"
    "            float d = length(neighbor + point - f);\n"
    "            minDist = min(minDist, d);\n"
    "        }\n"
    "    }\n"
    "    return minDist;\n"
    "}\n"
    "\n"
    // Shadow calculation — PCF 3x3 for soft edges at 480x300
    "float ShadowCalc(vec4 lsPos) {\n"
    "    vec3 proj = lsPos.xyz / lsPos.w;\n"
    "    proj = proj * 0.5 + 0.5;\n"
    "    if (proj.z > 1.0) return 0.0;\n"
    "    float bias = 0.002;\n"
    "    float shadow = 0.0;\n"
    "    vec2 texelSize = 1.0 / vec2(2048.0) * 1.3;\n"
    "    for (int x = -1; x <= 1; x++) {\n"
    "        for (int y = -1; y <= 1; y++) {\n"
    "            float d = texture(shadowMap, proj.xy + vec2(x,y)*texelSize).r;\n"
    "            shadow += proj.z - bias > d ? 1.0 : 0.0;\n"
    "        }\n"
    "    }\n"
    "    return shadow / 9.0;\n"
    "}\n"
    "\n"
    "void main() {\n"
    "    vec4 texColor = texture(texture0, fragTexCoord);\n"
    "    vec3 baseColor = texColor.rgb * colDiffuse.rgb * fragColor.rgb;\n"
    "    vec3 norm = normalize(fragNormal);\n"
    "\n"
    // === Procedural surface detail — modulates baseColor before lighting ===
    // Pick world-space UV based on dominant normal axis (wall vs floor vs ceiling)
    "    vec3 wp = fragPosition;\n"
    "    vec3 an = abs(norm);\n"
    "    vec2 surfUV;\n"
    "    if (an.y > an.x && an.y > an.z) surfUV = wp.xz;\n"  // floor/ceiling
    "    else if (an.x > an.z) surfUV = wp.yz;\n"              // X-facing wall
    "    else surfUV = wp.xy;\n"                                // Z-facing wall
    "\n"
    // Per-material specular multiplier (overridden below)
    "    float specMult = 0.15;\n"
    "\n"
    "    if (materialId == 0) {\n"
    "        // CONCRETE — noise color + normal perturbation for pitting\n"
    "        float n = noise(surfUV * 2.0);\n"
    "        baseColor *= 0.95 + n * 0.08;\n"
    "        // Normal detail — rough pitted surface catches light at micro scale\n"
    "        float nx = noise(surfUV * 4.0 + vec2(0.5, 0.0)) - 0.5;\n"
    "        float nz = noise(surfUV * 4.0 + vec2(0.0, 0.5)) - 0.5;\n"
    "        norm = normalize(norm + vec3(nx, 0.0, nz) * 0.10);\n"
    "        specMult = 0.05;\n"
    "    }\n"
    "    else if (materialId == 1) {\n"
    "        // MARBLE — veins + polished surface normal detail\n"
    "        float vein = sin(surfUV.x * 4.0 + surfUV.y * 2.0 + noise(surfUV * 2.0) * 2.0);\n"
    "        vein = smoothstep(0.0, 0.12, abs(vein));\n"
    "        baseColor *= mix(0.85, 1.0, vein);\n"
    "        // Subtle polish irregularity — makes specular highlights wobble\n"
    "        float mx = noise(surfUV * 6.0) - 0.5;\n"
    "        float mz = noise(surfUV * 6.0 + vec2(3.7, 1.2)) - 0.5;\n"
    "        norm = normalize(norm + vec3(mx, 0.0, mz) * 0.05);\n"
    "        specMult = 0.45;\n"
    "    }\n"
    "    else if (materialId == 2) {\n"
    "        // WOOD — grain color + grain normal ridges\n"
    "        float grain = sin(surfUV.y * 12.0 + noise(surfUV * 3.0) * 2.0) * 0.5 + 0.5;\n"
    "        grain = smoothstep(0.35, 0.65, grain);\n"
    "        baseColor *= 0.90 + grain * 0.10;\n"
    "        // Grain ridges — subtle bumps along grain direction\n"
    "        float grainBump = cos(surfUV.y * 24.0 + noise(surfUV * 3.0) * 4.0) * 0.06;\n"
    "        norm = normalize(norm + vec3(0.0, 0.0, grainBump));\n"
    "        specMult = 0.12;\n"
    "    }\n"
    "    else if (materialId == 3) {\n"
    "        // CARPET — soft fuzzy surface, scatters light\n"
    "        float n = noise(surfUV * 1.5);\n"
    "        baseColor *= 0.94 + n * 0.08;\n"
    "        // Pile direction — random micro-normal for soft scattering\n"
    "        float cx = noise(surfUV * 8.0) - 0.5;\n"
    "        float cz = noise(surfUV * 8.0 + vec2(2.1, 0.7)) - 0.5;\n"
    "        norm = normalize(norm + vec3(cx, 0.0, cz) * 0.14);\n"
    "        specMult = 0.02;\n"
    "    }\n"
    "    else if (materialId == 4) {\n"
    "        // WALLPAPER — subtle warm noise, NOT a visible pattern\n"
    "        float n = fbm(surfUV * 3.0);\n"
    "        baseColor *= 0.97 + n * 0.04;\n"
    "        specMult = 0.06;\n"
    "    }\n"
    "    else if (materialId == 5) {\n"
    "        // TILE — grout lines + per-tile normal variation\n"
    "        vec2 tileUV = fract(surfUV * 3.0);\n"
    "        float grout = smoothstep(0.03, 0.08, tileUV.x) * smoothstep(0.03, 0.08, tileUV.y);\n"
    "        baseColor *= mix(0.80, 1.0, grout);\n"
    "        // Grout recesses — tiles dome outward, grout sinks\n"
    "        norm = normalize(norm + vec3(sin((tileUV.x-0.5)*3.14), 0.0, sin((tileUV.y-0.5)*3.14)) * grout * 0.08);\n"
    "        specMult = 0.25;\n"
    "    }\n"
    "    else if (materialId == 6) {\n"
    "        // BRASS — brushed metal micro-scratches\n"
    "        float n = noise(surfUV * 8.0);\n"
    "        baseColor *= 0.96 + n * 0.06;\n"
    "        // Anisotropic scratches — elongated in one direction\n"
    "        float scratch = noise(vec2(surfUV.x * 30.0, surfUV.y * 2.0)) - 0.5;\n"
    "        norm = normalize(norm + vec3(scratch * 0.04, 0.0, 0.0));\n"
    "        specMult = 0.6;\n"
    "    }\n"
    "    else if (materialId == 7) {\n"
    "        // GLASS — near-perfect, very faint distortion\n"
    "        float gn = noise(surfUV * 12.0) - 0.5;\n"
    "        norm = normalize(norm + vec3(gn * 0.01, 0.0, gn * 0.01));\n"
    "        specMult = 0.55;\n"
    "    }\n"
    "    else if (materialId == 8) {\n"
    "        // LEATHER — pebbled grain texture\n"
    "        float n = noise(surfUV * 6.0);\n"
    "        baseColor *= 0.94 + n * 0.08;\n"
    "        // Pebble bumps — cellular noise for organic leather grain\n"
    "        float v = voronoi(surfUV * 8.0);\n"
    "        norm = normalize(norm + vec3(v - 0.5, 0.0, v - 0.5) * 0.11);\n"
    "        specMult = 0.22;\n"
    "    }\n"
    "    else if (materialId == 9) {\n"
    "        // FABRIC — soft, nearly flat with subtle noise\n"
    "        float n = noise(surfUV * 6.0);\n"
    "        baseColor *= 0.96 + n * 0.05;\n"
    "        specMult = 0.03;\n"
    "    }\n"
    "    else if (materialId == 10) {\n"
    "        // CHECKERBOARD — two-tone with per-tile slight normal tilt\n"
    "        vec2 checker = floor(surfUV * 2.0);\n"
    "        float check = mod(checker.x + checker.y, 2.0);\n"
    "        baseColor *= mix(0.62, 1.0, check);\n"
    "        // Each tile has a very slight random tilt — broken perfection\n"
    "        float tiltX = noise(checker) - 0.5;\n"
    "        float tiltZ = noise(checker + vec2(7.3, 2.1)) - 0.5;\n"
    "        norm = normalize(norm + vec3(tiltX, 0.0, tiltZ) * 0.02);\n"
    "        specMult = 0.3;\n"
    "    }\n"
    "    else if (materialId == 11) {\n"
    "        // HERRINGBONE — finer planks, subtler contrast\n"
    "        vec2 uv = surfUV * 6.0;\n"
    "        float row = floor(uv.y);\n"
    "        float shifted = uv.x + mod(row, 2.0) * 0.5;\n"
    "        float plank = mod(floor(shifted), 2.0);\n"
    "        baseColor *= mix(0.92, 1.0, plank);\n"
    "        vec2 plankUV = fract(vec2(shifted, uv.y));\n"
    "        float grout = smoothstep(0.02, 0.06, plankUV.x) * smoothstep(0.02, 0.06, plankUV.y);\n"
    "        baseColor *= mix(0.78, 1.0, grout);\n"
    "        // Wood grain bumps along plank direction\n"
    "        float grainN = cos(plankUV.y * 30.0 + noise(plankUV * 4.0) * 3.0) * 0.03;\n"
    "        norm = normalize(norm + vec3(grainN * (1.0 - grout), 0.0, 0.0));\n"
    "        specMult = 0.15;\n"
    "    }\n"
    "    else if (materialId == 12) {\n"
    "        // PARQUET — alternating tile direction with grain bumps\n"
    "        vec2 tile = floor(surfUV * 2.5);\n"
    "        float dir = mod(tile.x + tile.y, 2.0);\n"
    "        vec2 localUV = fract(surfUV * 2.5);\n"
    "        float grainCoord = (dir > 0.5) ? localUV.x : localUV.y;\n"
    "        float grain = sin(grainCoord * 20.0) * 0.5 + 0.5;\n"
    "        baseColor *= 0.92 + grain * 0.08;\n"
    "        float border = smoothstep(0.02, 0.05, localUV.x) * smoothstep(0.02, 0.05, localUV.y)\n"
    "                      * smoothstep(0.02, 0.05, 1.0-localUV.x) * smoothstep(0.02, 0.05, 1.0-localUV.y);\n"
    "        baseColor *= mix(0.80, 1.0, border);\n"
    "        // Grain normal in alternating directions\n"
    "        float gBump = cos(grainCoord * 40.0) * 0.025;\n"
    "        vec3 grainDir = (dir > 0.5) ? vec3(gBump, 0.0, 0.0) : vec3(0.0, 0.0, gBump);\n"
    "        norm = normalize(norm + grainDir * border);\n"
    "        specMult = 0.14;\n"
    "    }\n"
    "    else if (materialId == 13) {\n"
    "        // VELVET — view-dependent sheen + micro-fiber normal scatter\n"
    "        float NdotV = max(dot(norm, normalize(viewPos - fragPosition)), 0.0);\n"
    "        float sheen = pow(1.0 - NdotV, 1.8) * 0.35;\n"
    "        baseColor *= 0.90 + sheen;\n"
    "        // Micro-fiber scatter — random per-pixel normal tilt\n"
    "        float fx = noise(surfUV * 16.0) - 0.5;\n"
    "        float fz = noise(surfUV * 16.0 + vec2(5.3, 1.7)) - 0.5;\n"
    "        norm = normalize(norm + vec3(fx, 0.0, fz) * 0.12);\n"
    "        specMult = 0.04;\n"
    "    }\n"
    "    else if (materialId == 14) {\n"
    "        // WATER — Source-style: scrolling normals, Fresnel, fake env reflection\n"
    "        // 1) DUAL SCROLLING NORMALS — two noise layers, different speed/direction\n"
    "        vec2 scroll1 = surfUV * 3.0 + vec2(time * 0.08, time * 0.06);\n"
    "        vec2 scroll2 = surfUV * 2.0 + vec2(-time * 0.05, time * 0.07);\n"
    "        float n1 = noise(scroll1) - 0.5;\n"
    "        float n2 = noise(scroll2) - 0.5;\n"
    "        float n3 = noise(surfUV * 5.0 + time * 0.03) - 0.5;\n"
    "        vec3 perturbedN = normalize(norm + vec3(\n"
    "            (n1 + n2 * 0.6) * 0.12,\n"
    "            0.0,\n"
    "            (n1 * 0.7 - n2 * 0.5 + n3 * 0.3) * 0.10\n"
    "        ));\n"
    "        norm = perturbedN;\n"
    "\n"
    "        // 2) FRESNEL — reflective at glancing, transparent looking down\n"
    "        vec3 V = normalize(viewPos - fragPosition);\n"
    "        float NdotV = max(dot(norm, V), 0.0);\n"
    "        float fresnel = pow(1.0 - NdotV, 5.0);\n"
    "        fresnel = mix(0.04, 1.0, fresnel);\n"
    "\n"
    "        // 3) FAKE ENVIRONMENT REFLECTION — sample point lights as reflection sources\n"
    "        vec3 reflDir = reflect(-V, norm);\n"
    "        vec3 envColor = vec3(0.0);\n"
    "        for (int i = 0; i < 4; i++) {\n"
    "            if (pointRadius[i] > 0.0) {\n"
    "                vec3 toLight = normalize(pointPos[i] - fragPosition);\n"
    "                float reflMatch = max(dot(reflDir, toLight), 0.0);\n"
    "                reflMatch = pow(reflMatch, 8.0);\n"
    "                float dist = length(pointPos[i] - fragPosition);\n"
    "                float atten = 1.0 / (1.0 + dist * dist / (pointRadius[i] * pointRadius[i]));\n"
    "                envColor += pointColor[i] * reflMatch * atten * 0.6;\n"
    "            }\n"
    "        }\n"
    "        // Key light reflection\n"
    "        float keyRefl = pow(max(dot(reflDir, -lightDir), 0.0), 16.0);\n"
    "        envColor += lightColor * keyRefl * 0.3;\n"
    "        // Sky/ambient contribution to reflection\n"
    "        envColor += ambient * 0.4;\n"
    "\n"
    "        // 4) DEPTH-BASED COLOR — dark center, slight tint from baseColor\n"
    "        vec3 deepColor = baseColor * 0.15 + vec3(0.005, 0.012, 0.025);\n"
    "        // Animated caustics on the deep layer\n"
    "        float caustic = noise(surfUV * 6.0 + time * 0.12);\n"
    "        caustic = smoothstep(0.55, 0.75, caustic) * 0.06;\n"
    "        deepColor += vec3(caustic * 0.4, caustic * 0.6, caustic);\n"
    "\n"
    "        // Blend: reflection vs deep based on Fresnel\n"
    "        baseColor = mix(deepColor, envColor, fresnel);\n"
    "        specMult = 0.9;\n"
    "    }\n"
    "    else if (materialId == 15) {\n"
    "        // PUDDLE — Source-style: scrolling normals, Fresnel, env reflection, dissolve edges\n"
    "        // 1) EDGE DISSOLVE — organic blob shape\n"
    "        float edgeNoise = fbm(surfUV * 2.0);\n"
    "        vec2 puv = fract(surfUV * 0.5);\n"
    "        float edgeDist = min(min(puv.x, 1.0 - puv.x), min(puv.y, 1.0 - puv.y));\n"
    "        float dissolve = smoothstep(0.0, 0.15, edgeDist + edgeNoise * 0.2 - 0.1);\n"
    "        if (dissolve < 0.05) discard;\n"
    "\n"
    "        // 2) SCROLLING NORMALS — subtle, rain-disturbed surface\n"
    "        vec2 ps1 = surfUV * 4.0 + vec2(time * 0.05, time * 0.03);\n"
    "        vec2 ps2 = surfUV * 3.0 + vec2(-time * 0.04, time * 0.06);\n"
    "        float pn1 = noise(ps1) - 0.5;\n"
    "        float pn2 = noise(ps2) - 0.5;\n"
    "        norm = normalize(norm + vec3((pn1 + pn2 * 0.5) * 0.08, 0.0, (pn1 - pn2) * 0.06));\n"
    "\n"
    "        // Raindrop ripples — multiple expanding rings\n"
    "        float drops = 0.0;\n"
    "        for (int d = 0; d < 3; d++) {\n"
    "            vec2 center = vec2(hash(vec2(float(d)*3.7, 1.3)), hash(vec2(float(d)*5.1, 2.7)));\n"
    "            center = center * 6.0 - 3.0;\n"
    "            float phase = fract(time * 0.4 + float(d) * 0.33);\n"
    "            float ring = length(surfUV * 4.0 - center) - phase * 2.0;\n"
    "            ring = 1.0 - smoothstep(0.0, 0.15, abs(ring));\n"
    "            ring *= 1.0 - phase;\n"
    "            drops += ring * 0.03;\n"
    "        }\n"
    "        norm = normalize(norm + vec3(drops, 0.0, drops));\n"
    "\n"
    "        // 3) FRESNEL\n"
    "        vec3 V = normalize(viewPos - fragPosition);\n"
    "        float NdotV = max(dot(norm, V), 0.0);\n"
    "        float fresnel = pow(1.0 - NdotV, 5.0);\n"
    "        fresnel = mix(0.02, 0.9, fresnel);\n"
    "\n"
    "        // 4) FAKE ENVIRONMENT REFLECTION — sample point lights\n"
    "        vec3 reflDir = reflect(-V, norm);\n"
    "        vec3 envColor = vec3(0.0);\n"
    "        for (int i = 0; i < 4; i++) {\n"
    "            if (pointRadius[i] > 0.0) {\n"
    "                vec3 toLight = normalize(pointPos[i] - fragPosition);\n"
    "                float reflMatch = max(dot(reflDir, toLight), 0.0);\n"
    "                reflMatch = pow(reflMatch, 6.0);\n"
    "                float dist = length(pointPos[i] - fragPosition);\n"
    "                float atten = 1.0 / (1.0 + dist * dist / (pointRadius[i] * pointRadius[i]));\n"
    "                envColor += pointColor[i] * reflMatch * atten * 0.5;\n"
    "            }\n"
    "        }\n"
    "        envColor += lightColor * pow(max(dot(reflDir, -lightDir), 0.0), 12.0) * 0.25;\n"
    "        envColor += ambient * 0.3;\n"
    "\n"
    "        // 5) GROUND COLOR — dark wet pavement\n"
    "        vec3 groundColor = baseColor * 0.15 + vec3(0.005, 0.008, 0.015);\n"
    "\n"
    "        // Blend: ground vs reflection based on Fresnel and dissolve\n"
    "        baseColor = mix(groundColor, envColor, fresnel * dissolve);\n"
    "        // Edge brightening — thin film interference at puddle boundary\n"
    "        float edgeBright = smoothstep(0.05, 0.15, dissolve) * (1.0 - smoothstep(0.15, 0.4, dissolve));\n"
    "        baseColor += envColor * edgeBright * 0.2;\n"
    "        specMult = 0.7;\n"
    "    }\n"
    "    else if (materialId == 16) {\n"
    "        // EMISSIVE — unlit, outputs baseColor directly. For Earth, glow panels.\n"
    "        // Bypass all lighting — apply fog only, then output.\n"
    "        float fogDist = length(viewPos - fragPosition);\n"
    "        float fog = 1.0 - exp(-fogDensity * fogDist * fogDist);\n"
    "        fog = clamp(fog, 0.0, 1.0);\n"
    "        vec3 emissive = mix(baseColor, fogColor, fog);\n"
    "        finalColor = vec4(emissive, texColor.a * colDiffuse.a * fragColor.a);\n"
    "        return;\n"
    "    }\n"
    "\n"
    "    // Shadow\n"
    "    float shadow = ShadowCalc(fragPosLightSpace);\n"
    "\n"
    "    vec3 viewDir = normalize(viewPos - fragPosition);\n"
    "    float viewDot = max(dot(norm, viewDir), 0.0);\n"
    "\n"
    "    // ── Key light — Half-Lambert wrap (Valve's TF2 technique) ──\n"
    "    // Wraps light around surfaces instead of hard cutoff at 90 degrees.\n"
    "    // This is THE fix for flat-looking geometry.\n"
    "    float NdotL = dot(norm, -lightDir);\n"
    "    float halfLambert = NdotL * 0.5 + 0.5;\n"
    "    halfLambert = halfLambert * halfLambert;\n"  // square for softer falloff
    "    vec3 keyDiffuse = lightColor * halfLambert * (1.0 - shadow * 0.7);\n"
    "\n"
    "    // ── Fill light — Half-Lambert wrap, stronger contribution ──\n"
    "    // Fill gives dimensionality to the shadow side. Was 0.4x, now 0.7x.\n"
    "    float NdotF = dot(norm, -fillDir);\n"
    "    float fillHalf = NdotF * 0.5 + 0.5;\n"
    "    fillHalf = fillHalf * fillHalf;\n"
    "    vec3 fillDiffuse = fillColor * fillHalf * 0.7;\n"
    "\n"
    "    // ── Point lights — practicals with wrap lighting ──\n"
    "    vec3 pointLit = vec3(0.0);\n"
    "    for (int i = 0; i < 8; i++) {\n"
    "        if (pointRadius[i] > 0.0) {\n"
    "            vec3 toLight = pointPos[i] - fragPosition;\n"
    "            float pDist = length(toLight);\n"
    "            vec3 pDir = toLight / max(pDist, 0.001);\n"
    "            float NdotP = dot(norm, pDir) * 0.5 + 0.5;\n"  // half-Lambert for points too
    "            NdotP = NdotP * NdotP;\n"
    "            float atten = 1.0 / (1.0 + pDist * pDist / (pointRadius[i] * pointRadius[i] * 0.25));\n"
    "            atten *= smoothstep(pointRadius[i], pointRadius[i] * 0.5, pDist);\n"
    "            pointLit += pointColor[i] * NdotP * atten;\n"
    "            // Light pool on floors — warm circles of life\n"
    "            if (norm.y > 0.9) {\n"
    "                float floorDist = length(fragPosition.xz - pointPos[i].xz);\n"
    "                float poolR = pointRadius[i] * 0.35;\n"
    "                float pool = smoothstep(poolR, poolR * 0.15, floorDist);\n"
    "                pointLit += pointColor[i] * pool * 0.55 * atten;\n"
    "            }\n"
    "        }\n"
    "    }\n"
    "\n"
    "    // ── Rim light — edge catch, architectural silhouette ──\n"
    "    // Uses mix of key + fill for per-scene color variation.\n"
    "    float rim = 1.0 - viewDot;\n"
    "    rim = pow(rim, 2.5) * 0.22;\n"
    "    vec3 rimTint = mix(lightColor, fillColor, 0.4);\n"
    "    vec3 rimColor = rimTint * rim;\n"
    "\n"
    "    // ── Specular — per-material power + intensity ──\n"
    "    // Variable hardness: rough materials = broad soft highlight,\n"
    "    // smooth materials = tight sharp highlight.\n"
    "    float specPower = 32.0;\n"
    "    if (materialId == 6) specPower = 72.0;\n"       // brass — razor sharp
    "    else if (materialId == 7) specPower = 96.0;\n"  // glass — mirror tight
    "    else if (materialId == 1) specPower = 48.0;\n"  // marble — polished
    "    else if (materialId == 2) specPower = 16.0;\n"  // wood — broad
    "    else if (materialId == 3) specPower = 4.0;\n"   // carpet — almost none
    "    else if (materialId == 9) specPower = 6.0;\n"   // fabric — almost none
    "    else if (materialId == 13) specPower = 8.0;\n"  // velvet — soft sheen
    "    else if (materialId == 8) specPower = 12.0;\n"  // leather — broad
    "    else if (materialId == 14) specPower = 180.0;\n" // water — razor sun glint
    "    else if (materialId == 15) specPower = 96.0;\n"  // puddle — tight wet highlight
    "    vec3 halfDir = normalize(-lightDir + viewDir);\n"
    "    float spec = pow(max(dot(norm, halfDir), 0.0), specPower);\n"
    "    vec3 specColor = lightColor * spec * specMult * 1.3;\n"
    "\n"
    "    // ── Ambient Occlusion — multi-factor ──\n"
    "    // 1) Normal vs up — downward-facing surfaces darken\n"
    "    float aoUp = 0.5 + 0.5 * max(dot(norm, vec3(0.0, 1.0, 0.0)), 0.0);\n"
    "    // 2) Height gradient — lower parts of rooms naturally darker\n"
    "    float aoHeight = smoothstep(-0.5, 4.0, fragPosition.y);\n"
    "    // 3) Crease detection — where surfaces meet at angles\n"
    "    //    View-space normal curvature approximation\n"
    "    float aoCrease = smoothstep(0.0, 0.25, viewDot);\n"
    "    float ao = mix(0.30, 1.0, aoUp * mix(0.6, 1.0, aoHeight) * mix(0.5, 1.0, aoCrease));\n"
    "\n"
    "    // Self-lit — only very bright surfaces glow (light fixtures, not walls)\n"
    "    float brightness = dot(baseColor, vec3(0.299, 0.587, 0.114));\n"
    "    float selfLit = smoothstep(0.85, 0.95, brightness) * 0.2;\n"
    "\n"
    "    // ── Fresnel — edges catch more light (architectural depth cue) ──\n"
    "    float fresnel = pow(1.0 - viewDot, 2.5) * 0.18;\n"
    "\n"
    "    vec3 lit = baseColor * (ambient * ao * (1.0 - selfLit) + keyDiffuse + fillDiffuse + pointLit + selfLit)"
    "              + rimColor + specColor + vec3(fresnel);\n"
    "\n"
    "    // Fog — warm, not black\n"
    "    float fogDist = length(viewPos - fragPosition);\n"
    "    float fog = 1.0 - exp(-fogDensity * fogDist * fogDist);\n"
    "    fog = clamp(fog, 0.0, 1.0);\n"
    "    lit = mix(lit, fogColor, fog);\n"
    "\n"
    "    // Height fog — pools at floor level, thins above knee height\n"
    "    float heightFog = smoothstep(0.3, 0.0, fragPosition.y) * 0.15 * fogDensity * 500.0;\n"
    "    lit = mix(lit, fogColor, clamp(heightFog, 0.0, 0.12));\n"
    "\n"
    "    finalColor = vec4(lit, texColor.a * colDiffuse.a * fragColor.a);\n"
    "}\n";

EVLighting LoadEVLighting(void) {
    EVLighting lighting = {0};
    lighting.shader = LoadShaderFromMemory(vs_source, fs_source);

    if (lighting.shader.id > 0) {
        lighting.shader.locs[SHADER_LOC_MATRIX_MODEL] = GetShaderLocation(lighting.shader, "matModel");
        lighting.shader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(lighting.shader, "mvp");
        lighting.shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(lighting.shader, "viewPos");
        lighting.viewPosLoc = GetShaderLocation(lighting.shader, "viewPos");
        lighting.ambientLoc = GetShaderLocation(lighting.shader, "ambient");
        lighting.fogColorLoc = GetShaderLocation(lighting.shader, "fogColor");
        lighting.fogDensityLoc = GetShaderLocation(lighting.shader, "fogDensity");
        lighting.lightDirLoc = GetShaderLocation(lighting.shader, "lightDir");
        lighting.lightColorLoc = GetShaderLocation(lighting.shader, "lightColor");
        lighting.fillDirLoc = GetShaderLocation(lighting.shader, "fillDir");
        lighting.fillColorLoc = GetShaderLocation(lighting.shader, "fillColor");
        for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
            char name[32];
            snprintf(name, sizeof(name), "pointPos[%d]", i);
            lighting.pointPosLoc[i] = GetShaderLocation(lighting.shader, name);
            snprintf(name, sizeof(name), "pointColor[%d]", i);
            lighting.pointColorLoc[i] = GetShaderLocation(lighting.shader, name);
            snprintf(name, sizeof(name), "pointRadius[%d]", i);
            lighting.pointRadiusLoc[i] = GetShaderLocation(lighting.shader, name);
        }
        lighting.materialIdLoc = GetShaderLocation(lighting.shader, "materialId");
        lighting.timeLoc = GetShaderLocation(lighting.shader, "time");
        lighting.shadowMapLoc = GetShaderLocation(lighting.shader, "shadowMap");
        lighting.lightSpaceMatrixLoc = GetShaderLocation(lighting.shader, "lightSpaceMatrix");
        int matNormalLoc = GetShaderLocation(lighting.shader, "matNormal");
        lighting.shader.locs[SHADER_LOC_MATRIX_NORMAL] = matNormalLoc;

        // Apply default preset (room)
        SetSceneLighting(&lighting, LightingPreset_Room());

        lighting.ready = true;
        printf("[EV] Lighting loaded — per-scene presets ready\n");
    } else {
        printf("[EV] WARNING: Lighting shader failed\n");
        lighting.ready = false;
    }
    return lighting;
}

void UnloadEVLighting(EVLighting *lighting) {
    if (lighting->ready) { UnloadShader(lighting->shader); lighting->ready = false; }
}

void UpdateEVLighting(EVLighting *lighting, Camera3D camera, Color fogColor, float fogDensity, float time) {
    if (!lighting->ready) return;
    float viewPos[3] = {camera.position.x, camera.position.y, camera.position.z};
    SetShaderValue(lighting->shader, lighting->viewPosLoc, viewPos, SHADER_UNIFORM_VEC3);
    float fogCol[3] = {fogColor.r / 255.0f, fogColor.g / 255.0f, fogColor.b / 255.0f};
    SetShaderValue(lighting->shader, lighting->fogColorLoc, fogCol, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fogDensityLoc, &fogDensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(lighting->shader, lighting->timeLoc, &time, SHADER_UNIFORM_FLOAT);
}

void SetSceneLighting(EVLighting *lighting, SceneLighting preset) {
    if (!lighting->ready) return;
    lighting->activePreset = preset;

    float keyDir[3] = {preset.keyDir.x, preset.keyDir.y, preset.keyDir.z};
    SetShaderValue(lighting->shader, lighting->lightDirLoc, keyDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->lightColorLoc, preset.keyColor, SHADER_UNIFORM_VEC3);

    float fillDir[3] = {preset.fillDir.x, preset.fillDir.y, preset.fillDir.z};
    SetShaderValue(lighting->shader, lighting->fillDirLoc, fillDir, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->fillColorLoc, preset.fillColor, SHADER_UNIFORM_VEC3);

    SetShaderValue(lighting->shader, lighting->ambientLoc, preset.ambient, SHADER_UNIFORM_VEC3);

    // Point lights
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        float pPos[3] = {preset.pointPos[i].x, preset.pointPos[i].y, preset.pointPos[i].z};
        SetShaderValue(lighting->shader, lighting->pointPosLoc[i], pPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(lighting->shader, lighting->pointColorLoc[i], preset.pointColor[i], SHADER_UNIFORM_VEC3);
        SetShaderValue(lighting->shader, lighting->pointRadiusLoc[i], &preset.pointRadius[i], SHADER_UNIFORM_FLOAT);
    }
}

void SetPointLight(EVLighting *lighting, float x, float y, float z,
                   float r, float g, float b, float radius) {
    SetPointLightIdx(lighting, 0, x, y, z, r, g, b, radius);
}

void SetPointLightIdx(EVLighting *lighting, int index, float x, float y, float z,
                      float r, float g, float b, float radius) {
    if (!lighting->ready || index < 0 || index >= MAX_POINT_LIGHTS) return;
    float pos[3] = {x, y, z};
    float col[3] = {r, g, b};
    SetShaderValue(lighting->shader, lighting->pointPosLoc[index], pos, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointColorLoc[index], col, SHADER_UNIFORM_VEC3);
    SetShaderValue(lighting->shader, lighting->pointRadiusLoc[index], &radius, SHADER_UNIFORM_FLOAT);
}

void SetMaterialId(EVLighting *lighting, int materialId) {
    if (!lighting->ready) return;
    SetShaderValue(lighting->shader, lighting->materialIdLoc, &materialId, SHADER_UNIFORM_INT);
}

void AnimateLights(EVLighting *lighting, float time) {
    if (!lighting->ready) return;
    SceneLighting *p = &lighting->activePreset;
    for (int i = 0; i < MAX_POINT_LIGHTS; i++) {
        if (p->pointRadius[i] <= 0.0f) continue;
        float flick = 1.0f;
        if (p->pointFlicker[i] > 0.0f) {
            flick = 1.0f - p->pointFlicker[i] *
                (0.5f + 0.5f * sinf(time * 17.0f + p->pointPhase[i]));
        }
        float pulse = 1.0f;
        if (p->pointPulse[i] > 0.0f) {
            pulse = 1.0f + p->pointPulse[i] *
                sinf(time * p->pointPhase[i] * 0.5f);
        }
        float mod = flick * pulse;
        float col[3] = {
            p->pointColor[i][0] * mod,
            p->pointColor[i][1] * mod,
            p->pointColor[i][2] * mod,
        };
        SetShaderValue(lighting->shader, lighting->pointColorLoc[i], col, SHADER_UNIFORM_VEC3);
    }
}

// ============================================================
// SHADOW MAPPING
// ============================================================

static const char *shadow_vs =
    "#version 330\n"
    "in vec3 vertexPosition;\n"
    "uniform mat4 mvp;\n"
    "void main() {\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char *shadow_fs =
    "#version 330\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    finalColor = vec4(1.0);\n"
    "}\n";

void CreateShadowMap(EVLighting *lighting) {
    if (lighting->shadowReady) return;

    // Load depth-only shader
    lighting->shadowShader = LoadShaderFromMemory(shadow_vs, shadow_fs);
    if (lighting->shadowShader.id == 0) {
        printf("[EV] WARNING: Shadow shader failed\n");
        return;
    }
    lighting->shadowMvpLoc = GetShaderLocation(lighting->shadowShader, "mvp");

    // Create FBO with depth texture
    lighting->shadowFBO = rlLoadFramebuffer();
    if (lighting->shadowFBO == 0) {
        printf("[EV] WARNING: Shadow FBO failed\n");
        return;
    }

    rlEnableFramebuffer(lighting->shadowFBO);

    lighting->shadowDepthTex = rlLoadTextureDepth(SHADOW_MAP_SIZE, SHADOW_MAP_SIZE, false);
    rlTextureParameters(lighting->shadowDepthTex, RL_TEXTURE_WRAP_S, RL_TEXTURE_WRAP_CLAMP);
    rlTextureParameters(lighting->shadowDepthTex, RL_TEXTURE_WRAP_T, RL_TEXTURE_WRAP_CLAMP);
    rlFramebufferAttach(lighting->shadowFBO, lighting->shadowDepthTex,
                        RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0);

    // NOTE: glDrawBuffer(GL_NONE) omitted — Apple's Metal-translated GL
    // doesn't isolate per-FBO draw buffer state, kills color writes globally.
    // Depth-only FBO works fine without it (no color attachment = no color writes).

    if (rlFramebufferComplete(lighting->shadowFBO)) {
        lighting->shadowReady = true;
        lighting->shadowPassRan = false;
        printf("[EV] Shadow map created — %dx%d depth texture\n", SHADOW_MAP_SIZE, SHADOW_MAP_SIZE);
    } else {
        printf("[EV] WARNING: Shadow FBO incomplete\n");
        rlUnloadFramebuffer(lighting->shadowFBO);
        lighting->shadowFBO = 0;
    }

    rlDisableFramebuffer();
}

void DestroyShadowMap(EVLighting *lighting) {
    if (!lighting->shadowReady) return;
    rlUnloadTexture(lighting->shadowDepthTex);
    rlUnloadFramebuffer(lighting->shadowFBO);
    UnloadShader(lighting->shadowShader);
    lighting->shadowReady = false;
}

void UpdateShadowMatrix(EVLighting *lighting, Vector3 keyDir, Vector3 sceneCenter, float sceneRadius) {
    if (!lighting->ready) return;

    // Light camera — orthographic, looking at scene from key light direction
    Vector3 lightPos = Vector3Add(sceneCenter, Vector3Scale(Vector3Negate(keyDir), sceneRadius * 2.0f));
    Matrix lightView = MatrixLookAt(lightPos, sceneCenter, (Vector3){0, 1, 0});
    Matrix lightProj = MatrixOrtho(-sceneRadius, sceneRadius, -sceneRadius, sceneRadius,
                                    0.1f, sceneRadius * 4.0f);
    lighting->lightSpaceMatrix = MatrixMultiply(lightView, lightProj);

    // Upload to lighting shader
    SetShaderValueMatrix(lighting->shader, lighting->lightSpaceMatrixLoc, lighting->lightSpaceMatrix);
}

void BindShadowMap(EVLighting *lighting) {
    if (!lighting->shadowReady || !lighting->ready) return;
    // Bind shadow depth texture to texture unit 1
    rlActiveTextureSlot(1);
    rlEnableTexture(lighting->shadowDepthTex);
    int slot = 1;
    SetShaderValue(lighting->shader, lighting->shadowMapLoc, &slot, SHADER_UNIFORM_INT);
}

void UnbindShadowMap(void) {
    rlActiveTextureSlot(1);
    rlDisableTexture();
    rlActiveTextureSlot(0);
}

// ============================================================
// PER-SCENE LIGHTING PRESETS
// Each scene has its own emotional lighting. This is Hotel Chevalier.
// ============================================================

SceneLighting LightingPreset_Taxi(void) {
    // City light fragments through rain-streaked windows
    // Michael Mann night: dark but EVERY element reads
    // Half-Lambert note: pull back key ~30%, lower ambient — wrap does the work now
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.3f, -0.6f}),
        .keyColor = {1.2f, 0.9f, 0.58f},         // sodium streetlight — pulled back for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.4f, 0.5f}),
        .fillColor = {0.35f, 0.38f, 0.52f},       // dashboard blue
        .ambient = {0.18f, 0.16f, 0.20f},          // lower — half-Lambert wraps enough light
        // Taxi meter glow — green-teal dashboard
        .pointPos = {{0.45f, 0.72f, -1.06f}},
        .pointColor = {{0.6f, 1.0f, 0.8f}},
        .pointRadius = {5.0f}
    };
}

SceneLighting LightingPreset_Exterior(void) {
    // Auckland at 2AM — moonlight gradient, not a directional sun
    // Half-Lambert wraps the moonlight beautifully — pull it back
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.2f, -0.7f, 0.3f}),
        .keyColor = {0.30f, 0.34f, 0.45f},        // moonlight — moderated
        .fillDir = Vector3Normalize((Vector3){0.0f, 0.4f, -0.6f}),
        .fillColor = {0.10f, 0.09f, 0.08f},       // ground bounce — subtle
        .ambient = {0.10f, 0.11f, 0.15f},          // lower — night should feel dark
        // lamppost + warm hotel entrance spill
        .pointPos = {{-6.0f, 3.8f, -1.0f}, {0.0f, 2.0f, -4.0f}},
        .pointColor = {{0.8f, 0.65f, 0.40f}, {1.0f, 0.75f, 0.45f}},
        .pointRadius = {14.0f, 8.0f},
    };
}

SceneLighting LightingPreset_Lobby(void) {
    // Grand lobby — one dominant chandelier, deep shadows elsewhere
    // Wes Anderson symmetry: light pools on marble, dark wood walls
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.9f, -0.4f}),
        .keyColor = {0.9f, 0.65f, 0.35f},          // chandelier — steep, concentrated
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.4f}),
        .fillColor = {0.04f, 0.03f, 0.02f},        // barely visible counter bounce
        .ambient = {0.02f, 0.015f, 0.01f},          // near-black — only light pools illuminate
        // Main chandelier HOT, peripherals cold and dim — creates drama
        .pointPos = {{-2.0f, 6.4f, -2.0f}, {-4, 6.4f, 0}, {4, 6.4f, -3}, {0, 3.5f, -5}},
        .pointColor = {{1.4f, 1.0f, 0.55f}, {0.15f, 0.12f, 0.06f}, {0.15f, 0.12f, 0.06f}, {0.30f, 0.22f, 0.12f}},
        .pointRadius = {7.0f, 5.0f, 5.0f, 4.0f},
        .pointFlicker = {0.08f, 0.03f, 0.03f, 0.05f},
        .pointPulse = {0.05f, 0, 0, 0.03f},
        .pointPhase = {0.7f, 2.1f, 3.5f, 1.2f},
    };
}

SceneLighting LightingPreset_Elevator(void) {
    // Enclosed brass box — warm amber overhead, dark marble floor absorbs
    // Hotel Chevalier golden cage. Not clinical fluorescent.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .keyColor = {0.60f, 0.50f, 0.35f},       // warm amber — pulled back for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.0f, 1.0f, 0.0f}),
        .fillColor = {0.05f, 0.04f, 0.03f},      // dark marble absorbs
        .ambient = {0.03f, 0.03f, 0.025f},        // VERY LOW — brass catches light, deep shadows
        .pointPos = {{0, 2.4f, 0}, {-0.9f, 1.2f, 0}},
        .pointColor = {{0.7f, 0.58f, 0.38f}, {0.10f, 0.15f, 0.25f}},
        .pointRadius = {3.5f, 2.5f},
    };
}

SceneLighting LightingPreset_ElevatorSpace(void) {
    // Brass box in space — warm overhead panel, Earth blue from below
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .keyColor = {0.48f, 0.44f, 0.35f},       // warm brass — pulled back
        .fillDir = Vector3Normalize((Vector3){-0.3f, 1.0f, 0.0f}),
        .fillColor = {0.08f, 0.12f, 0.18f},      // Earth blue from below
        .ambient = {0.05f, 0.06f, 0.08f},         // low — brass catches light
        .pointPos = {{0, 2.4f, 0}, {-1.0f, -0.5f, 0}},
        .pointColor = {{0.45f, 0.38f, 0.28f}, {0.06f, 0.10f, 0.18f}},
        .pointRadius = {3.0f, 5.0f},
    };
}

SceneLighting LightingPreset_Hallway(void) {
    // Long corridor — overhead practicals at intervals, warm but dim
    // Think: Kubrick's Shining hallway, but warmer
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.85f, -0.3f}),
        .keyColor = {0.6f, 0.52f, 0.36f},        // warm overhead — pulled back
        .fillDir = Vector3Normalize((Vector3){0.4f, 0.3f, 0.2f}),
        .fillColor = {0.10f, 0.09f, 0.07f},      // wall bounce — dimmer
        .ambient = {0.06f, 0.05f, 0.04f},         // low — corridor should feel enclosed
        // First ceiling light panel
        .pointPos = {{0, 3.9f, -3.0f}},
        .pointColor = {{0.6f, 0.48f, 0.3f}},
        .pointRadius = {8.0f},
    };
}

SceneLighting LightingPreset_Room(void) {
    // The hotel room — the emotional core
    // 2AM: hot overhead pool vs dark corners. Bedside warmth for the bed area.
    // Contrast comes from key being DIRECTIONAL (steep angle) not from darkness.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.92f, -0.2f}),  // very steep — dramatic floor pools
        .keyColor = {0.85f, 0.65f, 0.38f},        // golden — enough to read surfaces
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.02f, 0.03f, 0.05f},       // barely-there night blue
        .ambient = {0.02f, 0.018f, 0.015f},        // low but not black
        // HOT ceiling pool (wide) + warm bedside lamps (tight)
        .pointPos = {{0, 3.68f, 0}, {-2.5f, 0.85f, -3.8f}, {2.5f, 0.85f, -3.8f}},
        .pointColor = {{1.0f, 0.72f, 0.40f}, {0.65f, 0.48f, 0.26f}, {0.65f, 0.48f, 0.26f}},
        .pointRadius = {7.0f, 3.5f, 3.5f},
        .pointFlicker = {0.04f, 0.12f, 0.12f},
        .pointPulse = {0.06f, 0.03f, 0.03f},
        .pointPhase = {0.5f, 1.8f, 3.1f},
    };
}

SceneLighting LightingPreset_Bathroom(void) {
    // Harsh overhead, clinical white — Mirror's Edge energy
    // Strong angled key casts visible shadows on tub edge and basin
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.3f, -0.9f, -0.2f}),
        .keyColor = {0.65f, 0.62f, 0.58f},       // clinical white — moderate
        .fillDir = Vector3Normalize((Vector3){-0.3f, 0.5f, 0.2f}),
        .fillColor = {0.02f, 0.03f, 0.04f},      // minimal cool
        .ambient = {0.015f, 0.018f, 0.022f},      // near-black — stark shadows under fixtures
        // Ando slot window — the ONE bright element
        .pointPos = {{0, 2.6f, -1.88f}},
        .pointColor = {{0.7f, 0.68f, 0.65f}},
        .pointRadius = {3.0f},
    };
}

SceneLighting LightingPreset_Balcony(void) {
    // Orbital observation deck — Earth glow from below, starfield above
    // The contrast IS the emotion — cold void, warm chair.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, 0.6f, -0.8f}),    // from Earth — uplighting
        .keyColor = {0.32f, 0.45f, 0.62f},        // Earth blue — pulled back, wrap does the work
        .fillDir = Vector3Normalize((Vector3){0.0f, -0.5f, 0.5f}),
        .fillColor = {0.25f, 0.18f, 0.10f},       // warm back wall
        .ambient = {0.12f, 0.14f, 0.20f},          // moderate — space is never pitch black
        // Earth glow + interior lamp + floor accents
        .pointPos = {{0.0f, 0.5f, -4.0f}, {0.0f, 1.5f, 3.0f}, {-1.5f, 0.2f, -2.0f}, {1.5f, 0.2f, -2.0f}},
        .pointColor = {{0.4f, 0.6f, 0.85f}, {0.55f, 0.38f, 0.20f}, {0.2f, 0.35f, 0.5f}, {0.2f, 0.35f, 0.5f}},
        .pointRadius = {35.0f, 5.0f, 12.0f, 12.0f},
    };
}

SceneLighting LightingPreset_SpaceLobby(void) {
    // Grand station lobby — warm brass vs cool Earth blue
    // Half-Lambert note: ambient raised so hull walls read, key warmer for brass
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.7f, -0.4f}),
        .keyColor = {0.65f, 0.50f, 0.32f},          // warm brass chandelier — NOT blue
        .fillDir = Vector3Normalize((Vector3){0.2f, 0.6f, -0.5f}),
        .fillColor = {0.18f, 0.22f, 0.32f},          // cool Earth bounce from window — boosted
        .ambient = {0.20f, 0.21f, 0.25f},             // hull walls MUST read — space is never pitch black
        // Chandelier (warm) + side window accents (cool) + Earth glow (cold blue)
        .pointPos = {{0, 6.4f, -3.0f}, {-8, 3, 0}, {8, 3, 0}, {0, 0.5f, -6.0f}},
        .pointColor = {{0.9f, 0.65f, 0.35f}, {0.25f, 0.35f, 0.50f}, {0.25f, 0.35f, 0.50f}, {0.20f, 0.35f, 0.55f}},
        .pointRadius = {16.0f, 12.0f, 12.0f, 10.0f},
        .pointFlicker = {0.06f, 0.02f, 0.02f, 0},
        .pointPulse = {0.04f, 0.08f, 0.08f, 0.10f},
        .pointPhase = {0.8f, 2.5f, 3.8f, 0.5f},
    };
}

SceneLighting LightingPreset_SpaceCorridor(void) {
    // Narrow corridor — warm amber strips vs cold void blue from portholes
    // Kubrick hallway: warm pools at intervals, blue-black between
    // Half-Lambert note: wrap lighting + strong points = walls finally read
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, -0.9f, -0.2f}),    // steep overhead
        .keyColor = {0.8f, 0.65f, 0.40f},            // amber — moderated for half-Lambert
        .fillDir = Vector3Normalize((Vector3){0.5f, 0.3f, 0.0f}),
        .fillColor = {0.12f, 0.16f, 0.28f},           // porthole starlight — cooler
        .ambient = {0.10f, 0.10f, 0.16f},              // moderate — hull must read but not wash
        // Amber ceiling panels + porthole accent
        .pointPos = {{0, 2.0f, 0}, {0, 2.0f, 6}, {0, 2.0f, 12}, {-2, 1.5f, 12}},
        .pointColor = {{1.0f, 0.8f, 0.45f}, {0.9f, 0.7f, 0.40f}, {0.8f, 0.65f, 0.38f}, {0.25f, 0.38f, 0.70f}},
        .pointRadius = {18.0f, 18.0f, 18.0f, 12.0f},
        .pointFlicker = {0.05f, 0.05f, 0.05f, 0},
        .pointPulse = {0.03f, 0.03f, 0.03f, 0.06f},
        .pointPhase = {0.3f, 1.7f, 3.3f, 0.9f},
    };
}

SceneLighting LightingPreset_SpaceSuite(void) {
    // Hotel Chevalier in orbit.
    // The emotional equation: warm amber pools (intimacy) vs cold Earth blue (void).
    // Dark corners are FEATURES — they make the lit zones precious.
    // When you turn the lamp on, the room transforms. That's the arc.
    return (SceneLighting){
        // Key: Earth blue from window — the dominant mood, diagonal for long shadows
        .keyDir = Vector3Normalize((Vector3){-0.6f, -0.4f, -0.2f}),
        .keyColor = {0.25f, 0.38f, 0.55f},
        // Fill: warm amber bounce from floor/wood — the Hotel Chevalier glow
        .fillDir = Vector3Normalize((Vector3){0.2f, 0.6f, 0.1f}),
        .fillColor = {0.14f, 0.10f, 0.06f},
        // Ambient: very low but warm-tinted — corners visible but moody
        .ambient = {0.06f, 0.05f, 0.04f},
        // 0: Ceiling wash above bed — wide, warm, the safe zone
        // 1-2: Bedside lamps — warm intimate pools (the "twos")
        // 3: Earth glow — strong cool blue from window, the emotional anchor
        // 4: Floor lamp (off — interaction activates)
        // 5: Desk lamp (off — interaction activates)
        // 6: Pendant over coffee table — warm, creates living zone
        // 7: Entry sconce — dim warm, breadcrumb to orient
        .pointPos = {
            {0, 4.8f, -4.0f}, {-2.5f, 0.85f, -4.8f}, {2.5f, 0.85f, -4.8f}, {-6.5f, 2.5f, -0.5f},
            {-5.5f, 1.4f, -1.0f}, {6.0f, 0.6f, -1.5f}, {-3, 4.0f, 3.5f}, {3, 2.2f, 5.5f},
        },
        .pointColor = {
            {0.70f, 0.50f, 0.28f},   // ceiling: warm amber
            {0.55f, 0.40f, 0.22f},   // left bedside: intimate warm
            {0.55f, 0.40f, 0.22f},   // right bedside: matching
            {0.20f, 0.40f, 0.70f},   // Earth glow: STRONG cool blue — the anchor
            {0, 0, 0},               // floor lamp: off
            {0, 0, 0},               // desk lamp: off
            {0.35f, 0.25f, 0.12f},   // pendant: warm dim
            {0.20f, 0.15f, 0.08f},   // entry sconce: breadcrumb
        },
        .pointRadius = {12.0f, 5.0f, 5.0f, 14.0f, 0, 0, 6.0f, 4.0f},
        // Breathing — the room is alive. Lights flicker like real practicals.
        .pointFlicker = {0.04f, 0.15f, 0.15f, 0, 0, 0, 0.08f, 0.06f},
        .pointPulse = {0.08f, 0.04f, 0.04f, 0.12f, 0, 0, 0.06f, 0.03f},
        .pointPhase = {1.0f, 2.3f, 3.7f, 0.3f, 0, 0, 1.8f, 4.2f},
    };
}

SceneLighting LightingPreset_Hyperspace(void) {
    // Wormhole tunnel — rings glow, convergence point blazes
    // Multiple point lights along tunnel so rings are visible at every depth
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){0.0f, 0.0f, -1.0f}),
        .keyColor = {1.0f, 0.8f, 0.55f},        // pulled back — half-Lambert wraps the glow
        .fillDir = Vector3Normalize((Vector3){0.0f, -1.0f, 0.0f}),
        .fillColor = {0.30f, 0.35f, 0.45f},      // fill — moderate for half-Lambert
        .ambient = {0.22f, 0.20f, 0.26f},        // moderate — no dead black, wrap handles rest
        .pointPos = {{0, 0, -5.0f}, {0, 0, -15.0f}, {0, 0, -30.0f}, {0, 0, -50.0f}},
        .pointColor = {{1.2f, 1.0f, 0.65f}, {1.0f, 0.85f, 0.55f}, {0.8f, 0.7f, 0.45f}, {0.6f, 0.5f, 0.35f}},
        .pointRadius = {15.0f, 20.0f, 25.0f, 30.0f},
    };
}

SceneLighting LightingPreset_ParisDream(void) {
    // Hotel Chevalier golden hour — HOT light, DEEP shadows
    // The dream is saturated, contrasty — memory intensifies everything
    // Half-Lambert note: golden wraps beautifully but needs pull-back to keep contrast
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.5f, -0.7f, -0.3f}),
        .keyColor = {1.4f, 0.95f, 0.42f},        // golden — pulled back, still hot
        .fillDir = Vector3Normalize((Vector3){0.3f, 0.5f, 0.2f}),
        .fillColor = {0.05f, 0.04f, 0.02f},      // barely there
        .ambient = {0.03f, 0.025f, 0.015f},       // VERY LOW — dream shadows go deep
        .pointPos = {{0, 3.6f, 0}, {-2.5f, 0.85f, -3.8f}, {2.5f, 0.85f, -3.8f}},
        .pointColor = {{1.2f, 0.85f, 0.40f}, {0.7f, 0.50f, 0.28f}, {0.7f, 0.50f, 0.28f}},
        .pointRadius = {7.0f, 3.5f, 3.5f},
    };
}

SceneLighting LightingPreset_ReturnTaxi(void) {
    // Dawn Auckland — golden hour glow through windshield
    // Everything reads warmer and brighter than night. Circle closing.
    return (SceneLighting){
        .keyDir = Vector3Normalize((Vector3){-0.3f, -0.4f, -0.5f}),
        .keyColor = {1.2f, 0.85f, 0.55f},             // dawn golden — pulled back
        .fillDir = Vector3Normalize((Vector3){0.2f, -0.3f, 0.5f}),
        .fillColor = {0.25f, 0.22f, 0.30f},           // sky bounce — lilac dawn
        .ambient = {0.25f, 0.22f, 0.20f},              // moderate — dawn but not washed
        .pointPos = {{0.45f, 0.72f, -1.06f}, {0, 0.5f, -3.0f}, {0, 1.0f, 2.0f}},
        .pointColor = {{0.5f, 0.7f, 0.5f}, {0.6f, 0.45f, 0.28f}, {0.7f, 0.52f, 0.32f}},
        .pointRadius = {4.0f, 8.0f, 12.0f},
    };
}
