#version 330 core
layout (location = 0) in vec3 aStart;
layout (location = 1) in vec2 aParams;

out float vAlpha;

uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float baseY;
uniform float height;
uniform float pointScale;

void main()
{
    float speed = aParams.x;
    float phase = aParams.y;
    float y = baseY + mod(aStart.y - speed * time, height);
    float x = aStart.x + sin(time * 0.8 + phase) * 0.3;
    float z = aStart.z + cos(time * 0.6 + phase * 1.3) * 0.2;
    vec4 viewPos = view * vec4(x, y, z, 1.0);
    gl_Position = projection * viewPos;
    float size = 0.035 + 0.03 * fract(phase * 7.13);
    gl_PointSize = pointScale * size / max(-viewPos.z, 0.1);
    vAlpha = clamp((y - baseY) / 0.5, 0.0, 1.0);
}