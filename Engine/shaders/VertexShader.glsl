#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in mat4 instanceMatrix;

out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

uniform bool useInstancing;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec4 FragPosLightSpace;

void main()
{
    mat4 finalModel = model;
    
    if (useInstancing)
    {
        finalModel = model * instanceMatrix;
    }

    FragPos = vec3(finalModel * vec4(aPos, 1.0));
    Normal = mat3(finalModel) * aNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * finalModel * vec4(aPos, 1.0);
}