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
uniform vec2 resolution;

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
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    // bias to reduce shadow acne
    // Bias to reduce shadow acne. lightDirection points FROM the fragment TOWARDS the light.
    // Slightly larger bias to suppress "stripy" acne on big flat surfaces (ground).
    float bias = max(0.0035 * (1.0 - dot(n, lightDirection)), 0.0010);

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
    // lightDir is the direction FROM the fragment TOWARDS the sun
    vec3 sunDir = normalize(lightDir);

    // Fade sun contribution as it goes below the horizon, so the scene doesn't abruptly go black
    float sunStrength = smoothstep(-0.05, 0.25, sunDir.y);

    vec3 ambientSun = 0.25 * lightColor; // ambient floor

    float diffSun = max(dot(norm, sunDir), 0.0);
    vec3 diffuseSun = diffSun * lightColor * sunStrength;

    vec3 reflectSun = reflect(-sunDir, norm);
    float specSun = pow(max(dot(viewDir, reflectSun), 0.0), 32.0);
    vec3 specularSun = 0.3 * specSun * lightColor * sunStrength;

    float shadow = ShadowCalculation(FragPosLightSpace, norm, sunDir);
    shadow *= sunStrength;
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

    // Screen-space rain overlay (falls "through" the camera, not on object UVs)
    if (rainEnabled && resolution.x > 1.0 && resolution.y > 1.0) {
        // Screen-space UV (origin at bottom-left in OpenGL). To make streaks fall DOWN,
        // we scroll them toward smaller Y as time increases.
        vec2 uv = gl_FragCoord.xy / resolution;

        // column-based pseudo-random streaks
        float cols = 220.0;
        float colId = floor(uv.x * cols);
        float rnd = hash12(vec2(colId, 1.0));

        // slight wind drift
        float drift = (rnd - 0.5) * 0.10;
        uv.x += drift * (uv.y + time * 0.15);

        float speed = mix(1.6, 3.0, rnd);
        // To move streaks DOWN the screen as time increases, scroll the phase forward.
        // With OpenGL's bottom-left origin, this makes the bright "head" region appear
        // at progressively smaller uv.y.
        float y = fract(uv.y * 2.2 + time * speed + rnd);

        float x = fract(uv.x * cols);
        float center = 0.5 + (rnd - 0.5) * 0.25;
        float width = mix(0.015, 0.06, rnd);

        float streak = 1.0 - smoothstep(width, width + 0.02, abs(x - center));
        float head = smoothstep(0.0, 0.12, y) * (1.0 - smoothstep(0.12, 0.42, y));
        float drops = streak * head;

        vec3 rainTint = vec3(0.85, 0.9, 1.0);
        finalColor = mix(finalColor, finalColor + rainTint * drops, 0.22);
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
