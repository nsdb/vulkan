#version 130

// vertex attributes
in vec3 position;
in vec3 normal;
in vec2 texcoord;

// matrices
uniform mat4 view_matrix;
uniform mat4 projection_matrix;
uniform mat4 model_matrix;

// outputs of vertex shader = input to fragment shader
out vec3 norm;  // per-vertex normal before interpolation
out vec2 tc;    // used for texture coordinate visualization
out vec4 epos;	// eye-coordinate position

// uniform variables
uniform float planet_radius;

void main()
{
	// apply radius
	vec3 regulated = position * planet_radius;

	// projection
	vec4 wpos = model_matrix * vec4(regulated, 1);
	epos = view_matrix * wpos;
	gl_Position = projection_matrix * epos;

	// pass eye-coordinate normal to fragment shader
	norm = normalize(mat3(view_matrix*model_matrix)*normal);
	tc = texcoord;

}
