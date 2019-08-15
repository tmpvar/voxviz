#pragma once

#include "gl-wrap.h"
//#include <openvdb/openvdb.h>


//using GridType = openvdb::FloatGrid;
//using TreeType = GridType::TreeType;

class SplatBuffer {
  public:
    SSBO *ssbo = nullptr;
    size_t splat_count = 0;
    Splat *data = nullptr;
    glm::mat4 model;
    size_t max_splats;
    SplatBuffer(bool startMap = false) {
      this->max_splats = (1 << 22);
      this->ssbo = new SSBO(sizeof(Splat) * max_splats);
      if (startMap) {
        this->data = (Splat *)ssbo->beginMapPersistent();
      }
      this->model = glm::mat4(1.0);
    }

    ~SplatBuffer() {
      this->ssbo->endMap();
      delete this->ssbo;
    }
};


class Splats {


  // buffers
  SSBO *voxelSpaceSSBO;

  SSBO *scratchBuffer;
  SSBO *indirectRenderBuffer[MAX_MIP_LEVELS+1];
  SSBO *indirectComputeBuffer[MAX_MIP_LEVELS+1];
  SSBO *instanceBuffer;
  SSBO *bucketsBuffer;

  SSBO *mipBuckets[MAX_MIP_LEVELS+2];

  public:
    // programs
    Program *computeIndirectBuffer;
    Program *computeCollectSeed;
    Program *computeCollectWork;
    Program *rasterSplats;

    Splats(SSBO *voxelSpaceSSBO) {
      this->voxelSpaceSSBO = voxelSpaceSSBO;

      // Programs
      {
        this->computeIndirectBuffer = Program::New()
          ->add(Shaders::get("splats-generate-indirect.comp"))
          ->link();

        this->computeCollectSeed = Program::New()
          ->add(Shaders::get("splats/extract-seed.comp"))
          ->link();

        this->computeCollectWork = Program::New()
          ->add(Shaders::get("splats/extract-work.comp"))
          ->link();

        this->rasterSplats = Program::New()
          ->add(Shaders::get("splats/raster.vert"))
          ->add(Shaders::get("splats/raster.frag"))
          ->output("outColor")
          ->link();
       }

      this->scratchBuffer = new SSBO(this->voxelSpaceSSBO->size() >> 3);

      // TODO: this might be confusing later -- this is actually a buffer that is fed into
      //       draw arrays
      this->instanceBuffer = new SSBO(sizeof(Splat) * SPLATS_MAX);
      this->bucketsBuffer = new SSBO(sizeof(SplatBucket) * SPLAT_BUCKETS);

      // build a set of mip level indirect buffers
      {
        for (uint8_t mip = 0; mip <= MAX_MIP_LEVELS; mip++) {
          cout << "splats: generate indirect render buffer" << endl;
          this->indirectRenderBuffer[mip] = new SSBO(sizeof(DrawArraysIndirectCommand));
          cout << "splats: generate indirect compute buffer" << endl;
          this->indirectComputeBuffer[mip] = new SSBO(sizeof(DispatchIndirectCommand));
        }
      }

      // build a set of mip level splat buckets
      {
        cout << "begin splat buckets for mips" << endl;
        uvec3 voxelSpaceDims = this->voxelSpaceSSBO->dims();
        for (uint8_t mip = 0; mip <= MAX_MIP_LEVELS; mip++) {
          glm::uvec3 bucketDims = voxelSpaceDims / glm::uvec3(1<<mip);

          uint64_t bucketBytes =
            static_cast<uint64_t>(bucketDims.x) *
            static_cast<uint64_t>(bucketDims.y) *
            static_cast<uint64_t>(bucketDims.z);

          if (mip == 0) {
            bucketBytes /= 4;
          }
          this->mipBuckets[mip] = new SSBO(bucketBytes * sizeof(Splat), bucketDims);
        }
      }
    }

