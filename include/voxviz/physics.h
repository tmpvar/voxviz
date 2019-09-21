#pragma once

#include <q3.h>

#include <vector>

namespace voxviz {
  class Physics {
    q3Scene *scene;

    vector<q3Body *> bodies;
    vector<Model *> models;
    public:
      Physics() {
        this->scene = new q3Scene(1.0 / 60.0);
      }

      void addModel(Model *model) {
        q3BodyDef bodyDef;
        q3Body *body = this->scene->CreateBody(bodyDef);

        // TODO: convert the voxel grid into a bunch of boxes that live
        //       in the physics body


        this->models.push_back(model);
        this->bodies.push_back(body);
      }
  };
}