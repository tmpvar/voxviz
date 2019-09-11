#pragma once

#include <Common/Common.h>
#include <Simulation/Simulation.h>
#include <Simulation/SimulationModel.h>
#include <Simulation/TimeStepController.h>
#include <Simulation/TimeManager.h>
#include <Simulation/CubicSDFCollisionDetection.h>
#include <Utils/Logger.h>
#include <Utils/Timing.h>

#include <vector>

INIT_LOGGING
INIT_TIMING
//INIT_COUNTING

namespace voxviz {
  class Physics {
    public:
      u32 steps = 10;
      PBD::SimulationModel *simModel;
      PBD::TimeStepController *timeStepController;

      std::vector<Model *> models;
      PBD::DistanceFieldCollisionDetection cd;


      Physics() {
        this->simModel = new PBD::SimulationModel();
        this->simModel->init();
        PBD::Simulation::getCurrent()->setModel(this->simModel);

        this->timeStepController = new PBD::TimeStepController();
        PBD::Simulation::getCurrent()->setSimulationMethod(0);

        PBD::TimeManager::getCurrent()->setTimeStepSize(static_cast<Real>(0.001));
        PBD::Simulation::getCurrent()->getTimeStep()->setCollisionDetection(*this->simModel, &this->cd);

      }

      Physics *step() {
        for (u32 i = 0; i < this->steps; i++) {
          START_TIMING("SimStep");
          PBD::Simulation::getCurrent()->getTimeStep()->step(*this->simModel);
          STOP_TIMING_AVG;
        }

        // apply new transforms to the models
        PBD::SimulationModel::RigidBodyVector &rigidBodies = this->simModel->getRigidBodies();

        u32 total_bodies = rigidBodies.size();
        for (u32 i = 0; i < total_bodies; i++) {
          // TODO: sanity checks here to ensure these are the same object and both exist!
          const Vector3r &p = rigidBodies[i]->getPosition();
          this->models[i]->setPosition(p[0], p[1], p[2]);
        }

        return this;
      }

      // TODO: this only handles a single model and we are going to need something that
      // contains multiple models for this world to work properly
      void addModel(Model *model, bool fixed = false) {
        this->models.push_back(model);

        PBD::SimulationModel::RigidBodyVector &rigidBodies = this->simModel->getRigidBodies();

        const Real density = 100.0;

        u32 total_verts = model->mesh->verts.size();
        u32 total_faces = model->mesh->faces.size();

        Utilities::IndexedFaceMesh mesh;
        mesh.initMesh(total_verts, total_faces, 0);

        // TODO: compute this
        PBD::VertexData vd;
        vd.reserve(total_verts);

        for (u32 i = 0; i < total_verts; i+=3) {
          vd.addVertex(Vector3r(
            model->mesh->verts[i],
            model->mesh->verts[i+1],
            model->mesh->verts[i+2]
          ));
        }

        for (u32 vert = 0; vert < total_verts; vert += 3) {
          vd.addVertex(Vector3r(
            model->mesh->verts[vert],
            model->mesh->verts[vert + 1],
            model->mesh->verts[vert + 2]
          ));
        }

        int posIndices[3];
        for (u32 idx = 0; idx < total_faces; idx+=3) {
          posIndices[0] = model->mesh->faces[idx];
          posIndices[1] = model->mesh->faces[idx+1];
          posIndices[2] = model->mesh->faces[idx + 2];
          // This creates a copy
          mesh.addFace(&posIndices[0]);
        }

        vec3 pos = model->getPosition();

        PBD::RigidBody *body = new PBD::RigidBody();

        body->initBody(
          density,
          Vector3r(pos.x, pos.y, pos.z),
          Quaternionr(1.0, 0.0, 0.0, 0.0),
          vd,
          mesh
        );

        body->setRestitutionCoeff(0.0000001);

        if (fixed) {
          body->setMass(0.0);
        }
        else {
          body->setMass(1000.0);
        }

        mesh.buildNeighbors();
        mesh.updateNormals(vd, 0);
        mesh.updateVertexNormals(vd);

        const vector<Vector3r> *vertices = body->getGeometry().getVertexDataLocal().getVertices();
        const u32 nVert = static_cast<unsigned int>(vertices->size());
        this->cd.addCollisionBox(
          rigidBodies.size(),
          PBD::CollisionDetection::CollisionObject::RigidBodyCollisionObjectType,
          &(*vertices)[0],
          nVert,
          Vector3r(model->dims.x, model->dims.y, model->dims.z)
        );

        rigidBodies.push_back(body);
      }

      Physics *debug() {
        u32 idx = 0;
        for (auto &body : this->simModel->getRigidBodies()) {
          Vector3r pos = body->getPosition();
          ImGui::Text("body %u pos(%.5f, %.5f, %.5f)", idx, pos[0], pos[1], pos[2]);
          idx++;
        }
        return this;
      }
  };
}