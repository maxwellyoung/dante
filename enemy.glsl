extern number time;
extern vec3 base_color;

float noise(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

float pnoise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = noise(i);
    float b = noise(i + vec2(1.0, 0.0));
    float c = noise(i + vec2(0.0, 1.0));
    float d = noise(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

vec4 effect(vec4 color, Image tex, vec2 tc, vec2 sc) {
    vec2 uv = tc - 0.5;
    float dist = length(uv);
    
    float turbulence = pnoise(uv * 4.0 + time * 0.5) * 0.2;
    float pulse = (sin(time * 3.0 + dist * 20.0) + 1.0) / 2.0;
    pulse = pow(pulse, 3.0);
    
    float combined = smoothstep(0.45, 0.5, dist - turbulence + pulse * 0.1);
    
    vec3 final_color = base_color + vec3(pulse * 0.4, pulse * 0.2, 0.0);
    
    return vec4(final_color, 1.0 - combined);
} 