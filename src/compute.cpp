#include "compute.h"

Compute::Compute() {
  clu_compute_init(&this->job);
}

Compute::~Compute() {
  clu_compute_destroy(&this->job);
}
