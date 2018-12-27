// TODO: move this into core.h or somewhere where we can share

struct Cell {
  uint state;
  uint start;
  uint end;
};

struct Level {
  Cell cells[4096];
};

struct SlabEntry {
  uint volume;
  ivec3 brickIndex;
  uint32_t *brickData;
};