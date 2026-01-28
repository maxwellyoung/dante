extern number time;
extern vec2 resolution;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

vec4 effect(vec4 color, Image tex, vec2 tc, vec2 sc) {
    vec4 original_color = Texel(tex, tc);
    
    // Vignette
    vec2 uv = tc - 0.5;
    float vignette = 1.0 - dot(uv, uv) * 0.8;
    vignette = pow(vignette, 1.5);
    
    // Film Grain
    float grain = random(sc / resolution.xy + mod(time, 1.0)) * 0.1;
    
    vec3 final_color = original_color.rgb * vignette - grain;
    
    return vec4(final_color, original_color.a);
} 