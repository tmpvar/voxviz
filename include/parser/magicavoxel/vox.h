#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include "glm/glm.hpp"

using namespace std;

typedef struct vox_chunk_t {
  char id[4];
  uint32_t chunk_bytes;
  uint32_t child_chunk_bytes;
  char *content;
};

static bool cmp(const char *a, const char *b) {
  return strcmp(a, b) == 0;
}

class VOXModel {
  public:
  glm::uvec4 palette[256];
  glm::uvec3 dims;
  size_t bytes = 0;
  uint8_t *buffer = nullptr;

  ~VOXModel() {
    if (this->buffer != nullptr) {
      free(this->buffer);
    }
  }

  u32 voxelIndex(uvec3 pos) {
    // TODO: use morton order
    return (
      pos.x +
      pos.y * this->dims.x +
      pos.z * this->dims.x * this->dims.y
    );
  }

  void setVoxel(uvec3 pos, u8 val) {
    if (glm::any(glm::greaterThanEqual(pos, this->dims))) {
      return;
    }
    this->buffer[this->voxelIndex(pos)] = val;
  }

  u8 getVoxel(uvec3 pos) {
    if (glm::any(glm::greaterThanEqual(pos, this->dims))) {
      return 0;
    }
    return this->buffer[this->voxelIndex(pos)];
  }

  bool oob(ivec3 pos) {
    if (glm::any(glm::greaterThanEqual(pos, ivec3(this->dims)))) {
      return true;
    }

    if (glm::any(glm::lessThan(pos, ivec3(0)))) {
      return true;
    }

    return false;
  }

  bool visible(uvec3 pos) {
    if (pos.x == 0 || pos.y == 0 || pos.z == 0) {
      return true;
    }

    ivec3 p = ivec3(pos);
    for (int d = 0; d < 3; d++) {
      ivec3 l(0, 0, 0);
      ivec3 u(0, 0, 0);

      l[d] = -1;
      u[d] = 1;

      l += p;
      u += p;

      if (oob(l) || !this->getVoxel(l)) {
        return true;
      }

      if (oob(u) || !this->getVoxel(u)) {
        return true;
      }
    }

    return false;
  }

};

class VOXParser {

public:
	static VOXModel *parse(string filename) {
    cout << "Loading VOX file: " << filename.c_str() << endl;
    ifstream ifs(filename, ios::binary | ios::ate);
    if (!ifs.is_open()) {
      return nullptr;
    }
    ifs.seekg(0, ios::beg);

    //cout << "  VOX Header" << endl;
    // Header (Name)
    char id[5] = { 0, 0, 0, 0, 0 };
    ifs.read(&id[0], 4);
    //cout << "    id: " << id << endl;

    // Header (Version)
    uint32_t version;
    ifs.read((char *)&version, 4);
    //cout << "    version: " << version << endl;

    VOXModel *vox = new VOXModel();
    while (!ifs.eof()) {
      char chunk_id[5] = { 0, 0, 0, 0, 0 };
      ifs.read(&chunk_id[0], 4);
      //cout << "  Chunk '" << chunk_id << "'" << endl;

      //if (cmp(chunk_id, "MAIN")) {
        uint32_t chunk_bytes;
        ifs.read((char *)&chunk_bytes, 4);
        //cout << "    num bytes chunk content: " << chunk_bytes << endl;

        uint32_t chunk_children_bytes;
        ifs.read((char *)&chunk_children_bytes, 4);
        //cout << "    num bytes children chunks: " << chunk_children_bytes << endl;
      //}
      if (cmp(chunk_id, "MAIN")) {
        // noop
      }
      else if (cmp(chunk_id, "PACK")) {
        //cout << "    skip" << endl;
        ifs.seekg(4, ifs.cur);
      }
      else if (cmp(chunk_id, "SIZE")) {
        ifs.read((char *)&vox->dims.x, 4);
        // swap y and z axes
        ifs.read((char *)&vox->dims.z, 4);
        ifs.read((char *)&vox->dims.y, 4);

        const uint64_t bytes = vox->dims.x*vox->dims.y*vox->dims.z;
        vox->buffer = (uint8_t *)malloc(bytes);
        memset(vox->buffer, 0, bytes);
      }
      else if (cmp(chunk_id, "RGBA")) {
        uint32_t palette_entry;
        for (int i = 0; i <= 254; i++) {
          ifs.read((char *)&palette_entry, 4);

          // read the alpha channel
          vox->palette[i + 1].a = palette_entry && 0xFF;

          // read the blue channel
          palette_entry = palette_entry >> 8;
          vox->palette[i + 1].b = palette_entry && 0xFF;

          // read the green channel
          palette_entry = palette_entry >> 8;
          vox->palette[i + 1].g = palette_entry && 0xFF;

          // read the red channel
          palette_entry = palette_entry >> 8;
          vox->palette[i + 1].r = palette_entry && 0xFF;
        }
      }
      else if (cmp(chunk_id, "MATT")) {
        // skip id, type, weight
        ifs.seekg(4 * 3, ifs.cur);

        uint32_t material_property_bits;
        ifs.read((char *)&material_property_bits, 4);

        // skip prop values
        uint32_t tmp = material_property_bits - ((material_property_bits >> 1) & 0x55555555);
        tmp = (tmp & 0x33333333) + ((tmp >> 2) & 0x33333333);
        uint32_t cnt = ((tmp + (tmp >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
        ifs.seekg(4 * cnt);
      }
      else if (cmp(chunk_id, "XYZI")) {
        uint32_t num_voxels;
        ifs.read((char *)&num_voxels, 4);
        cout << "    num voxels: " << num_voxels << endl;
        uint8_t val[4];
        for (uint32_t i = 0; i < num_voxels; i++) {
          ifs.read((char *)&val[0], 4);
          // swap y and z axes
          glm::uvec3 pos = glm::uvec3(val[0], val[2], val[1]);

          vox->setVoxel(pos, val[3]);
        }
      }
      else {
        break;
      }
    }

    return vox;
	}
};