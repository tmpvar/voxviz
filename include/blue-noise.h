#pragma once

#include "gl-wrap.h"
#include <stb_image.h>
#include <stdio.h>

#define BLUENOISE_SLICE_BYTES 4096 * 16

class BlueNoise1x64x64x64 {
public:
  // TODO: consider making this static.
  SSBO *ssbo;

  BlueNoise1x64x64x64() {
    this->ssbo = new SSBO(BLUENOISE_SLICE_BYTES * 64);
  }

  void upload() {
    char *s = (char *)malloc(1024);
    int x, y, n;
    char *gpuBuf = (char *)this->ssbo->beginMap(SSBO::MAP_WRITE_ONLY);
    for (unsigned int i = 0; i < 64; i++) {
      sprintf(s, "img/blue-noise/LDR_RG01_%u.png", i);
      unsigned char *tmp = (unsigned char *)stbi_loadf(s, &x, &y, &n, 0);
      memcpy(gpuBuf + (BLUENOISE_SLICE_BYTES * i), tmp, BLUENOISE_SLICE_BYTES);
      stbi_image_free(tmp);
    }
    this->ssbo->endMap();
    free(s);
  }

};