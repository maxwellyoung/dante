// vignette.glsl
extern number radius = 0.7;
extern number softness = 0.4;
extern vec2 center = vec2(0.5, 0.5);

vec4 effect(vec4 color, Image texture, vec2 texture_coords, vec2 screen_coords) {
    // Combine texture color with vertex color for compatibility
    vec4 original_color = Texel(texture, texture_coords) * color;

    float distance = distance(screen_coords, center);
    float vignette = smoothstep(radius, radius - softness, distance);

    return vec4(original_color.rgb * vignette, original_color.a);
} 