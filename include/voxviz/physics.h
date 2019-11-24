#pragma once
#define BT_ENABLE_PROFILE 1

//#include <Simulation/SimulationModel.h>
//#include <Simulation/TimeStepController.h>

#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionShapes/btMiniSDF.h>

#include <LinearMath/btQuickprof.h>

#include <voxviz/physics/broadphase-grid.h>

namespace voxviz {

  glm::mat4 bulletTransformToGLM(btScalar* matrix) {
    return glm::mat4(
      matrix[0], matrix[1], matrix[2], matrix[3],
      matrix[4], matrix[5], matrix[6], matrix[7],
      matrix[8], matrix[9], matrix[10], matrix[11],
      matrix[12], matrix[13], matrix[14], matrix[15]);
  }

  class Physics {
    public:

      btDiscreteDynamicsWorld* dynamicsWorld;
      btSequentialImpulseConstraintSolver* solver;
      btBroadphaseInterface* broadphase;
      btCollisionDispatcher* dispatcher;
      btDefaultCollisionConfiguration* collisionConfiguration;

      btAlignedObjectArray<btCollisionShape*> collisionShapes;
      btCollisionShape* voxelShape;

      vector <Model *>models;

      Physics() {
        this->collisionConfiguration = new btDefaultCollisionConfiguration();

        ///use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        this->dispatcher = new btCollisionDispatcher(this->collisionConfiguration);

        ///btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
        //this->broadphase = new btDbvtBroadphase();
        this->broadphase = new bt32BitAxisSweep3(
          btVector3(0.0, 0.0, 0.0),
          btVector3(1024.0, 1024.0, 1024.0)
        );
        //this->broadphase = new voxviz::physics::BroadphaseGrid(uvec3(1024));



        ///the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        this->solver = new btSequentialImpulseConstraintSolver;

        this->dynamicsWorld = new btDiscreteDynamicsWorld(
          this->dispatcher,
          this->broadphase,
          this->solver,
          this->collisionConfiguration
        );

        dynamicsWorld->setGravity(btVector3(0, -10, 0));

        // add a single voxel as a collision shape
        const btScalar one = btScalar(0.5);
        this->voxelShape = new btBoxShape(btVector3(one, one, one));
        //this->voxelShape = new btSphereShape(1.0);
        collisionShapes.push_back(this->voxelShape);
      }


      btRigidBody* createRigidBody(
        float mass,
        const btTransform& startTransform,
        btCollisionShape* shape,
        const btVector4& color = btVector4(1, 0, 0, 1)
      ) {
        btAssert((!shape || shape->getShapeType() != INVALID_SHAPE_PROXYTYPE));

        //rigidbody is dynamic if and only if mass is non zero, otherwise static
        bool isDynamic = (mass != 0.f);

        btVector3 localInertia(0, 0, 0);
        if (isDynamic) {
          shape->calculateLocalInertia(mass, localInertia);
        }

        btDefaultMotionState* myMotionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo cInfo(mass, myMotionState, shape, localInertia);
        btRigidBody* body = new btRigidBody(cInfo);

        body->setUserIndex(this->models.size());
        this->dynamicsWorld->addRigidBody(body);
        return body;
      }

      void tick(float dt) {

        CProfileManager::Start_Profile("tick");
        dynamicsWorld->stepSimulation(dt, 10);
        // TODO: apply transformations back to the models
        for (int j = this->dynamicsWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
          btCollisionObject* obj = this->dynamicsWorld->getCollisionObjectArray()[j];
          btRigidBody* body = btRigidBody::upcast(obj);
          btTransform trans;
          if (body && body->getMotionState())
          {
            body->getMotionState()->getWorldTransform(trans);
          }
          else
          {
            trans = obj->getWorldTransform();
          }

          Model *m = this->models[j];
          trans.getOpenGLMatrix(glm::value_ptr(m->matrix));
        }
        CProfileManager::Stop_Profile();
      }

      void addModel(Model * m, bool fixed = false) {
        this->addModelOBB(m, fixed);
        //this->addModelComposite(m, fixed);

        this->models.push_back(m);
      }

      void addModelOBB(Model * m, bool fixed = false) {
        vec3 hd = vec3(m->dims) * vec3(0.5);
        btCollisionShape* aabbShape = new btBoxShape(
          btVector3(hd.x, hd.y, hd.z)
        );
        collisionShapes.push_back(aabbShape);

        btTransform mat;
        mat.setIdentity();
        const vec3 bodyPos = m->getPosition() + hd;
        mat.setOrigin(btVector3(bodyPos.x, bodyPos.y, bodyPos.z));

        float mass = m->dims.x * m->dims.y * m->dims.z;
        btRigidBody *body = this->createRigidBody(
          fixed ? 0.0 : mass,
          mat,
          aabbShape
        );

        body->setUserPointer((void *)m);
      }

