#version 330 core

// Segments as vec4(ax, ay, bx, by) in NDC
uniform samplerBuffer u_segments;  // TBO (GL_RGBA32F)
uniform int   u_count;
uniform vec2  u_resolution;        // window size in pixels

out vec4 FragColor;

float isLeft(vec2 a, vec2 b, vec2 p) {
    return (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
}

// Non-zero winding number on a ray to +X
int windingNumber(vec2 p_ndc) {
    int wn = 0;
    for (int i = 0; i < u_count; ++i) {
        vec4 s = texelFetch(u_segments, i);
        vec2 a = s.xy, b = s.zw;
        if (a.y <= p_ndc.y && b.y >  p_ndc.y && isLeft(a,b,p_ndc) > 0.0) wn += 1; // upward
        if (a.y >  p_ndc.y && b.y <= p_ndc.y && isLeft(a,b,p_ndc) < 0.0) wn -= 1; // downward
    }
    return wn;
}

// Even-odd parity (ray-crossing)
bool insideEvenOdd(vec2 p_ndc) {
    bool odd = false;
    for (int i = 0; i < u_count; ++i) {
        vec4 s = texelFetch(u_segments, i);
        vec2 a = s.xy, b = s.zw;
        bool straddles = ((a.y > p_ndc.y) != (b.y > p_ndc.y));
        if (straddles) {
            float xint = a.x + (p_ndc.y - a.y) * (b.x - a.x) / (b.y - a.y);
            if (xint > p_ndc.x) odd = !odd;
        }
    }
    return odd;
}

void main() {
    vec2 uv   = gl_FragCoord.xy / u_resolution; // 0..1
    vec2 pNDC = uv * 2.0 - 1.0;                 // -1..+1

    int  wn       = windingNumber(pNDC);
    bool eoInside = insideEvenOdd(pNDC);

    // Green if NZW says inside; Red if EvenOdd says inside; Yellow if both.
    vec3 col = vec3(0.0);
    if (wn != 0)    col += vec3(0.0, 1.0, 0.0);
    if (eoInside)   col += vec3(1.0, 0.0, 0.0);
    FragColor = vec4(col, 1.0);
}
