#version 410 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec4 FragPosLightSpace;

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
uniform sampler2D shadowMap;

// optional animated effect (toggle with Y)
uniform float time;
uniform bool rainEnabled;

float hash12(vec2 p) {
    // cheap deterministic noise
    vec3 p3  = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 n, vec3 lightDirection)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1]
    projCoords = projCoords * 0.5 + 0.5;

    // outside the shadow map
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // bias to reduce shadow acne
    float bias = max(0.0015 * (1.0 - dot(n, -lightDirection)), 0.0005);

    // simple PCF (3x3)
    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

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

    float shadow = ShadowCalculation(FragPosLightSpace, norm, sunDir);
    vec3 sunResult = ambientSun + (1.0 - shadow) * (diffuseSun + specularSun);

    // Point Light (lamp)
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
    vec3 finalColor = lighting * texColor;

    // Simple "rain streak" overlay in texture space (cheap but animated)
    if (rainEnabled) {
        vec2 uv = TexCoords;
        uv.y += time * 2.5; // move drops downward

        float colId = floor(uv.x * 60.0);
        float rnd = hash12(vec2(colId, floor(uv.y * 10.0)));
        float streak = smoothstep(0.97, 1.0, rnd);
        float line = 1.0 - smoothstep(0.0, 0.02, abs(fract(uv.x * 60.0) - 0.5));
        float drops = streak * line;

        finalColor = mix(finalColor, finalColor * 0.85, 0.35);
        finalColor += vec3(drops) * 0.25;
    }

    // FOG
    if (fogEnabled) {
        float distanceFog = length(viewPos - FragPos);
        float fogFactor = exp(-pow(distanceFog * fogDensity, 2.0));
        fogFactor = clamp(fogFactor, 0.0, 1.0);
        finalColor = mix(fogColor, finalColor, fogFactor);
    }

    FragColor = vec4(finalColor, 1.0);
}
