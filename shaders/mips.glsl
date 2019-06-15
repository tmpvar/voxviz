#ifndef _LOD_DEFS
#define _LOD_DEFS

#define MAX_MIPS 7

const vec2 lod_defs[8] = vec2[8](
    vec2(0, 1),
    vec2(1, 0.12499999999),
    vec2(1.12499999999, 0.0156249999975),
    vec2(1.1406249999875, 0.00195312499953125),
    vec2(1.1425781249870313, 0.000244140624921875),
    vec2(1.142822265611953, 0.000030517578112792968),
    vec2(1.142852783190066, 0.000003814697263793945),
    vec2(1.1428565978873297, 4.7683715793609617e-7)
);

#endif
