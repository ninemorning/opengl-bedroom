#version 330 core
in float vAlpha;

out vec4 FragColor;

void main()
{
    vec2 c = gl_PointCoord * 2.0 - 1.0;
    float d = dot(c, c);
    if (d > 1.0)
        discard;
    float a = (1.0 - d) * vAlpha * 0.9;
    FragColor = vec4(0.85, 0.9, 1.0, a);
}