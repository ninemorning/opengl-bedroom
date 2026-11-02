#version 330 core
#define NUM_LIGHTS 7

in vec3 FragPos;
in vec3 Normal;
in vec2 UV;

out vec4 FragColor;

uniform sampler2D tex;
uniform vec3 lightPos[NUM_LIGHTS];
uniform vec3 lightColor[NUM_LIGHTS];
uniform float lightIntensity[NUM_LIGHTS];
uniform vec3 viewPos;
uniform float ambient;
uniform vec3 emissive;
uniform float alpha;
uniform float exposure;

vec3 pointLight(int i, vec3 color, vec3 n, vec3 v)
{
    vec3 l = normalize(lightPos[i] - FragPos);
    vec3 h = normalize(l + v);
    float dist = length(lightPos[i] - FragPos);
    float att = 1.0 / (1.0 + 0.09 * dist + 0.032 * dist * dist);
    vec3 diffuse = max(dot(n, l), 0.0) * color;
    vec3 specular = pow(max(dot(n, h), 0.0), 32.0) * 0.15 * color;
    return (diffuse + specular) * lightColor[i] * lightIntensity[i] * att;
}

void main()
{
    vec3 color = texture(tex, UV).rgb;
    vec3 n = normalize(Normal);
    vec3 v = normalize(viewPos - FragPos);
    vec3 result = ambient * color;
    for (int i = 0; i < NUM_LIGHTS; i++)
        result += pointLight(i, color, n, v);
    result += emissive;
    vec3 mapped = vec3(1.0) - exp(-result * exposure);
    FragColor = vec4(mapped, alpha);
}