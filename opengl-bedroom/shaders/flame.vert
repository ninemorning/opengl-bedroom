#version 330 core
layout (location = 0) in vec2 aPos;

out vec2 UV;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 center;
uniform vec2 size;

void main()
{
    vec3 right = normalize(vec3(view[0][0], view[1][0], view[2][0]));
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 pos = center + right * aPos.x * size.x + up * aPos.y * size.y;
    UV = vec2(aPos.x + 0.5, aPos.y);
    gl_Position = projection * view * vec4(pos, 1.0);
}