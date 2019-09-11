#pragma once

#include <Common/Common.h>
#include <Simulation/Simulation.h>
#include <Simulation/SimulationModel.h>
#include <Simulation/TimeStepController.h>
#include <Simulation/TimeManager.h>
#include <Utils/Logger.h>
#include <Utils/Timing.h>


INIT_LOGGING
INIT_TIMING
//INIT_COUNTING

namespace voxviz {
  class Physics {
    public:
      u32 steps = 10;
      PBD::SimulationModel *simulationModel;
      PBD::TimeStepController *timeStepController;
      Physics() {
        this->simulationModel = new PBD::SimulationModel();
        this->simulationModel->init();

        this->timeStepController = new PBD::TimeStepController();

        PBD::TimeManager::getCurrent()->setTimeStepSize(static_cast<Real>(0.005));


      }

      Physics *step() {
        for (u32 i = 0; i < this->steps; i++) {
          START_TIMING("SimStep");
          PBD::Simulation::getCurrent()->getTimeStep()->step(*this->simulationModel);
          STOP_TIMING_AVG;
        }
        return this;
      }
  };
}