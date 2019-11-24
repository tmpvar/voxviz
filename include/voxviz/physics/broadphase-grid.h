#pragma once

#include <BulletCollision/BroadphaseCollision/btBroadphaseInterface.h>
#include <BulletCollision/BroadphaseCollision/btOverlappingPairCache.h>
#include <BulletCollision/BroadphaseCollision/btBroadphaseProxy.h>
#include "core.h"


namespace voxviz {
  namespace physics {
    struct GridCell {
      u32 objectId;
      u32 time;
    };

    inline const btBroadphaseProxy* castProxy(btBroadphaseProxy* proxy) {
      return static_cast<const btBroadphaseProxy*>(proxy);
    }

    struct BroadphaseProxy : public btBroadphaseProxy
    {
      BroadphaseProxy() {};

      BroadphaseProxy(
        const btVector3& minpt,
        const btVector3& maxpt,
        int shapeType,
        void* userPtr,
        int collisionFilterGroup,
        int collisionFilterMask
      )
      : btBroadphaseProxy(minpt, maxpt, userPtr, collisionFilterGroup, collisionFilterMask)
      {
        (void)shapeType;
      }

      //SIMD_FORCE_INLINE void SetNextFree(int next) { m_nextFree = next; }
      //SIMD_FORCE_INLINE int GetNextFree() const { return m_nextFree; }
    };


    #define INVALID_POSITION 0xFFFFFFFF

    class BroadphaseGrid: public btBroadphaseInterface {
      private:
        GridCell *grid;
        uvec3 dims;
        u32 time = 1;
        btOverlappingPairCache* pairCache;
        vector<BroadphaseProxy *> proxies;

        bool oob(uvec3 pos) {
          return any(greaterThanEqual(pos, this->dims));
        }

        u64 voxelIndex(uvec3 pos) {
          return pos.x + pos.y * dims.x + pos.z * dims.x * dims.y;
        }

        GridCell *get(uvec3 pos) {
          if (this->oob(pos)) {
            return nullptr;
          }

          return &this->grid[this->voxelIndex(pos)];
        }

        u32 set(uvec3 pos, u32 objectId) {
          if (this->oob(pos)) {
            return INVALID_POSITION;
          }

          GridCell *old = this->get(pos);
          if (old->objectId != objectId && old->time >= this->time) {
            return old->objectId;
          }

          GridCell *n = &this->grid[this->voxelIndex(pos)];
          n->objectId = objectId;
          n->time = this->time;

          return objectId;
        }

      public:
        BroadphaseGrid(uvec3 worldDims) {
          this->dims = worldDims;
          u64 cells = worldDims.x * worldDims.y * worldDims.z;
          u64 total_size = sizeof(GridCell) * cells;
          this->grid = (GridCell *)malloc(total_size);
          memset(this->grid, 0, total_size);

          void* mem = btAlignedAlloc(sizeof(btHashedOverlappingPairCache), 16);
          this->pairCache = new (mem) btHashedOverlappingPairCache();
        }

        void getAabb(btBroadphaseProxy* proxy, btVector3& aabbMin, btVector3& aabbMax) const {
          aabbMin = proxy->m_aabbMin;
          aabbMax = proxy->m_aabbMax;
        }

        void setAabb(
          btBroadphaseProxy* proxy,
          const btVector3& aabbMin,
          const btVector3& aabbMax,
          btDispatcher* dispatcher
        )
        {
          proxy->m_aabbMin = aabbMin;
          proxy->m_aabbMax = aabbMax;
        }

        bool aabbOverlap(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) {
          return (
            proxy0->m_aabbMin[0] <= proxy1->m_aabbMax[0] &&
            proxy1->m_aabbMin[0] <= proxy0->m_aabbMax[0] &&
            proxy0->m_aabbMin[1] <= proxy1->m_aabbMax[1] &&
            proxy1->m_aabbMin[1] <= proxy0->m_aabbMax[1] &&
            proxy0->m_aabbMin[2] <= proxy1->m_aabbMax[2] &&
            proxy1->m_aabbMin[2] <= proxy0->m_aabbMax[2]
          );
        }

        btBroadphaseProxy *createProxy(
          const btVector3& aabbMin,
          const btVector3& aabbMax,
          int shapeType,
          void* userPtr,
          int collisionFilterGroup,
          int collisionFilterMask,
          btDispatcher* dispatcher
        ) {
          BroadphaseProxy *p = new BroadphaseProxy(
            aabbMin,
            aabbMax,
            shapeType,
            userPtr,
            collisionFilterGroup,
            collisionFilterMask
          );

          this->proxies.push_back(p);

          return p;
        }

        void destroyProxy(btBroadphaseProxy* proxyOrg, btDispatcher* dispatcher) {
          this->pairCache->removeOverlappingPairsContainingProxy(proxyOrg, dispatcher);
          BroadphaseProxy *p = static_cast<BroadphaseProxy *>(proxyOrg);
          cout << "destroyProxy not implemented" << endl;
          exit(1);

          delete p;
        }


        void calculateOverlappingPairs(btDispatcher* dispatcher) {
          i32 i = -1;
          for (auto &p : this->proxies) {
            i++;
            if (!p->m_clientObject) {
              continue;
            }

            btRigidBody *co = (btRigidBody *)p->m_clientObject;
            Model *m = (Model *)co->getUserPointer();
            btTransform tx;
            co->getMotionState()->getWorldTransform(tx);

            uvec3 idx(0);
            u32 objectId = co->getUserIndex();
            u32 otherId = this->set(idx, objectId);
            if (otherId == INVALID_POSITION || otherId == objectId) {
              continue;
            }

            for (auto &other : this->proxies) {
              if (other == p || !other->m_clientObject) {
                continue;
              }

              if (((btCollisionObject *)other->m_clientObject)->getUserIndex() != otherId) {
                continue;
              }

              this->pairCache->addOverlappingPair(p, other);
            }
          }
        }

        void rayTest(const btVector3& rayFrom, const btVector3& rayTo, btBroadphaseRayCallback& rayCallback, const btVector3& aabbMin = btVector3(0, 0, 0), const btVector3& aabbMax = btVector3(0, 0, 0)) {
          for (BroadphaseProxy* proxy : this->proxies) {
            if (!proxy->m_clientObject)
            {
              continue;
            }
            rayCallback.process(proxy);
          }
        }

        btOverlappingPairCache* getOverlappingPairCache() {
          return this->pairCache;
        }

        const btOverlappingPairCache* getOverlappingPairCache() const {
          return this->pairCache;
        }

        ///getAabb returns the axis aligned bounding box in the 'global' coordinate frame
        ///will add some transform later
        void getBroadphaseAabb(btVector3& aabbMin, btVector3& aabbMax) const {
          aabbMin.setValue(-BT_LARGE_FLOAT, -BT_LARGE_FLOAT, -BT_LARGE_FLOAT);
          aabbMax.setValue(BT_LARGE_FLOAT, BT_LARGE_FLOAT, BT_LARGE_FLOAT);
        };

        ///reset broadphase internal structures, to ensure determinism/reproducability
        virtual void resetPool(btDispatcher* dispatcher) { (void)dispatcher; };
        void printStats() {

        };

        void aabbTest(const btVector3& aabbMin, const btVector3& aabbMax, btBroadphaseAabbCallback& callback) {
          for (BroadphaseProxy *proxy : this->proxies) {
            if (!proxy->m_clientObject) {
              continue;
            }

            if (TestAabbAgainstAabb2(aabbMin, aabbMax, proxy->m_aabbMin, proxy->m_aabbMax)) {
              callback.process(proxy);
            }
          }
        }
    };
  }
}