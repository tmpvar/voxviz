#version 330 core

in vec3 rayOrigin;

out vec4 outColor;

uniform sampler3D volume;
uniform ivec3 dims;
uniform vec3 eye;
uniform vec3 center;
uniform int showHeat;

#define ITERATIONS 50

float getVoxel(vec3 worldPos, vec3 fdims, vec3 hfdims) {
  vec3 pos = (hfdims + worldPos - center) / vec3(dims);
  return any(lessThan(pos, vec3(0.0))) || any(greaterThan(pos, vec3(1.0))) ? -1.0 : texture(volume, pos).r;
}

float voxel(vec3 worldPos) {
  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims * 0.5;

  vec3 pos = (hfdims + worldPos - center) / fdims;
  return any(lessThan(pos, vec3(0.0))) || any(greaterThan(pos, vec3(1.0))) ? -1.0 : texture(volume, pos).r;
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 0.6666666666, 0.33333333333, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 heat(float amount, float total) {
  float p = (amount / total);
  vec3 hsv = vec3(0.66 - p, 1.0, .75);
  return vec4(hsv2rgb(hsv), 1.0);
}

// phong shading
vec3 shading( vec3 v, vec3 n, vec3 eye ) {
	vec3 final = vec3( 0.0 );

	// light 0
	{
		vec3 light_pos = eye + vec3(0.0, 0.0, 10.0);//vec3( 100.0, 110.0, 150.0 );
		vec3 light_color = vec3( 1.0 );
		vec3 vl = normalize( light_pos - v );
		float diffuse  = max( 0.0, dot( vl, n ) );
		final += light_color * diffuse;
	}

	// light 1
	{
		vec3 light_pos = -vec3( 100.0, 110.0, 120.0 );
		vec3 light_color = vec3( 1.0 );
		vec3 vl = normalize( light_pos - v );
		float diffuse  = max( 0.0, dot( vl, n ) );
		final += light_color * diffuse;
	}

	return final;
}

float march(vec3 pos, vec3 dir, out vec3 mask, out vec3 center, out float hit, out float iterations) {
	// grid space
  vec3 grid = floor( pos );
	vec3 grid_step = sign( dir );
	vec3 corner = max( grid_step, vec3( 0.0 ) );

	// ray space
	vec3 inv = vec3( 1.0 ) / dir;
	vec3 ratio = ( grid + corner - pos ) * inv;
	vec3 ratio_step = grid_step * inv;

	// dda
	hit = -1.0;
  iterations = 0.0;
	for ( int i = 0; i < ITERATIONS; i++ ) {
		if ( voxel( grid ) > 0.0 ) {
			hit = 1.0;
			continue;
		}
    iterations++;
		vec3 cp = step( ratio, ratio.yzx );

		mask = cp * ( vec3( 1.0 ) - cp.zxy );

		grid  += grid_step  * mask;
		ratio += ratio_step * mask;
	}

	center = grid + vec3( 0.5 );
	return dot( ratio - ratio_step, mask ) * hit;
}

void main() {
  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims / 2.0;

  vec3 fulldims = fdims * 16.0;

  vec3 pos = gl_FrontFacing ? rayOrigin : eye;
  vec3 dir = normalize(gl_FrontFacing ? rayOrigin - eye : eye - rayOrigin);

  vec3 mask, center;
  float hit, iterations;
  float depth = march(pos, dir, mask, center, hit, iterations);

  if (hit <= 0.0) {
    // we have to discard to avoid obscuring opaque bricks that live behind
    // the current one
    discard;
  }

	vec3 p = pos + dir * depth;
	vec3 n = -vec3(mask) * sign( dir );
  vec3 color = shading( p, n, eye );

  outColor = vec4(color * p / fulldims, 1.0);

  // outColor = vec4(normalize(sideDist), 1.0);
  // outColor = vec4(vec3(mask), 1.0);

  //outColor = outColor * distance()

  // if (mask.x) {
  //   outColor = outColor * 0.5;
  // } else if (mask.z) {
  //   outColor = outColor * 0.75;
  // }

  outColor = mix(outColor, heat(iterations, ITERATIONS), showHeat);
}
