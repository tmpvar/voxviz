#include "aabb.h"
#include <iostream>
using namespace std;

void aabb_print(aabb_t v) {
  cout << "lower("
    << v.lower.x << ", "
    << v.lower.y << ", "
    << v.lower.z << ") upper("
    << v.upper.x << ", "
    << v.upper.y << ", "
    << v.upper.z << ")"
    << endl << endl;
}