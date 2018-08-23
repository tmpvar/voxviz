#version 330 core

#extension GL_ARB_shader_image_load_store : enable
#extension GL_EXT_bindable_uniform : enable
#extension GL_EXT_shader_image_load_store : enable

#define ABUFFER_MAX_DEPTH_COMPLEXITY 16u

//coherent uniform layout(size4x32) image2DArray aBuffer;
//coherent uniform layout(size1x32) uimage2D aBufferIndex;

uniform vec2 res;
uniform float id;
uniform vec3 eye;

uniform sampler3D volume;

in vec3 rayOrigin;
layout(location = 0) out vec4 outColor;

void main() {
	//ivec2 coords = ivec2(gl_FragCoord.xy);
	//uint idx = imageAtomicAdd(aBufferIndex, coords, ABUFFER_MAX_DEPTH_COMPLEXITY);
	//vec3 pos = gl_FrontFacing ? rayOrigin : eye;

	// TODO: optimize this to use less memory. 16bit? only store 2 components (depth, id)?
	// TODO: is this really an ivec3?
	//imageStore(aBuffer, ivec3(coords, 0), vec4(1.0, 1.0, 1.0, 1.0));
	outColor = vec4(1.0);
}