      void addModelComposite(Model * m, bool fixed = false) {
        vec3 hd = vec3(m->dims) * vec3(0.5);

        // TODO: build a composite shape out of the incoming model
        // TODO: compute the mass
        btScalar mass(fixed ? 0.0 : 1.0);
        btCompoundShape* compositeShape = new btCompoundShape();
        collisionShapes.push_back(compositeShape);
        uvec3 pos(0);

        vector<btScalar> masses;
        float totalMass = 0.0;

        // collect all surface voxels
        if (false) {
          for (pos.x = 0; pos.x < m->dims.x; pos.x+=2) {
            for (pos.y = 0; pos.y < m->dims.y; pos.y+=2) {
              for (pos.z = 0; pos.z < m->dims.z; pos.z+=2) {
                if (m->vox->getVoxel(pos) && m->vox->visible(pos)) {
                  // TODO: different mass based on material?
                  btTransform t;
                  t.setIdentity();
                  t.setOrigin(btVector3(
                    static_cast<btScalar>(pos.x) - hd.x,
                    static_cast<btScalar>(pos.y) - hd.y,
                    static_cast<btScalar>(pos.z) - hd.z
                  ));
                  totalMass += 1.0;
                  masses.push_back(fixed ? 0.0 : 1.0);
                  compositeShape->addChildShape(t, this->voxelShape);
                }
              }
            }
          }
        }

        // collect sheets
        if (true) {
          // find the largest axis or use 0
          u8 axis = 0;
          if (m->dims.y > m->dims[axis]) {
            axis = 1;
          }

          if (m->dims.z > m->dims[axis]) {
            axis = 2;
          }

          u8 otherA = (axis + 1) % 3;
          u8 otherB = (axis + 2) % 3;


          for (i32 i = 0; i < m->dims[axis]; i++) {
            // grow an aabb that covers all of the voxels in this slice
            ivec3 lb(m->dims[axis] * 2);
            ivec3 ub(m->dims[axis] * -2);
            lb[axis] = i;
            ub[axis] = i+1;

            uvec3 idx(0);
            idx[axis] = i;

            float mass = 0.0;
            for (i32 a = 0; a < m->dims[otherA]; a++) {
              for (i32 b = 0; b < m->dims[otherB]; b++) {
                idx[otherA] = a;
                idx[otherB] = b;

                if (m->vox->getVoxel(idx)) {
                  // TODO: different mass based on material?
                  mass += 1.0;
                  lb[otherA] = glm::min(a, lb[otherA]);
                  lb[otherB] = glm::min(b, lb[otherB]);

                  ub[otherA] = glm::max(a, ub[otherA]);
                  ub[otherB] = glm::max(b, ub[otherB]);
                }
              }
            }

            vec3 center = vec3(lb + ub)  * vec3(0.5);
            vec3 dims = vec3(ub - lb);
            vec3 hd = dims * vec3(0.5);

            if (any(lessThanEqual(hd, vec3(0.0)))) {
              continue;
            }

            btTransform t;
            t.setIdentity();
            t.setOrigin(btVector3(center.x, center.y, center.z));
            totalMass += 1.0;
            masses.push_back(fixed ? 0.0 : 1.0);
            compositeShape->addChildShape(t,
              new btBoxShape(btVector3(hd.x, hd.y, hd.z))
            );
          }
        }

        cout << "total boxes: " << masses.size() << "/" << (m->dims.x * m->dims.y * m->dims.z) << endl;

        btVector3 localInertia(0, 0, 0);
        btTransform xform;
        xform.setIdentity();
        if (!fixed) {
          compositeShape->calculatePrincipalAxisTransform(
            masses.data(),
            xform,
            localInertia
          );
        }

        btTransform invXform = xform.inverse();
        btCompoundShape* compoundActual = new btCompoundShape();
        for (int i = 0; i < compositeShape->getNumChildShapes(); i++) {
          compoundActual->addChildShape(
            compositeShape->getChildTransform(i) * invXform,
            compositeShape->getChildShape(i)
          );
        }

        delete compositeShape;

        btTransform mat;
        mat.setIdentity();
        const vec3 bodyPos = m->getPosition() + hd;
        mat.setOrigin(btVector3(bodyPos.x, bodyPos.y, bodyPos.z));
        this->createRigidBody(fixed ? 0.0 : totalMass, mat, compoundActual);
      }

      void debugProfile() {
        cout << "dump stats...\n";
        CProfileManager::dumpAll();
      }
  };
}