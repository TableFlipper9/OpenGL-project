#version 410 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 FragColor;

uniform vec3 lightDir;     // sun (directional)
uniform vec3 lightColor;

uniform vec3 lampPos;      // point light
uniform vec3 lampColor;
uniform bool lampEnabled;

uniform vec3 fogColor;     // fog
uniform float fogDensity;
uniform bool fogEnabled;

uniform vec3 viewPos;
uniform sampler2D diffuseTexture;


void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // Directional Light
    vec3 sunDir = normalize(-lightDir);

    vec3 ambientSun = 0.2 * lightColor;

    float diffSun = max(dot(norm, sunDir), 0.0);
    vec3 diffuseSun = diffSun * lightColor;

    vec3 reflectSun = reflect(-sunDir, norm);
    float specSun = pow(max(dot(viewDir, reflectSun), 0.0), 32.0);
    vec3 specularSun = 0.3 * specSun * lightColor;

    vec3 sunResult = ambientSun + (diffuseSun + specularSun);

    //  (Point Light
    vec3 lampDir = normalize(lampPos - FragPos);
    float distLamp = length(lampPos - FragPos);

    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
    float attenuation = 1.0 /
        (constant + linear * distLamp + quadratic * distLamp * distLamp);

    vec3 ambientLamp = 0.1 * lampColor;

    float diffLamp = max(dot(norm, lampDir), 0.0);
    vec3 diffuseLamp = diffLamp * lampColor;

    vec3 reflectLamp = reflect(-lampDir, norm);
    float specLamp = pow(max(dot(viewDir, reflectLamp), 0.0), 32.0);
    vec3 specularLamp = 0.5 * specLamp * lampColor;

    vec3 lampResult = vec3(0.0);
    if (lampEnabled) {
        lampResult = (ambientLamp + diffuseLamp + specularLamp) * attenuation;
    }

    // TEXTURE
    vec3 lighting = sunResult + lampResult;
    vec3 texColor = texture(diffuseTexture, TexCoords).rgb;

    vec3 shadedColor = lighting * texColor;

    // FOG
    float distanceFog = length(viewPos - FragPos);
    float fogFactor = exp(-pow(distanceFog * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = shadedColor;

    if (fogEnabled) {
        finalColor = mix(fogColor, shadedColor, fogFactor);
    }

    FragColor = vec4(finalColor, 1.0);
}
