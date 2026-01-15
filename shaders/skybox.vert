#version 410 core

layout (location = 0) in vec3 position;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = position;

    // remove translation from view matrix
    mat4 viewNoTranslate = mat4(mat3(view));

    vec4 pos = projection * viewNoTranslate * vec4(position, 1.0);
    gl_Position = pos.xyww; // force depth to far plane
}
