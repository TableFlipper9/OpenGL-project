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

// optional animated effect
uniform float time;
uniform bool rainEnabled;
uniform vec2 resolution;

// camera matrices are already set for the vertex shader; we also use them here
uniform mat4 view;
uniform mat4 projection;

float hash12(vec2 p) {
    // cheap deterministic noise
    vec3 p3  = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec2 hash22(vec2 p) {
    float n = hash12(p);
    return vec2(n, hash12(p + n + 17.17));
}

mat2 rot2(float a) {
    float s = sin(a), c = cos(a);
    return mat2(c, -s, s, c);
}

float ShadowCalculation(vec4 fragPosLightSpace, vec3 n, vec3 lightDirection)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    // outside the shadow map
    if (projCoords.z > 1.0) return 0.0;
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) return 0.0;

    float currentDepth = projCoords.z;

    float bias = max(0.0015 * (1.0 - dot(n, lightDirection)), 0.0005);

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
    vec3 sunDir = normalize(lightDir);

    // Fade sun contribution as it goes below the horizon, so that the scene doesn't abruptly go black
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

    // Screen-space rain 
    if (rainEnabled) {
        vec3 rainDirWorld = normalize(vec3(0.15, -1.0, 0.10)); // wind + gravity (points downward)
        vec3 rainTint = vec3(0.85, 0.90, 1.0);

        float L = 2.0; // world-space step for projection (small but not tiny)
        vec4 clip0 = projection * view * vec4(FragPos, 1.0);
        vec4 clip1 = projection * view * vec4(FragPos + rainDirWorld * L, 1.0);
        vec2 ndc0 = clip0.xy / max(clip0.w, 1e-6);
        vec2 ndc1 = clip1.xy / max(clip1.w, 1e-6);

        vec2 dir2 = ndc1 - ndc0;
        float dirLen = length(dir2);
        vec2 rainDirSS = (dirLen > 1e-5) ? (dir2 / dirLen) : vec2(0.0, -1.0);
        vec2 rainPerpSS = vec2(-rainDirSS.y, rainDirSS.x);

        vec2 uv = (gl_FragCoord.xy / max(resolution, vec2(1.0))) * 2.0 - 1.0;

        float along = dot(uv, rainDirSS);
        float across = dot(uv, rainPerpSS);

        float lanes = 99.0; 
        float laneCoord = across * lanes;
        float laneId = floor(laneCoord);
        float laneRnd = hash12(vec2(laneId, 91.7));

        // width varies per lane
        float width = mix(0.035, 0.08, laneRnd);
        float x = abs(fract(laneCoord + laneRnd * 3.0) - 0.5);
        float core = 1.0 - smoothstep(width, width + 0.02, x);

        // animate streak segments along the direction
        float speed = mix(1.2, 2.4, laneRnd);
        float phase = fract(along * 1.7 - time * speed + laneRnd);
        float head = smoothstep(0.0, 0.08, phase) * (1.0 - smoothstep(0.10, 0.75, phase));

        // streaks should collapse toward dots.
        float dotness = smoothstep(0.12, 0.02, dirLen);
        float rainMask = core * head;

        if (dotness > 0.001) {
            // Dots in screen space
            vec2 p = uv * 18.0;
            vec2 id = floor(p);
            vec2 f = fract(p);
            vec2 rnd = hash22(id);
            vec2 c = rnd;
            float d = length(f - c);
            float sp = mix(0.6, 1.4, rnd.x);
            float tt = fract(time * sp + rnd.y);
            float life = smoothstep(0.0, 0.12, tt) * (1.0 - smoothstep(0.35, 0.9, tt));
            float drop = 1.0 - smoothstep(0.08, 0.14, d);
            float dots = drop * life;
            rainMask = mix(rainMask, dots, dotness);
        }

        // Subtle additive wet
        finalColor = mix(finalColor, finalColor + rainTint * rainMask, 0.22);
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
