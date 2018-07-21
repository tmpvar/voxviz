

float sdUnterprim(float3 p, float4 s, float3 r, float2 ba, float sz2) {
    float3 d = fabs(p) - s.xyz;
    float q = length(max(d.xy, (float2)(0.0, 0.0))) + fmin(0.0f,fmax(d.x,d.y)) - r.x;
    
    float2 pa = (float2)(q, p.z - s.z);
    float2 diag = pa - (float2)(r.z,sz2) * clamp(dot(pa,ba), 0.0f, 1.0f);
    float2 h0 = (float2)(fmax(q - r.z,0.0f),p.z + s.z);
    float2 h1 = (float2)(fmax(q,0.0f),p.z - s.z);
    
    return sqrt(min(dot(diag,diag),fmin(dot(h0,h0),dot(h1,h1))))
        * sign(fmax(dot(pa,(float2)(-ba.y, ba.x)), d.z)) - r.y;
}

// s: width, height, depth, thickness
// r: xy corner radius, z corner radius, bottom radius offset
float sdUberprim(float3 p, float4 s, float3 r) {
    // these operations can be precomputed
    s.xy -= r.x;
    r.x -= s.w;
    s.w -= r.y;
    s.z -= r.y;
    float2 ba = (float2)(r.z, -2.0f*s.z);
    return sdUnterprim(p, s, r, ba/dot(ba,ba), ba.y);
}

// example parameters
#define SHAPE_COUNT 9.0f
float4 getShapeFactor (int i) {
  if (i == 0) { // cube
    return (float4)(1.0,1.0,1.0,1.0);
  } else if (i == 1) { // cylinder
    return (float4)(1.0,1.0,1.0,1.0);
  } else if (i == 2) { // cone
    return (float4)(0.0,0.0,1.0,1.0);
  } else if (i == 3) { // pill
    return (float4)(1.0,1.0,2.0,1.0);
  } else if (i == 4) { // sphere
    return (float4)(1.0,1.0,1.0,1.0);
  } else if (i == 5) { // pellet
    return (float4)(1.0,1.0,0.25,1.0);
  } else if (i == 6) { // torus
    return (float4)(1.0,1.0,0.25,0.25);
  } else if (i == 7) { // pipe
    return (float4)(1.0,1.0,1.0,0.25);
  } else if (i == 8) { // corridor
    return (float4)(1.0,1.0,1.0,0.25);
  }
}

float3 getRadiusFactor(int i) {
  if (i == 0) { // cube
    return (float3)(0.0,0.0,0.0);
  } else if (i == 1) { // cylinder
    return (float3)(1.0,0.0,0.0);
  } else if (i == 2) { // cone
    return (float3)(0.0,0.0,1.0);
  } else if (i == 3) { // pill
    return (float3)(1.0,1.0,0.0);
  } else if (i == 4) { // sphere
    return (float3)(1.0,1.0,0.0);
  } else if (i == 5) { // pellet
    return (float3)(1.0,0.25,0.0);
  } else if (i == 6) { // torus
    return (float3)(1.0,0.25,0.0);
  } else if (i == 7) { // pipe
    return (float3)(1.0,0.1,0.0);
  } else if (i == 8) { // corridor
    return (float3)(0.1,0.1,0.0);
  }
}


__kernel void hello1(__write_only image3d_t image, __global float3* centerPointer, int time) {
  float on = 1.0;

  int3 ipos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float3 center = *centerPointer;

  float hd = (float) get_global_size(0) / 2;

  float x = (float)get_global_id(0);
  float y = (float)get_global_id(1);
  float z = (float)get_global_id(2);


  float k = (float)time*0.01;
  float tmp;
  float u = smoothstep(0.0f,1.0f,smoothstep(0.0f,1.0f,fract(k, &tmp)));
  int s1 = (int)fmod(k, SHAPE_COUNT);
  int s2 = (int)fmod(k+1.0f, SHAPE_COUNT);

  float scale = 100.0f;
  float4 shapeScale = (float4)(scale, scale, scale, scale);
  float3 radiusScale = (float3)(scale, scale, scale);


  float4 shapeFactorA = getShapeFactor(s1) * shapeScale;
  float4 shapeFactorB = getShapeFactor(s2) * shapeScale;

  float3 radiusFactorA = getRadiusFactor(s1) * radiusScale;
  float3 radiusFactorB = getRadiusFactor(s2) * radiusScale;


  float3 pos = (float3)(x, y, z) + center;

  float v = sdUberprim((float3)(128.0, 128.0, 128.0) - pos, mix(shapeFactorA,shapeFactorB,u), mix(radiusFactorA,radiusFactorB,u));

  on = v <= 0 ? 1.0 : 0.0;
  write_imagef(image, (int4)(ipos, 0.0f), (float4)(on, on, on, on));
}

