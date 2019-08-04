#pragma once

#include "gl-wrap.h"

class Splats {
  // programs
  Program *computeIndirectBuffer;
  Program *computeCollectSeed;
  Program *rasterSplats;

  // buffers
  SSBO *voxelSpaceSSBO;

  SSBO *scratchBuffer;
  SSBO *indirectBuffer[MAX_MIP_LEVELS+1];
  SSBO *instanceBuffer;
  SSBO *bucketsBuffer;

  SSBO *mipBuckets[MAX_MIP_LEVELS+1];

  public:
    Splats(SSBO *voxelSpaceSSBO) {
      this->voxelSpaceSSBO = voxelSpaceSSBO;

      // Splats
      this->computeIndirectBuffer = Program::New()
        ->add(Shaders::get("splats-generate-indirect.comp"))
        ->link();

      this->computeCollectSeed = Program::New()
        ->add(Shaders::get("splats/extract-seed.comp"))
        ->link();

      this->rasterSplats = Program::New()
        ->add(Shaders::get("splats/raster.vert"))
        ->add(Shaders::get("splats/raster.frag"))
        ->output("outColor")
        ->link();

      this->scratchBuffer = new SSBO(this->voxelSpaceSSBO->size() >> 3);

      // TODO: this might be confusing later -- this is actually a buffer that is fed into
      //       draw arrays
      this->instanceBuffer = new SSBO(sizeof(Splat) * SPLATS_MAX);
      this->bucketsBuffer = new SSBO(sizeof(SplatBucket) * SPLAT_BUCKETS);

      // build a set of mip level splat buckets
      {
        cout << "begin splat buckets for mips" << endl;
        uvec3 voxelSpaceDims = this->voxelSpaceSSBO->dims();
        for (uint8_t mip = 1; mip <= MAX_MIP_LEVELS; mip++) {

          this->indirectBuffer[mip] = new SSBO(sizeof(DrawArraysIndirectCommand));

          glm::uvec3 bucketDims = voxelSpaceDims / glm::uvec3(1<<mip);

          uint64_t bucketBytes =
            static_cast<uint64_t>(bucketDims.x) *
            static_cast<uint64_t>(bucketDims.y) *
            static_cast<uint64_t>(bucketDims.z);

          this->mipBuckets[mip] = new SSBO(bucketBytes * sizeof(Splat), bucketDims);
        }
      }
    }

    void extract() {
      uvec3 voxelSpaceDims = this->voxelSpaceSSBO->dims();

      // Reset mip level splat counters
      {
        for (uint8_t mip = 1; mip <= MAX_MIP_LEVELS; mip++) {
          DrawArraysIndirectCommand *b = (DrawArraysIndirectCommand *)this->indirectBuffer[mip]->beginMap(SSBO::MAP_WRITE_ONLY);
          b->baseInstance = 0;
          b->count = 0;
          b->first = 0;
          b->primCount = 1;
          this->indirectBuffer[mip]->endMap();
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

        if (true) {
          glm::uvec3 mipDims = voxelSpaceDims / uvec3(1<<MAX_MIP_LEVELS);
          glm::uvec3 lowerMipDims = voxelSpaceDims / (glm::uvec3(1 << (MAX_MIP_LEVELS - 1)));

          this->computeCollectSeed
            ->use()
            ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
            ->uniformVec3("volumeSlabDims", voxelSpaceDims)

            ->ssbo("splatIndirectCommandBuffer", this->indirectBuffer[MAX_MIP_LEVELS])
            ->ssbo("splatInstanceBuffer", this->mipBuckets[MAX_MIP_LEVELS])
            //->ssbo("splatBucketBuffer", this->bucketsBuffer)

            ->uniformVec3ui("mipDims", mipDims)
            ->uniformVec3ui("lowerMipDims", lowerMipDims)
            ->uniform1ui("mipLevel", MAX_MIP_LEVELS)
            ->uniform1ui("lowerMipLevel", MAX_MIP_LEVELS - 1)
            // launch with x2 because we want enough threads to process
            // the lower mip cells
            ->timedCompute("collect splats seed", mipDims * uvec3(2));
        }

        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        DrawArraysIndirectCommand *b = (DrawArraysIndirectCommand *)this->indirectBuffer[MAX_MIP_LEVELS]->beginMap(SSBO::MAP_READ_ONLY);
        ImGui::Text("%lu splats; %lu", b->count, b->primCount);
        this->indirectBuffer[MAX_MIP_LEVELS]->endMap();

        /*for (unsigned int i = MAX_MIP_LEVELS - 1; i > 0; i--) {
          glm::uvec3 mipDims = voxelSpaceDims / (glm::uvec3(1 << i));
          glm::uvec3 lowerMipDims = voxelSpaceDims / (glm::uvec3(1 << (i - 1)));
          ostringstream mipDebug;

          mipDebug << "collect splats " << i << " dims: " << mipDims.x << "," << mipDims.y << "," << mipDims.z;

          mipmapVoxelSpace
            ->use()
            ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
            ->uniformVec3("volumeSlabDims", voxelSpaceDims)

            ->uniformVec3ui("mipDims", mipDims)
            ->uniformVec3ui("lowerMipDims", lowerMipDims)
            ->uniform1ui("mipLevel", i)
            ->uniform1ui("lowerMipLevel", i - 1)
            ->timedCompute(mipDebug.str().c_str(), mipDims);
          gl_error();
          // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
        }*/
      }

      // Splat collection at lowest level
      if (true) {
        this->computeIndirectBuffer
          ->use()
          ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
          ->uniformVec3("volumeSlabDims", voxelSpaceDims)
          ->ssbo("splatIndirectCommandBuffer", this->indirectBuffer[2])
          ->ssbo("splatInstanceBuffer", this->mipBuckets[2])
          ->ssbo("splatBucketBuffer", this->bucketsBuffer)
          ->timedCompute("splats: extraction", voxelSpaceDims / uvec3(2));

        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      }
    }

    void render(const mat4 &mvp, const vec3 &eye, const uvec2 &screenResolution, float fov) {
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      glEnable(GL_PROGRAM_POINT_SIZE);

      for (int mip = MAX_MIP_LEVELS; mip >= 1; mip--) {
        GLuint query;
        GLuint64 elapsed_time;
        GLint done = 0;
        glGenQueries(1, &query);
        glBeginQuery(GL_TIME_ELAPSED, query);


        this->indirectBuffer[mip]->bind(GL_DRAW_INDIRECT_BUFFER);
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

        this->indirectBuffer[mip]->unbind(GL_DRAW_INDIRECT_BUFFER);

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
};