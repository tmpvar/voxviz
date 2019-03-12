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
  uint8_t palette[256];
  glm::uvec3 dims;
  size_t bytes = 0;
  uint8_t *buffer = nullptr;
  
  ~VOXModel() {
    if (this->buffer != nullptr) {
      free(this->buffer);
    }
  }
};

class VOXParser {

public:
	static VOXModel *parse(string filename) {
    cout << "Loading VOX file: " << filename.c_str() << endl;
    ifstream ifs(filename, ios::binary | ios::ate);
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
        uint32_t size;
                
        ifs.read((char *)&size, 4);
        vox->dims.x = size;

        ifs.read((char *)&size, 4);
        vox->dims.y = size;

        ifs.read((char *)&size, 4);
        vox->dims.z = size;
        
        const uint64_t bytes = vox->dims.x*vox->dims.y*vox->dims.z;
        vox->buffer = (uint8_t *)malloc(bytes);
        memset(vox->buffer, 0, bytes);
      }
      else if (cmp(chunk_id, "RGBA")) {
        ifs.read((char *)&vox->palette, 256);
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
        //cout << "    num voxels: " << num_voxels << endl;
        for (uint32_t i = 0; i < num_voxels; i++) {
          uint8_t val[4];
          ifs.read((char *)&val[0], 4);
          glm::uvec3 pos = glm::uvec3(val[0], val[2], val[1]);

          if (glm::any(glm::greaterThanEqual(pos, vox->dims))) {
            continue;
          }
          uint64_t idx = (
            pos.x +
            pos.y * vox->dims.x +
            pos.z * vox->dims.x * vox->dims.y
          );

          vox->buffer[idx] = val[3];
        }
      }
      else {
        break;
      }
    }

    return vox;
	}
};