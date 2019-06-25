#pragma once

#include <string>
#include <fstream>
#include <iostream>

using namespace std;

class RAWParser {

public:
  static void parse(string filename, VolumeManager *volumeManager, glm::uvec3 dims) {
    cout << "Loading RAW file: " << filename.c_str() << endl;
    ifstream ifs(filename, ios::binary | ios::ate);
    if (!ifs.is_open()) {
      cout << "unable to open file" << endl;
      return;
    }
    ifs.seekg(0, ios::beg);


    Volume *volume = new Volume(glm::vec3(0, 50, 0));
    volumeManager->addVolume(volume);

    uint8_t b;
    
    uint64_t idx = 0;
    while (!ifs.eof()) {
      ifs.read((char *)&b, 1);
      
      if (idx % 100000000 == 0) { cout << "process raw idx " << idx << endl; }
      if (b > 100 && b < 110) {
        // compute voxel coordinates
        uint64_t t = idx;
        glm::uvec3 pos;
        pos.x = t % (dims.x);
        t /= (dims.x);
        pos.y = t % (dims.y);
        t /= (dims.y);
        pos.z = t;

        glm::uvec3 brickIndex = pos / glm::uvec3(BRICK_DIAMETER);
        glm::uvec3 voxelIndex = pos % glm::uvec3(BRICK_DIAMETER);
        
        // upsert brick
        Brick *brick = volume->getBrick(brickIndex, true);
        
        // set voxel
        brick->setVoxel(voxelIndex);
      }
      
      idx++;
    }
      
    // upload all bricks
    for (auto& it : volume->bricks) {
      Brick *brick = it.second;
      brick->upload();
    }

    for (auto& it : volume->bricks) {
      Brick *brick = it.second;
      brick->freeHostMemory();
    }

   
  }
};