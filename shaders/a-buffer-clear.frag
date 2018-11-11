#version 420 core

#extension GL_ARB_shader_image_load_store : enable
#extension GL_EXT_bindable_uniform : enable
#extension GL_EXT_shader_image_load_store : enable

#include "../include/core.h"

coherent uniform layout(size4x32,binding=3) image2DArray aBuffer;
coherent uniform layout(size1x32,binding=4) uimage2D aBufferIndex;

void main() {
	ivec2 coords = ivec2(gl_FragCoord.xy);
	imageStore(aBufferIndex, coords, uvec4(0));
	memoryBarrier();
	for (int i=0; i<ABUFFER_MAX_DEPTH_COMPLEXITY; i++) {
		imageStore(aBuffer, ivec3(coords, i), vec4(0.0));
		memoryBarrier();
	}
	discard;
}