__kernel void waves(__write_only image3d_t image, __global float3* centerPointer, int time) {
  float on = 1.0;

  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float3 center = *centerPointer;

  float hd = (float) get_global_size(0) / 2;

  float x = (float)get_global_id(0);
  float y = (float)get_global_id(1);
  float z = (float)get_global_id(2);

  float xx =  0.5 * (sin((center.x + x)/100.0 + time / 10.0) + 1) * hd;
  float yy =  0.5 * (cos((center.z + z)/10.0 + time / 100.0) + 1) * hd;

  // float xx =  0.5 * (cos((center.x + x)/100.0 + time / 10.0) + 1) * hd;
  // float yy =  0.5 * (tan((center.z + z)/100.0 + time / 100.0) + 1) * hd;
  float d = 80;
  float sphere = distance(center + (float3)(x, y, z), (float3)(d, d/4.0, d)) - d - (x/y);
  float wave = sin((center.x + x)/10.0 + time / 2.0) / 1000;
  float v = sin((center.x + x)/100.0 + sphere / 10.0 + time / 10.0) / 10.0;
  v += cos((center.y - x)/5.0 + v/10.0 * time/100.0) / 60.0;

  on = v < 0.0 ? 1.0 : 0.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}

// sphere
__kernel void sphere(__write_only image3d_t image, __global float *centerPointer, int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float hd = (float) get_global_size(0) / 2;
  float3 center = *centerPointer;

  // float hd = 256.0;
  float dx = get_global_id(0) - hd;
  float dy = get_global_id(1) - hd;
  float dz = get_global_id(2) - hd;
  float sphere = sqrt(dx*dx + dy*dy + dz*dz) - hd;

  float on = sphere < 0.0 ? 1.0 : 0.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}

// sphere
__kernel void cylinder(__write_only image3d_t image, __global float *centerPointer, int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float2 fpos = (float2)((float)pos.x, (float)pos.z);
  float2 hp = (float2)(
    (float) get_global_size(0) / 2.0f,
    (float) get_global_size(2) / 2.0f
  );

  float r = fmin(hp.x, hp.y) - 4.0f;
  float d = distance(hp, fpos) - r;

  float on = d < 0.0f ? 1.0 : 0.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}

// torus
__kernel void torus(__write_only image3d_t image, __global float *centerPointer, int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float3 fpos = (float3)((float)pos.x, (float)pos.y, (float)pos.z);
  float3 hp = (float3)(
    (float) get_global_size(0) / 2.0f,
    (float) get_global_size(1) / 2.0f,
    (float) get_global_size(2) / 2.0f
  );

  float2 t = (float2)(32.0f, 10.0f);
  float3 p = hp - fpos;
  float2 q = (float2)(length(p.xz) - t.x, p.y);
  float d = length(q) - t.y;

  float on = d < 0.0f ? 1.0 : 0.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}


// fill all
__kernel void fillAll(__write_only image3d_t image, __global float *centerPointer, int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float on = 1.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}

// cut


__constant sampler_t tool_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void opCut(write_only image3d_t stock, const __global int3 *stock_offset, read_only image3d_t cutter, const __global int3 *cutter_offset)
{
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );


  int3 read_pos = pos + (*cutter_offset);
  int3 write_pos = pos + (*stock_offset);

  float v = read_imagef(cutter, tool_sampler,(int4)(read_pos, 0.0)).x;
  if (v > 0.0) {
    float on = 0.0;
    write_imagef(stock, (int4)(write_pos, 0.0), (float4)(on, on, on, on));
  }
}