    void tick() {
      uvec3 voxelSpaceDims = this->voxelSpaceSSBO->dims();

      // Reset mip level splat counters
      {
        for (uint8_t mip = 0; mip <= MAX_MIP_LEVELS; mip++) {
          SSBO *buf = this->indirectRenderBuffer[mip];
          DrawArraysIndirectCommand *b = (DrawArraysIndirectCommand *)buf->beginMap(SSBO::MAP_WRITE_ONLY);
          b->baseInstance = 0;
          b->count = 0;
          b->first = 0;
          b->primCount = 1;
          buf->endMap();
        }
      }
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // Reset mip level indirect compute counters
      {
        for (uint8_t mip = 0; mip <= MAX_MIP_LEVELS; mip++) {
          SSBO *buf = this->indirectComputeBuffer[mip];
          DispatchIndirectCommand *b = (DispatchIndirectCommand *)buf->beginMap(SSBO::MAP_WRITE_ONLY);
          b->num_groups_x = 0;
          b->num_groups_y = 1;
          b->num_groups_z = 1;
          buf->endMap();
        }
      }

      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // Hierarchical collection of splats using the dense octree
      if (true) {
        /*
          TODO:
            - setup indirect compute buffer and initialize it w/ highest mip level (all cells active)
            - loop over all mips starting at highest (CPU)
              - for each visible cell
                - increment the next indirect compute buffer's groups - likely has to be converted to a 1d problem.. more like a job queue
                - add a splat to the current mip's splat buffer (must include grid index)
                - add it to the next indirect compute buffer add it to a buffer that tracks the offset + length into the splat buffer
              -
          4 [                              ][                              ] \
          3 [              ][              ][              ][              ]  \
          2 [      ][      ][      ][      ][      ][      ][      ][      ]   } [4][4][3][3][3][3][2]...
          1 [  ][  ][  ][  ][  ][  ][  ][  ][  ][  ][  ][  ][  ][  ][  ][  ]  /
          0 [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][] /

        */

        // DEBUG: collect the smallest splat bucket (mip 7)
        if (false) {
          glm::uvec3 mipDims = voxelSpaceDims / uvec3(1<<MAX_MIP_LEVELS);
          glm::uvec3 lowerMipDims = voxelSpaceDims / (glm::uvec3(1 << (MAX_MIP_LEVELS - 1)));

          this->computeCollectSeed
            ->use()
            ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
            ->uniformVec3("volumeSlabDims", voxelSpaceDims)

            ->ssbo("splatIndirectCommandBuffer", this->indirectRenderBuffer[MAX_MIP_LEVELS])
            ->ssbo("splatInstanceBuffer", this->mipBuckets[MAX_MIP_LEVELS])

            ->uniformVec3ui("mipDims", mipDims)
            ->uniform1ui("mipLevel", MAX_MIP_LEVELS)
             ->timedCompute("collect splats seed", mipDims);
        } else {
          // TODO: this is an uber hack while I try to get something to work.
          // I'm hoping that some patterns emerge as I get further into this
          // to aid in refactoring.

          // The basic gist here is that this is my attempt at using indirect compute.
          // My theory is that things can be optimized quite well if we generate work
          // in upper mips and pass the exact amount of work that needs to be done down
          // to lower mips.
          //
          // so the flow is like this:
          // - generate work for mip by "seeding" the pipeline and follow the general flow below
          //   - generate a list of voxels that are visible from at least one direction
          //   - atomically increment the next mip level's workgroup count (by 8 to
          //     include 2^2 children)
          // - run the next mip using the previous mip's output and generate work using the above approach
          //
          // Sounds easy in practice but this has been quite brain melting and I'm sure there is an off by one
          // (or two) in here.

          // Seed work from the highest mip level
          {
            int mip = MAX_MIP_LEVELS;
            uvec3 highestLevelMipDims = voxelSpaceDims / uvec3(1 << mip);
            this->computeCollectSeed
              ->use()
              ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
              ->uniformVec3("volumeSlabDims", voxelSpaceDims)
              ->ssbo("splatIndirectRenderBuffer", this->indirectRenderBuffer[mip])
              ->ssbo("splatInstanceBuffer", this->mipBuckets[mip])
              ->ssbo("nextSplatIndirectComputeBuffer", this->indirectComputeBuffer[mip-1])
              ->uniformVec3ui("mipDims", highestLevelMipDims)
              ->uniform1ui("mipLevel", mip)
              ->timedCompute("collect splats work", highestLevelMipDims);
          }

          // Work through the rest of the mips using the results of the previous
          {
            for (int mip = MAX_MIP_LEVELS-1; mip >= 0; mip--) {
              glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
              glm::uvec3 mipDims = voxelSpaceDims / uvec3(1 << mip);

              stringstream s;
              s << "extract splats" << mip;

              this->computeCollectWork
                ->use()
                ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
                ->uniformVec3("volumeSlabDims", voxelSpaceDims)

                ->ssbo("splatIndirectRenderBuffer", this->indirectRenderBuffer[mip])
                ->ssbo("splatInstanceBuffer", this->mipBuckets[mip])

                ->ssbo("prevSplatInstanceBuffer", this->mipBuckets[mip + 1])

                ->uniformVec3ui("mipDims", mipDims)
                ->uniform1ui("mipLevel", mip);
              if (mip > 0) {
                this->computeCollectWork->ssbo("nextSplatIndirectComputeBuffer", this->indirectComputeBuffer[mip - 1]);
              }
              this->computeCollectWork->timedIndirectCompute(s.str().c_str(), this->indirectComputeBuffer[mip]);
              #if 0
              glMemoryBarrier(GL_ALL_BARRIER_BITS);
              DrawArraysIndirectCommand *b = (DrawArraysIndirectCommand *)this->indirectRenderBuffer[mip]->beginMap(SSBO::MAP_READ_ONLY);
              ImGui::Text("mip: %lu; %lu splats; %lu", mip, b->count, b->primCount);
              this->indirectRenderBuffer[mip]->endMap();
              #endif

            }
          }
        }
      }

      // Splat collection at lowest level
      if (false) {
        this->computeIndirectBuffer
          ->use()
          ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
          ->uniformVec3("volumeSlabDims", voxelSpaceDims)
          ->ssbo("splatIndirectCommandBuffer", this->indirectRenderBuffer[2])
          ->ssbo("splatInstanceBuffer", this->mipBuckets[2])
          ->ssbo("splatBucketBuffer", this->bucketsBuffer)
          ->timedCompute("splats: extraction", voxelSpaceDims / uvec3(2));

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      }
    }

