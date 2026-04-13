#version 430 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

// instancing data
layout(location = 3) in vec4 instanceData; // position + radius
layout(location = 4) in vec3 instanceColor;

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
out vec3 InstanceColor;

void main()
{
    vec4 localPosition = vec4(aPos, 1.0);
    
    // used in particle based fluid simulation context
    if (useInstancing)
    {
        localPosition = vec4(aPos * instanceData.w + instanceData.xyz, 1.0);
    }

    FragPos = vec3(model * localPosition);
    Normal = mat3(model) * aNormal;
    TexCoords = aTexCoords;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    InstanceColor = instanceColor;
    gl_Position = projection * view * model * localPosition;
}
