#version 450

// input from vertex shader
//in vec3 norm_sky;
//in vec3 epos_sky;
in vec3 texCoord;

// the only output variable
out vec4 fragColor;

// uniform variables
//uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{             
    //vec3 I = normalize(epos_sky - cameraPos);
    //vec3 R = reflect(I, normalize(norm_sky));
    //fragColor = vec4(texture(skybox, R).rgb, 1.0);

	fragColor = texture(skybox, normalize(texCoord));
	//fragColor = vec4(normalize(texCoord), 1.0);
	//fragColor = vec4(normalize(texCoord), 1.0) * 0.5 + texture(skybox, normalize(texCoord)) * 0.5;
}