    void render(const mat4 &mvp, const vec3 &eye, const uvec2 &screenResolution, float fov) {
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      glEnable(GL_PROGRAM_POINT_SIZE);

      if (false) {
        int mip = 1;
        this->indirectRenderBuffer[mip]->bind(GL_DRAW_INDIRECT_BUFFER);
        this->rasterSplats
          ->use()
          ->uniformMat4("mvp", mvp)
          ->uniformVec3("eye", eye)
          ->uniformFloat("fov", fov)
          ->uniformVec2ui("res", screenResolution)
          ->uniform1ui("mipLevel", mip)
          ->ssbo("splatInstanceBuffer", this->mipBuckets[mip]);

        glDrawArraysIndirect(GL_POINTS, nullptr);
        gl_error();
      }
      else {
        for (int mip = 0; mip >= 0; mip--) {
          GLuint query;
          GLuint64 elapsed_time;
          GLint done = 0;
          glGenQueries(1, &query);
          glBeginQuery(GL_TIME_ELAPSED, query);

          this->indirectRenderBuffer[mip]->bind(GL_DRAW_INDIRECT_BUFFER);
          this->rasterSplats
            ->use()
            ->uniformMat4("mvp", mvp)
            ->uniformVec3("eye", eye)
            ->uniformFloat("fov", fov)
            ->uniformVec2ui("res", screenResolution)
            ->uniform1ui("mipLevel", mip)
            ->ssbo("splatInstanceBuffer", this->mipBuckets[mip]);

          glDrawArraysIndirect(GL_POINTS, nullptr);
          gl_error();

          this->indirectRenderBuffer[mip]->unbind(GL_DRAW_INDIRECT_BUFFER);

          glEndQuery(GL_TIME_ELAPSED);
          while (!done) {
            glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
          }

          // get the query result
          glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
          ImGui::Text("splat render: %.3f.ms", elapsed_time / 1000000.0);
          glDeleteQueries(1, &query);
        }
      }
    }
};