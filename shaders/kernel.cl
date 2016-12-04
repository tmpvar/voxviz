__kernel void hello(__write_only image3d_t image, __global float3* centerPointer, const int time) {
  int3 pos = (int3)(
    get_global_id(0),
    get_global_id(1),
    get_global_id(2)
  );

  float3 center = *centerPointer;

  float hd = (float) get_global_size(0) / 2;

  float dx = get_global_id(0) - hd;
  float dy = get_global_id(1) - hd;
  float dz = get_global_id(2) - hd;
  float sphere = sqrt(dx*dx + dy*dy + dz*dz) - (hd - 8);

  float x = (float)get_global_id(0);
  float y = (float)get_global_id(1);
  float z = (float)get_global_id(2);

  float xv = ((sin((center.x + x + time)/10) + 1) + (cos((y + time)/20) + 1)) / 10 * hd;
  float zv = (cos((y + time)/150) + 1) + (sin((z + time)/50) + 1) / 50 * hd;
  float on = z < xv ? 1.0 : 0.0;

  // float on = z < (sin((x + center.x + time*10) / 50.0f) + 1) * .5 * get_global_size(0) ? 1.0 : 0.0;

  write_imagef(image, (int4)(pos, 0.0), (float4)(on, on, on, on));
}
