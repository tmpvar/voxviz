#pragma once

#include "volume.h"
#include <vector>

using namespace std;

class VolumeManager {
public:
vector<Volume *> volumes;

VolumeManager() {

}


Volume *addVolume(Volume *volume) {
  if (volume == nullptr) {
    return nullptr;
  }
  // TODO: uniqueness test
  this->volumes.push_back(volume);
}

};