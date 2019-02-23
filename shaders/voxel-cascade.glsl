// TODO: move this into core.h or somewhere where we can share

struct Cell {
  uint state;
  uint start;
  uint end;
  uint _padding;
};

struct SlabEntry {
  mat4 invTransform;       // 64 (16)
  ivec4 brickIndex;     // 16 (4)
  uint32_t *brickData;  // 8 (2)
  uint8_t _padding[8];
};

struct VoxelCascadeTraversalState {
  uint level;

  uvec3 index;

  vec3 worldPos;
  // Is the current cell empty?
  bool empty;
  // Is this a step iteration or a cascade traversal
  bool step;
  Cell cell;
};

bool voxel_cascade_get(in Cell *cascade_index, in uint levelIdx, in ivec3 pos, out Cell found) {
  if (levelIdx >= TOTAL_VOXEL_CASCADE_LEVELS) {
    return false;
  }

  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return false;
  }

  uint idx = (levelIdx*BRICK_VOXEL_COUNT + pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  found = cascade_index[idx];
  return found.state > 0;
}

ivec3 index_pos_to_grid_pos(ivec3 vec, uint val) {
  ivec3 signed_grid_pos = sign(vec) * ivec3((uvec3(abs(vec)) >> val));
  return ivec3(BRICK_RADIUS) + signed_grid_pos;
}

// TODO: should state be modified or should we build an new state object to return?
bool voxel_cascade_next(in Cell *cascade_index, inout VoxelCascadeTraversalState state) {
  int cellSize = 1 << uint(state.level + 1);
  int lowerCellSize = 1 << uint(state.level);

  bool canGoUp = state.level < TOTAL_VOXEL_CASCADE_LEVELS;
  bool canGoDown = state.level > 0 && all(lessThan(state.index >> 1, uvec3(BRICK_DIAMETER)));
  bool canStep = all(lessThan(state.index, uvec3(BRICK_DIAMETER)));

  state.empty = !voxel_cascade_get(
    cascade_index,
    state.level,
    ivec3(state.index),
    state.cell
  );

  Cell upperCell;
  bool upperEmpty = canGoUp && !voxel_cascade_get(
    cascade_index,
    state.level+1,
    ivec3(state.index << 1),
    upperCell
  );

  int levelChange = !state.empty
  ? -int(canGoDown)
  : int(canGoUp && upperEmpty);

  state.level += levelChange;
  state.step = levelChange == 0;// && (!upperEmpty || !canGoUp);

  // return termination criteria for cascade traversal
  return any(greaterThanEqual(
    state.index << state.level,
    uvec3(BRICK_DIAMETER * (1 << TOTAL_VOXEL_CASCADE_LEVELS))
  ));
}
