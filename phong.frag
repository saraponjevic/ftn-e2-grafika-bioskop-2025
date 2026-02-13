#version 330 core

struct Light{ //Svjetlosni izvor
	vec3 pos; //Pozicija
	vec3 kA; //Ambijentalna komponenta (Indirektno svjetlo)
	vec3 kD; //Difuzna komponenta (Direktno svjetlo)
	vec3 kS; //Spekularna komponenta (Odsjaj)
     int enabled;
};
struct Material{ //Materijal objekta
	vec3 kA;
	vec3 kD;
	vec3 kS;
	float shine; //Uglancanost
};

in vec3 chNor;
in vec3 chFragPos;

out vec4 outCol;

//uniform Light uLight;
uniform Light uLights[2];

uniform Material uMaterial;
uniform vec3 uViewPos;	//Pozicija kamere (za racun spekularne komponente)

void main()
{
    vec3 normal = normalize(chNor);
    vec3 viewDirection = normalize(uViewPos - chFragPos);

    vec3 res = vec3(0.0);

    for(int i = 0; i < 2; i++)
    {

     if(uLights[i].enabled == 0) continue; 
        // Ambient
        vec3 resA = uLights[i].kA * uMaterial.kA;

        // Diffuse
        vec3 lightDirection = normalize(uLights[i].pos - chFragPos);
        float nD = max(dot(normal, lightDirection), 0.0);
        vec3 resD = uLights[i].kD * (nD * uMaterial.kD);

        // Specular
        vec3 reflectionDirection = reflect(-lightDirection, normal);
        float s = pow(max(dot(viewDirection, reflectionDirection), 0.0), uMaterial.shine);
        vec3 resS = uLights[i].kS * (s * uMaterial.kS);

        res += (resA + resD + resS);
    }

    outCol = vec4(res, 1.0);
}
