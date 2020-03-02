#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler[2]; // 0: Color, 1: Alpha

layout(binding = 2) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
	vec4 light;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
	bool applyLight;
} ubo;

layout(location = 0) in vec4 epos; // eye-coordinate position
layout(location = 1) in vec3 norm; // per-vertex normal before interpolation
layout(location = 2) in vec2 tc;   // used for texture coordinate visualization

// Light 적용 이전
// layout(location = 0) in vec3 fragColor;
// layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {

	if(ubo.applyLight) {
	
		// 빛 적용한 텍스쳐 색상
		vec4 lpos = ubo.view * ubo.light;                             // light position in the eye-space coordinate

		vec3 n = normalize(norm);                                     // norm interpolated via rasterizer should be normalized again here
		vec3 p = epos.xyz;                                            // 3D position of this fragment
		vec3 l = normalize(lpos.xyz - (lpos.a == 0.0 ? vec3(0) : p)); // lpos.a==0 means directional light
		vec3 v = normalize(-p);                                       // eye-epos = vec3(0)-epos
		vec3 h = normalize(l + v);                                    // the halfway vector

		vec4 light_texture = texture(texSampler[0], tc);
		vec4 Ira = light_texture * ubo.ambient;                                         // ambient reflection
		vec4 Ird = max(light_texture * dot(l, n) * ubo.diffuse, 0.0);                   // diffuse reflection
		vec4 Irs = max(light_texture * pow(dot(h, n), ubo.shininess) * ubo.specular, 0.0);  // specular reflection

		outColor = Ira + Ird + Irs;

	} else {

		// 텍스쳐 색상
		outColor = texture(texSampler[0], tc);

	}





	// 텍스쳐 Coordinate 색상
	// outColor = vec4(norm, 0.0, 1.0);

	// 텍스쳐와 기본색상 혼합
    // outColor = vec4(fragColor * texture(texSampler[0], norm).rgb, 1.0);

	// 알파 텍스쳐
	vec4 alpha_texture = texture(texSampler[1], tc);
	outColor.a = alpha_texture.r;

}