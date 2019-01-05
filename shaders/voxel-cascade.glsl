// TODO: move this into core.h or somewhere where we can share

struct Cell {
  uint state;
  uint start;
  uint end;
};

struct SlabEntry {
  uint volume;
  ivec3 brickIndex;
  uint32_t *brickData;
};

bool voxel_cascade_get(in Cell *cascade_index, in uint levelIdx, in ivec3 pos) {
  if (levelIdx >= TOTAL_VOXEL_CASCADE_LEVELS) {
    return false;
  }

  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(BRICK_DIAMETER)))) {
    return false;
  }

  uint idx = (levelIdx*BRICK_VOXEL_COUNT + pos.x + pos.y * BRICK_DIAMETER + pos.z * BRICK_DIAMETER * BRICK_DIAMETER);
  Cell cell = cascade_index[idx];
  return cell.state > 0;
}
