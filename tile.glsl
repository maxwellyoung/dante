//
// Tile Shader
//
// Gives tiles a simple, clean, 3D-ish look with gradients and highlights.
//

#ifdef VERTEX
vec4 position(mat4 transform_projection, vec4 vertex_position) {
    return transform_projection * vertex_position;
}
#endif

#ifdef PIXEL
extern vec3 base_color; // The tile's main color
extern vec2 tile_pos; // The top-left corner of the tile on the screen

float noise(vec2 uv) {
    return fract(sin(dot(uv, vec2(12.9898, 78.233))) * 43758.5453);
}

float smooth_noise(vec2 uv) {
    vec2 i = floor(uv);
    vec2 f = fract(uv);
    f = f * f * (3.0 - 2.0 * f);
    
    float a = noise(i);
    float b = noise(i + vec2(1.0, 0.0));
    float c = noise(i + vec2(0.0, 1.0));
    float d = noise(i + vec2(1.0, 1.0));
    
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

vec4 effect(vec4 color, Image texture, vec2 texture_coords, vec2 screen_coords) {
    vec2 uv = texture_coords;
    vec2 world_pos = tile_pos + texture_coords * 32.0; // Assuming 32px tile size
    
    // Create subtle texture variation
    float texture_noise = smooth_noise(world_pos * 0.1) * 0.15;
    
    // Create depth/lighting effect based on position
    float depth_factor = (1.0 - uv.y) * 0.2; // Lighter at top
    float edge_lighting = 1.0 - smoothstep(0.0, 0.1, min(min(uv.x, 1.0 - uv.x), min(uv.y, 1.0 - uv.y)));
    
    // Combine effects
    vec3 final_color = base_color;
    final_color += texture_noise;
    final_color += depth_factor;
    final_color += edge_lighting * 0.3;
    
    // Add subtle parallax shift based on camera
    float parallax = sin(world_pos.x * 0.02 + world_pos.y * 0.03) * 0.05;
    final_color += parallax;

    return vec4(final_color, 1.0);
}
#endif 