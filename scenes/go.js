const obj = {
  pipelines: {
    setup: [{
      program: {
        files: ['fill-all-voxels.comp'],
        type: "compute",
        inputs: {
          voxelBuffer: {type: "buffer", id: "voxels" }
        },
        dims: [100, 100, 100]
      }
    }],
    frame: [{
      program: {
        files: ['raytrace.comp'],
        type: "compute",
        buffers: {
          voxelBuffer: "voxels"
        },
        dims: [
          { name: "window.width", type: "uint" },
          { name: "window.width", type: "uint" },
          0
        ],
      }
    },
    {
      program: {
        files: ['blit.vert', 'blit.frag'],
        type: "render",
        buffers: {
          voxels: "voxels"
        }
      }
    }
  ]
  },
  buffers: {
    voxels: {
      dims: [100, 100, 100],
      slotSize: 4,
      type: "SSBO"
    },
    rayTermination: {
      dims: [
        { name: "window.width", type: "uint" },
        { name: "window.width", type: "uint" },
        0
      ],
      slotSize: 64,
      type: "SSBO"
    }
  }
}

console.log(JSON.stringify(obj))
