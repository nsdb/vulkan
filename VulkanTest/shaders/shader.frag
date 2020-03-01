#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 1) uniform sampler2D texSampler[2]; // 0: Color, 1: Alpha

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {

	// �ؽ��� Coordinate ����
	// outColor = vec4(fragTexCoord, 0.0, 1.0);

	// �ؽ��Ŀ� �⺻���� ȥ��
    // outColor = vec4(fragColor * texture(texSampler[0], fragTexCoord).rgb, 1.0);

	// �ؽ��� ����
	outColor = texture(texSampler[0], fragTexCoord);

	// ���� �ؽ���
	vec4 alpha_texture = texture(texSampler[1], fragTexCoord);
	outColor.a = alpha_texture.r;
}