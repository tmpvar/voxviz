__kernel void hello2(__write_only image3d_t image, const __global float3* centerPointer, const int time) {
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
  float yy =  0.5 * (cos((center.z + z)/100.0 + time / 100.0) + 1) * hd;

  // float xx =  0.5 * (cos((center.x + x)/100.0 + time / 10.0) + 1) * hd;
  // float yy =  0.5 * (tan((center.z + z)/100.0 + time / 100.0) + 1) * hd;
  float d = 80;
  float sphere = distance(center + (float3)(x, y, z), (float3)(d, d, d)) - d - 10;
  float wave = sin((center.x + x)/100.0 * time / 2.0);
  float v = sin((center.x + x)/100.0 + sphere / 10.0 + time / 10.0) / 10.0;
  v += cos((center.y - y)/5.0 + v/10.0 * time/100.0) / 60.0;
  on = v < 0.0 ? 1.0 : 0.0;
  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}

__kernel void hello(__write_only image3d_t image, const __global float *centerPointer, const int time) {
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

  // return;

  // float x = (float)get_global_id(0);
  // float y = (float)get_global_id(1);
  // float z = (float)get_global_id(2);
  //
  // float xx =  0.5 * (sin((center.x + x)/100.0 + time / 10.0) + 1) * hd;
  // float yy =  0.5 * (sin((center.z + z)/100.0 + time / 100.0) + 1) * hd;
  //
  // float on = y < xx + yy ? 1.0 : 0.0;
  // write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}
