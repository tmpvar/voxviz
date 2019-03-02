#define NUM_NOISE_OCTAVES 5


//float hash(float n) { return fract(sin(n) * 1e4); }
// float hash(vec2 p) { return fract(1e4 * sin(17.0 * p.x + p.y * 0.1) * (0.1 + abs(sin(p.y * 13.0 + p.x)))); }

// float noise(vec3 x) {
//     const vec3 step = vec3(110, 241, 171);
//
//     vec3 i = floor(x);
//     vec3 f = fract(x);
//
//     // For performance, compute the base input to a 1D hash from the integer part of the argument and the
//     // incremental change to the 1D based on the 3D -> 1D wrapping
//     float n = dot(i, step);
//
//     vec3 u = f * f * (3.0 - 2.0 * f);
//     return mix(mix(mix( hash(n + dot(step, vec3(0, 0, 0))), hash(n + dot(step, vec3(1, 0, 0))), u.x),
//                    mix( hash(n + dot(step, vec3(0, 1, 0))), hash(n + dot(step, vec3(1, 1, 0))), u.x), u.y),
//                mix(mix( hash(n + dot(step, vec3(0, 0, 1))), hash(n + dot(step, vec3(1, 0, 1))), u.x),
//                    mix( hash(n + dot(step, vec3(0, 1, 1))), hash(n + dot(step, vec3(1, 1, 1))), u.x), u.y), u.z);
// }

float fbm(vec3 x) {
	float v = 0.0;
	float a = 0.5;
	vec3 shift = vec3(100.0);
	for (int i = 0; i < NUM_NOISE_OCTAVES; ++i) {
		//v += a * noise(x);
		x = x * 2.0 + shift;
		a *= 0.5;
	}
	return v;
}
