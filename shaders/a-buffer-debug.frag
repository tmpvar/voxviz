#version 330 core
#extension GL_EXT_shader_image_load_store : enable

uniform layout(size4x32) image2DArray aBuffer;

uniform float id;
in vec2 uv;
out vec4 outColor;

void main() {
	vec4 val = imageLoad(aBuffer, ivec3(gl_FragCoord.xy, 0));
	outColor = val;
}
