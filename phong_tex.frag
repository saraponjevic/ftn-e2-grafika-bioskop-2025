#version 330 core

struct Light {
    vec3 pos;
    vec3 kA;
    vec3 kD;
    vec3 kS;
     int enabled;
};
struct Material{
    vec3 kA;
    vec3 kD;
    vec3 kS;
    float shine;
};

in vec3 chNor;
in vec3 chFragPos;
in vec2 chUV;

out vec4 outCol;

uniform Light uLights[2];
uniform Material uMaterial;
uniform vec3 uViewPos;

uniform sampler2D uTex;
//uniform float uTiling; // koliko puta se ponavlja tekstura

uniform float uEmissive; 

void main() {
    vec3 normal = normalize(chNor);
    vec3 viewDirection = normalize(uViewPos - chFragPos);

    vec3 texCol = texture(uTex, chUV).rgb;


    vec3 res = vec3(0.0);
    for(int i=0;i<2;i++){
     if(uLights[i].enabled == 0) continue; 
        vec3 resA = uLights[i].kA * (uMaterial.kA * texCol);

        vec3 lightDirection = normalize(uLights[i].pos - chFragPos);
        float nD = max(dot(normal, lightDirection), 0.0);
        vec3 resD = uLights[i].kD * (nD * (uMaterial.kD * texCol));

        vec3 reflectionDirection = reflect(-lightDirection, normal);
        float s = pow(max(dot(viewDirection, reflectionDirection), 0.0), uMaterial.shine);
        vec3 resS = uLights[i].kS * (s * uMaterial.kS);

        res += (resA + resD + resS);
    }
    res += texCol * uEmissive;
outCol = vec4(res, 1.0);

 outCol = vec4(res, 1.0);

}
