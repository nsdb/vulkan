#version 130


// input from vertex shader
in vec4 epos; // eye-coordinate position
in vec3 norm; // per-vertex normal before interpolation
in vec2 tc;	  // used for texture coordinate visualization

// the only output variable
out vec4 fragColor;

// uniform variables
uniform sampler2D TEX;                      // texture
uniform sampler2D TEX_ALPHA;                // alpha texture
uniform mat4	view_matrix;
uniform vec4	light_position, Ia, Id, Is;	// light
uniform float	shininess;
uniform bool    use_shader;                 // shader use check
uniform bool    use_alpha_tex;              // alpha texture use check


void main()
{
	if(use_shader) {

		// light position in the eye-space coordinate
		vec4 lpos = view_matrix*light_position;

		vec3 n = normalize(norm);	// norm interpolated via rasterizer should be normalized again here
		vec3 p = epos.xyz;			// 3D position of this fragment
		vec3 l = normalize(lpos.xyz-(lpos.a==0.0?vec3(0):p));	// lpos.a==0 means directional light
		vec3 v = normalize(-p);		// eye-epos = vec3(0)-epos
		vec3 h = normalize(l+v);	// the halfway vector

		vec4 texture = texture2D( TEX, tc );
		vec4 Ira = texture*Ia;									// ambient reflection
		vec4 Ird = max(texture*dot(l,n)*Id,0.0);					// diffuse reflection
		vec4 Irs = max(texture*pow(dot(h,n),shininess)*Is,0.0);	// specular reflection

		fragColor = Ira + Ird + Irs;
		

	} else {

		fragColor = texture2D( TEX, tc );

	}

	if(use_alpha_tex) {

		// alpha texture image is grayscale - so texture.r, .g, .b are equal
		vec4 alpha_texture = texture2D( TEX_ALPHA, tc );
		fragColor.a = alpha_texture.r;

	}

}
