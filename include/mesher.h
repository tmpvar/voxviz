#ifndef H_VX_MESHER
#define H_VX_MESHER

  #include <cstdio>
  #include <vector>

  #ifndef vx_mesher
    #define vx_mesher vx_stupid_mesher
  #endif

  struct vector3 {
    vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    float x, y, z;
  };

  void vx_stupid_mesher(
    const int *volume,
    const int *dims,
    std::vector<float> &vertices,
    std::vector<unsigned int> &faces
  );

  void vx_greedy_mesher(
    const int *volume,
    const int *dims,
    std::vector<float> &vertices,
    std::vector<unsigned int> &faces
  );
#endif
