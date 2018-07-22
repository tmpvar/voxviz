
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

  float2 t = (float2)(200.0f, 56.0f);
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

// Gradient y
__kernel void gradientY(__write_only image3d_t image, __global float *centerPointer, int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float on = (float)pos.y / (float)get_global_size(1);
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}

// Gradient XY
__kernel void gradientXY(__write_only image3d_t image, __global float *centerPointer, int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
    );

  float y = (float)pos.y / (float)get_global_size(1);
  float x = (float)pos.x / (float)get_global_size(0);
  float on = (x + y) / 2.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}


// cut
__constant sampler_t tool_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
__constant sampler_t stock_sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;

__kernel void opCut(
  read_only image3d_t stock_read,
  write_only image3d_t stock_write,
  const __global int3 *stock_offset,
  read_only image3d_t cutter,
  const __global int3 *cutter_offset,
  read_write __global uint *affected
)
{
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  int3 read_pos = pos + (*cutter_offset);
  int3 write_pos = pos + (*stock_offset);

  float t = read_imagef(cutter, tool_sampler, (int4)(read_pos, 0.0)).x;
  float s = read_imagef(stock_read, stock_sampler, (int4)(write_pos, 0.0)).x;
  if (t > 0.0 && s > 0.0) {
    atomic_add(affected, 1);
    float on = 0.0;
    write_imagef(stock_write, (int4)(write_pos, 0.0), (float4)(on, on, on, on));
  }
}