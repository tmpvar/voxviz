#version 420 core
#extension GL_ARB_shader_image_load_store : enable
#extension GL_EXT_bindable_uniform : enable
#extension GL_EXT_shader_image_load_store : enable

#include "../include/core.h"

coherent uniform layout(size4x32,binding=3) image2DArray aBuffer;
coherent uniform layout(size1x32,binding=4) uimage2D aBufferIndex;

uniform vec2 res;
uniform vec3 eye;
flat in uint brickId;
in vec3 rayOrigin;
layout(location = 0) out vec4 outColor;

void main() {
	ivec2 coords = ivec2(gl_FragCoord.xy);
	vec3 pos = gl_FrontFacing ? rayOrigin : eye;

	uint idx = imageAtomicAdd(aBufferIndex, coords, 1);
	memoryBarrier();
	// TODO: optimize this to use less memory. 16bit? only store 2 components (depth, id)?
	imageStore(aBuffer, ivec3(coords, idx), vec4(pos, float(brickId)));
	memoryBarrier();
	outColor = vec4(1.0);
}