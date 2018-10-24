#pragma once

#include <fstream>
#include <iostream>

using namespace std;

/*
FORMAT
HEADER
string "VZD"
uint32 volumeCount

VOLUME
mat4 transform (length: 64 bytes)
uint32 materialLength
void *material (length: materialLength bytes)
uint32 voxelSize (bytes)
uint32 brickCount

BRICKS
int32 x-index
int32 y-index
int32 z-index
void *brickVoxels (length: voxelSize * BRICK_DIAMETER^3)

repeat VOLUME
*/

// TODO: make loading async
class VZDParser {

public:
	static void parse(string filename, VolumeManager *volumeManager, q3Scene *scene, q3BodyDef bodyDef) {
    cout << "Loading VZD file: " << filename.c_str() << endl;
    ifstream ifs(filename, ios::binary | ios::ate);
    ifs.seekg(0, ios::beg);

    // file ID
    char id[4] = { 0,0,0,0 };
    ifs.read(&id[0], 3);
    cout << "found ID: " << id << endl;

    // Volume Count
    uint32_t volumeCount;
    ifs.read((char *)&volumeCount, 4);
    cout << "Total Volumes: " << volumeCount << endl;

    // Volume
    for (uint32_t i = 0; i < volumeCount; i++) {
      // Volume Transform
      glm::mat4 xt;
      ifs.read((char *)&xt, sizeof(glm::mat4));
      cout << "Volume Transform: " << endl;
      cout << "  " << xt[0][0] << " " << xt[0][1] << " " << xt[0][2] << " " << xt[0][3] << " " << endl;
      cout << "  " << xt[1][0] << " " << xt[1][1] << " " << xt[1][2] << " " << xt[1][3] << " " << endl;
      cout << "  " << xt[2][0] << " " << xt[2][1] << " " << xt[2][2] << " " << xt[2][3] << " " << endl;

      // TODO: this should not live here
      Volume *volume = new Volume(glm::vec3(0.0, 0, 0));
      volumeManager->addVolume(volume);


      // Volume Material
      uint32_t materialSize;
      ifs.read((char *)&materialSize, 4);
      cout << "Material Size: " << materialSize << endl;
      
      // TODO: material could be any number of things
      uint8_t color[3];
      ifs.read((char*)&color, 3);
      cout << "Material Color: r:" << unsigned(color[0]) << " g:" << unsigned(color[1]) << " b:" << unsigned(color[2]) << endl;
      
      volume->material.r = color[0] / 255.0f;
      volume->material.g = color[1] / 255.0f;
      volume->material.b = color[2] / 255.0f;
      volume->material.a = 1.0;

      // Voxel size in bytes
      // TODO: if this is not 1 byte then we need to propagate this up
      //       into the brick
      uint32_t voxelSize;
      ifs.read((char *)&voxelSize, 4);
      cout << "Voxel Size (bytes): " << voxelSize << endl;

      // Brick count
      uint32_t totalBricks;
      ifs.read((char *)&totalBricks, 4);
      cout << "Total Bricks: " << totalBricks << endl;
      
      uint8_t *brickData = (uint8_t *)malloc(BRICK_VOXEL_COUNT);

      q3BoxDef boxDef;

      // Collect brick data
      for (uint32_t i = 0; i < totalBricks; i++) {

        glm::ivec3 brickIndex;
        ifs.read((char *)&brickIndex, 12);
        ifs.read((char *)brickData, BRICK_VOXEL_COUNT);
        //cout << "brick index: " << i << " of " << totalBricks << endl;
        Brick *brick = volume->AddBrick(brickIndex);
        
        for (uint32_t j = 0; j < BRICK_VOXEL_COUNT; j++) {
          brick->data[j] = float(brickData[j]);
        }
      }

      for (auto& it : volume->bricks) {
        Brick *brick = it.second;
        brick->createGPUMemory();
      }
    }
  }
};