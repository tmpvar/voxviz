#pragma once

#include <q3.h>

#include <vector>

namespace voxviz {
  class Physics {
    q3Scene *scene;

    vector<q3Body *> bodies;
    vector<Model *> models;
    vector<string> names;
    public:
      Physics() {
        this->scene = new q3Scene(1.0 / 60.0);
      }

      void addModel(Model *model, bool fixed = false, string name = "<nil>") {
        q3BodyDef bodyDef;
        vec3 dims = vec3(model->vox->dims);
        vec3 halfDims =  dims / vec3(2.0);
        vec3 p = model->getPosition();// + halfDims;

        bodyDef.position.Set(p.x, p.y, p.z);
        if (!fixed) {
          bodyDef.bodyType = eDynamicBody;
        }

        q3Body *body = this->scene->CreateBody(bodyDef);

        // add a single box
        q3BoxDef boxDef;
        if (fixed) {
          boxDef.SetRestitution(0.0);
        }
        q3Transform localSpace;
        q3Identity(localSpace);
        boxDef.Set(localSpace, q3Vec3(dims.x, dims.y, dims.z));
        body->AddBox(boxDef);

        this->models.push_back(model);
        this->bodies.push_back(body);
        this->names.push_back(name);
      }

      void step() {
        this->scene->Step();

        u32 body_count = this->bodies.size();
        for (u32 idx = 0; idx < body_count; idx++) {
          q3Transform tx = this->bodies[idx]->GetTransform();
          /*ImGui::Text("body %s pos(%.3f, %.3f, %.3f)",
            this->names[idx],
            tx.position.x,
            tx.position.y,
            tx.position.z
          );*/

          vec3 newPos = vec3(
            tx.position.x,
            tx.position.y,
            tx.position.z
          );

          q3Mat3 r = tx.rotation;
          mat4 rMat (
            vec4(r[0].x, r[0].y, r[0].z, 0.0),
            vec4(r[1].x, r[1].y, r[1].z, 0.0),
            vec4(r[2].x, r[2].y, r[2].z, 0.0),
            vec4(0.0, 0.0, 0.0, 1.0)
          );

          mat4 tMat = translate(mat4(1.0), newPos);


          this->models[idx]->matrix = tMat * rMat;
        }
      }
  };
}