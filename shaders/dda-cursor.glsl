struct DDACursor {
  vec3 mask;
  vec3 mapPos;
  vec3 rayStep;
  vec3 rayDir;
  vec3 rayPos;
  vec3 sideDist;
  vec3 deltaDist;
};

DDACursor dda_cursor_create(
  in const vec3 pos,
  in const vec3 gridCenter,
  in const vec3 gridRadius,
  in const vec3 rayDir
) {
  DDACursor cursor;
  vec3 gp = (pos - gridCenter) + gridRadius;
  cursor.rayPos = pos;
  cursor.rayDir = rayDir;
  cursor.mapPos = floor(gp);
  cursor.deltaDist = abs(vec3(length(rayDir)) / rayDir);
  cursor.rayStep = sign(rayDir);
  vec3 p = (cursor.mapPos - gp);
  cursor.sideDist = (
    cursor.rayStep * p + (cursor.rayStep * 0.5) + 0.5
  ) * cursor.deltaDist;
  cursor.mask = step(cursor.sideDist.xyz, cursor.sideDist.yzx) *
                step(cursor.sideDist.xyz, cursor.sideDist.zxy);
  return cursor;
}

void dda_cursor_step(in out DDACursor cursor, out vec3 normal) {
  vec3 sideDist = cursor.sideDist;
  normal = vec3(1.0);//cursor.mask;
  cursor.mask = step(sideDist.xyz, sideDist.yzx) *
                step(sideDist.xyz, sideDist.zxy);
  cursor.sideDist += cursor.mask * cursor.deltaDist;
  cursor.mapPos += cursor.mask * cursor.rayStep;
  cursor.rayPos += cursor.mask * cursor.rayStep;
}
