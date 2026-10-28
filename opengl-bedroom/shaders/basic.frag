#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

out vec4 FragColor;

uniform sampler2D tex;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main()
{
    vec3 color = texture(tex, UV).rgb;
    vec3 n = normalize(Normal);
    vec3 l = normalize(lightPos - FragPos);
    vec3 v = normalize(viewPos - FragPos);
    vec3 h = normalize(l + v);
    float dist = length(lightPos - FragPos);
    float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
    vec3 ambient = 0.15 * color;
    vec3 diffuse = max(dot(n, l), 0.0) * color * lightColor;
    vec3 specular = pow(max(dot(n, h), 0.0), 32.0) * 0.2 * lightColor;
    FragColor = vec4(ambient + (diffuse + specular) * att, 1.0);
}