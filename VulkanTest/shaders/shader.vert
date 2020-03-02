#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
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

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNorm;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec4 epos;	  // eye-coordinate position
layout(location = 1) out vec3 norm;   // per-vertex normal before interpolation
layout(location = 2) out vec2 tc;     // used for texture coordinate visualization

// Light 적용 이전
// layout(location = 0) out vec3 fragColor;
// layout(location = 1) out vec2 fragTexCoord;

void main() {

	epos = ubo.view * ubo.model * vec4(inPosition, 1.0);
	gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);

	// pass eye-coordinate normal to fragment shader
	norm = normalize(mat3(ubo.view * ubo.model) * inNorm);
	tc = inTexCoord;

	// Light 적용 이전
    // gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);
    // fragColor = inNorm;
    // fragTexCoord = inTexCoord;



}