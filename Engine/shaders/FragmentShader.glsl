#version 430 core

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;
in vec4 FragPosLightSpace;
in vec3 InstanceColor;

#define MAX_LIGHTS_COUNT 8

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float shininess;
};

struct Light 
{
    int type;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;

    // point light
    float radius;

    // spot light
    float cutOff;
    float outCutOff;
};


uniform Light     lights[MAX_LIGHTS_COUNT];
uniform int       lightsCount;
uniform Material  material;

uniform sampler2D shadowMap;
uniform sampler2D texture_diffuse1;

uniform vec3 viewPos;

uniform bool wireframe;
uniform bool textured;
uniform bool useInstanceColor;

float GetShadowFactor(vec4 fragPosLightSpace, vec3 lightDir, vec3 normal)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r;
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    float shadow = 0.0;

    if (projCoords.z > 1.0)
        return shadow;

    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // reduce shadow darkness to avoid black spots
    shadow = clamp(shadow * 0.8, 0.0, 1.0);

    return shadow;
}

vec3 ComputeDirectionalLighting(Light light, vec3 ambientColor, vec3 surfaceColor)
{
    // ambiant lighting
    vec3 ambient = light.color * ambientColor;

    // diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * surfaceColor;

    // specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, reflectDir), 0.0), material.shininess * 128);

    vec3 specular = spec * light.color * material.specular;

    float lightAngleT = clamp((light.direction.y + 1.0) * 0.5, 0.0, 1.0);
    float shadow = GetShadowFactor(FragPosLightSpace, lightDir, norm);

    return mix((ambient + (1.0 - shadow) * (diffuse + specular)) * light.intensity, vec3(0), lightAngleT);
}

vec3 ComputePointLighting(Light light, vec3 ambientColor, vec3 surfaceColor)
{
    float distance = length(light.position - FragPos);
    float radius = light.radius;
    float constant = 1.0;
    float linear = 2.0 / (radius * 0.7);
    float quadratic = 1.0 / (radius * radius);
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);

    // ambiant lighting
    vec3 ambient = light.color * ambientColor * attenuation;

    // diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * surfaceColor * attenuation;

    // specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, reflectDir), 0.0), material.shininess * 128);

    vec3 specular = spec * light.color * material.specular * attenuation;

    return (ambient + diffuse + specular) * light.intensity;
}

vec3 ComputeSpotLighting(Light light, vec3 ambientColor, vec3 surfaceColor)
{
    vec3 lightDir = normalize(light.position - FragPos);
    float theta = dot(lightDir, normalize(-light.direction));

    // outerCutOff is used to make a fade edge of the spot
    float epsilon = light.cutOff - light.outCutOff;
    float intensity = clamp((theta - light.outCutOff) / epsilon, 0.0, 1.0);

    // attenuation
    float distance = length(light.position - FragPos);
    float constant = 1.0;
    float linear = 0.09;
    float quadratic = 0.032;
    float attenuation = 1.0 / (constant + linear * distance + quadratic * distance * distance);


    // ambiant lighting
    vec3 ambient = light.color * ambientColor * attenuation * intensity;

    // diffuse lighting
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * light.color * surfaceColor * intensity * attenuation;

    // specular lighting
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(halfwayDir, reflectDir), 0.0), material.shininess * 128);
    vec3 specular = spec * light.color * material.specular * intensity * attenuation;

    return (ambient + diffuse + specular) * light.intensity;
}

vec3 ComputeLighting(Light light, vec3 ambientColor, vec3 surfaceColor)
{
    if (light.type == 0)
    {
        return ComputeDirectionalLighting(light, ambientColor, surfaceColor);
	}
    else if (light.type == 1)
    {
		return ComputePointLighting(light, ambientColor, surfaceColor);
	}
    else if (light.type == 2)
    {
		return ComputeSpotLighting(light, ambientColor, surfaceColor);
    }

    return vec3(0.0);
}

void main()
{
    vec3 surfaceColor = material.diffuse;
    vec3 ambientColor = material.ambient;

    if (useInstanceColor)
    {
        surfaceColor = InstanceColor;
        ambientColor = InstanceColor;
    }
    else if (textured)
    {
        surfaceColor = texture(texture_diffuse1, TexCoords).rgb;
        ambientColor = surfaceColor;
    }

    if (!wireframe)
    {
        vec3 computedLight = vec3(0.0);

        for (int i = 0; i < lightsCount; i++)
        {
            computedLight += ComputeLighting(lights[i], ambientColor, surfaceColor);
        }
        
        FragColor = vec4(computedLight, 1.0);
    }
    else if (textured)
    {
		FragColor = texture(texture_diffuse1, TexCoords);
	}
    else if (useInstanceColor)
    {
        FragColor = vec4(InstanceColor, 1.0);
    }
	else
    {
        FragColor = vec4(material.diffuse, 1);
    }
}
