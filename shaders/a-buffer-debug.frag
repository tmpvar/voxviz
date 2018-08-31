#version 420 core
#extension GL_EXT_shader_image_load_store : enable

coherent uniform layout(size4x32,binding=3) image2DArray aBuffer;

uniform int slice;
in vec2 uv;
out vec4 outColor;

void main() {
	memoryBarrier();
	vec4 val = imageLoad(aBuffer, ivec3(gl_FragCoord.xy, slice));
	
	outColor = val;
}
