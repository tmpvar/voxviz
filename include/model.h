#pragma once

#include "gl-wrap.h"
#include "greedy-mesher.h"
#include "parser/magicavoxel/vox.h"

#include <glm/glm.hpp>
#include <string>

using namespace std;
using namespace glm;

class Model {

  Model(VOXModel *m) {
    this->vox = m;
    this->mesh = new Mesh();

    greedy(this->vox->buffer, ivec3(this->vox->dims), this->mesh);
    this->mesh->upload();
    this->matrix = mat4(1.0);
    u32 total_bytes = this->vox->dims.x * this->vox->dims.y * this->vox->dims.z;

    this->data = new SSBO(
      total_bytes,
      this->vox->dims
    );

    this->dims = this->vox->dims;

    uint8_t *buf = (uint8_t *)this->data->beginMap(SSBO::MAP_WRITE_ONLY);
    memcpy(buf, this->vox->buffer, total_bytes);
    this->data->endMap();
  }

public:

  enum SourceModelType {
    Unknown = 0,
    MagicaVoxel
  };

  u8 *buffer;
  uvec3 dims;
  Mesh *mesh;
  SSBO *data;
  VOXModel *vox;
  SourceModelType source_model_type = SourceModelType::Unknown;
  mat4 matrix;

  static Model *New(string filename) {
    VOXModel *vox = VOXParser::parse(filename);
    if (vox == nullptr) {
      cout << "cannot load " << filename << endl;
      return nullptr;
    }
    return new Model(vox);
  }


  void render(Program *program, mat4 VP) {
    program
      ->uniform1ui("totalTriangles", this->mesh->faces.size() / 6)
      ->uniformMat4("model", this->matrix)
      ->uniform1ui("totalVerts", this->mesh->verts.size());

    this->mesh->render(program, "position");
  }
};