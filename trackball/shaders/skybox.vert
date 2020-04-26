#version 450

// vertex attributes
in vec3 position;
in vec3 normal;

// matrices
uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 model_matrix;

// outputs of vertex shader = input to fragment shader
//out vec3 norm_sky;
//out vec3 epos_sky;
out vec3 texCoord;

void main()
{
    //norm_sky = mat3(transpose(inverse(model_matrix))) * normal;
    //epos_sky = vec3(model_matrix * vec4(position, 1.0));
	texCoord = position;

	// projection
	vec4 wpos = model_matrix * vec4(position, 1.0);
	vec4 epos = view_matrix * wpos;
	gl_Position = projection_matrix * epos;
}