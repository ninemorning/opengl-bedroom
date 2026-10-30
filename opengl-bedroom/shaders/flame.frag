#version 330 core
in vec2 UV;

out vec4 FragColor;

uniform float time;
uniform float seed;
uniform float intensity;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x), u.y);
}

float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 4; i++)
    {
        v += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

void main()
{
    float n = fbm(vec2(UV.x * 3.0 + seed, UV.y * 2.5 - time * 2.5 + seed));
    float x = (UV.x - 0.5) * 2.0 + (n - 0.5) * 0.6 * UV.y;
    float width = (1.0 - UV.y) * (0.6 + 0.5 * n);
    float body = 1.0 - smoothstep(0.0, width + 0.001, abs(x));
    body *= smoothstep(0.0, 0.08, UV.y);
    body *= 1.0 - smoothstep(0.55, 1.0, UV.y + (n - 0.5) * 0.4);
    vec3 col = mix(vec3(0.8, 0.15, 0.02), vec3(1.0, 0.85, 0.35), body * body);
    col = mix(col, vec3(1.0, 0.95, 0.7), pow(body, 4.0));
    FragColor = vec4(col * body * intensity, 1.0);
}