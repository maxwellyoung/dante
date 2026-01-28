//
// Refraction shader
//
// This shader distorts the screen based on a "normal map" texture.
// We can use this for water, heat haze, or other weird visual effects.
//

// The vertex shader is standard. It passes through the vertex data.
#ifdef VERTEX
vec4 position(mat4 transform_projection, vec4 vertex_position) {
    return transform_projection * vertex_position;
}
#endif

// The fragment shader does the distortion.
#ifdef PIXEL
extern Image NormalMap; // A texture with our distortion vectors
extern float distortion_strength;
extern number time;

vec4 effect(vec4 color, Image texture, vec2 texture_coords, vec2 screen_coords) {
    // Animate the distortion map coordinates over time
    vec2 scroll_coords = screen_coords / love_ScreenSize.xy;
    scroll_coords.x += time * 0.05;
    scroll_coords.y += time * 0.03;

    // Get the distortion vector from the normal map
    vec2 distortion = Texel(NormalMap, scroll_coords).rg;
    
    // The values in the normal map are from 0-1, so we shift them to be -1 to 1
    distortion = (distortion - 0.5) * 2.0;

    // Apply the distortion to our texture coordinates
    vec2 distorted_coords = texture_coords + distortion * distortion_strength;

    // Grab the pixel from the original screen texture at the distorted coordinates
    vec4 final_color = Texel(texture, distorted_coords);
    
    return final_color;
}
#endif 