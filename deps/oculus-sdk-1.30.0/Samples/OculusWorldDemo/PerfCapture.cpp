/************************************************************************************
  Created     :   Jun 25, 2018
  Copyright   :   Copyright 2018 Oculus, Inc. All Rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*************************************************************************************/

#include "PerfCapture.h"
#include "Extras/OVR_Math.h"
#include "Kernel/OVR_System.h"
#include <iosfwd>
#include <sstream>
#include <string>
#include <cerrno>

//////////////////////////////////////////////////////////////////////////
/// PerfCaptureSerializer
//////////////////////////////////////////////////////////////////////////

PerfCaptureSerializer::PerfCaptureSerializer(
    double totalDuration,
    Units units,
    const std::string& filePath)
    : CompletionValue(0.0),
      TotalDuration(totalDuration),
      DurationUnits(units),
      FilePath(filePath),
      CurrentStatus(Status::None),
      BufferedPerfStats(),
      CurrentFrame(0) {}

void PerfCaptureSerializer::Reset() {
  if (CurrentStatus == Status::Started) {
    EndCapture();
  }

  // Return to the newly constructed state.
  FilePath.clear();
  CompletionValue = 0.0;
  CurrentStatus = Status::None;
  TotalDuration = 0;
  DurationUnits = Units::none;

  if (SerializedFile.is_open()) {
    SerializedFile.close();
    SerializedFile.clear();
  }

  CurrentFrame = 0;
}

void PerfCaptureSerializer::SerializeBuffer() {
  if (CurrentStatus == Status::Started) {
    char* byteData = reinterpret_cast<char*>(&BufferedPerfStats[0]);
    size_t dataSize = BufferedPerfStats.size() * sizeof(ovrPerfStats);
    SerializedFile.write(byteData, dataSize);
  } else {
    OVR_FAIL();
  }

  // clear it out regardless to avoid memory bloat
  BufferedPerfStats.clear();
}

void PerfCaptureSerializer::StartCapture() {
  bool success = false;
  try {
    // create/open file
    SerializedFile.open(FilePath, std::ios::out | std::ios::binary);

    // ready buffer vector
    BufferedPerfStats.clear();
    BufferedPerfStats.reserve(NumBufferedStatsReserve);
    success = true;
  } catch (std::ios_base::failure&) {
    OVR_FAIL();
    success = false;
  }

  CurrentStatus = success ? Status::Started : Status::Error;
}

void PerfCaptureSerializer::EndCapture() {
  if (CurrentStatus == Status::Started) {
    if (BufferedPerfStats.size() > 0) {
      SerializeBuffer();
    }

    CurrentStatus = Status::Complete;
  } else {
    CurrentStatus = Status::Error; // called end before start
  }

  if (SerializedFile.is_open()) {
    SerializedFile.close();
  }
}

PerfCaptureSerializer::Status PerfCaptureSerializer::Step(ovrSession session) {
  if (CurrentStatus == Status::None) {
    StartCapture();
  }

  if (CurrentStatus == Status::Started) {
    ovrPerfStats perfStats = {};
    ovr_GetPerfStats(session, &perfStats);

    if (perfStats.FrameStatsCount > 0) {
      CurrentFrame = perfStats.FrameStats[0].AppFrameIndex;
    }

    bool timeIsUp = false;

    // make sure we got some stats before we start
    if (CurrentFrame > 0) {
      BufferedPerfStats.push_back(perfStats);

      if (CompletionValue == 0) // If the capture hasn't started yet...
      {
        if (DurationUnits == PerfCaptureSerializer::Units::ms) {
          CompletionValue = (ovr_GetTimeInSeconds() + (TotalDuration / 1000));
        } else if (DurationUnits == PerfCaptureSerializer::Units::s) {
          CompletionValue = (ovr_GetTimeInSeconds() + TotalDuration);
        } else if (DurationUnits == PerfCaptureSerializer::Units::frames) {
          CompletionValue = (CurrentFrame + TotalDuration);
        } else {
          OVR_FAIL();
        }
      }

      if ((DurationUnits == PerfCaptureSerializer::Units::ms) ||
          (DurationUnits == PerfCaptureSerializer::Units::s)) {
        if (ovr_GetTimeInSeconds() >= CompletionValue) // If done
        {
          timeIsUp = true;
        }
      } else // PerfCaptureSerializer::Units::frames
      {
        if (CurrentFrame >= CompletionValue) // If done
        {
          timeIsUp = true;
        }
      }
    }

    if (timeIsUp) {
      EndCapture();
    } else {
      bool shouldSerialize = (int)BufferedPerfStats.size() > NumBufferedStatsSerializeTrigger;
      if (shouldSerialize) {
        SerializeBuffer();
      }
    }
  }

  return CurrentStatus;
}

//////////////////////////////////////////////////////////////////////////
/// PerfCaptureSnapshot
//////////////////////////////////////////////////////////////////////////

PerfCaptureSnapshot::PerfCaptureSnapshot() : PerfStats(), FilePath(), LoadSuccess(false) {}

void PerfCaptureSnapshot::Reset() {}

bool PerfCaptureSnapshot::LoadSnapshot(const std::string& filePath) {
  LoadSuccess = false;

  FilePath = filePath;

  std::ifstream SerializedFile;
  SerializedFile.open(filePath, std::ios::binary);

  if (!SerializedFile.is_open()) {
    Reset();
    LoadSuccess = false;
  } else {
    // now deserialize data
    size_t fileSize = 0;
    {
      std::ifstream fileSizeCheck;
      fileSizeCheck.open(filePath, std::ios::binary | std::ios::ate);
      fileSize = (size_t)fileSizeCheck.tellg();
    }

    size_t numPerfStats = (fileSize / sizeof(ovrPerfStats));
    PerfStats.resize(numPerfStats);

    for (size_t curStatIdx = 0; curStatIdx < numPerfStats; ++curStatIdx) {
      ovrPerfStats& curStatBuffer = PerfStats[curStatIdx];
      SerializedFile.read((char*)(&curStatBuffer), sizeof(curStatBuffer));
    }

    LoadSuccess = true;
  }

  return LoadSuccess;
}

//////////////////////////////////////////////////////////////////////////
/// PerfCaptureAnalyzer
//////////////////////////////////////////////////////////////////////////
PerfCaptureAnalyzer::PerfCaptureAnalyzer() : CachedResult{} {}
PerfCaptureAnalyzer::~PerfCaptureAnalyzer() {}

void PerfCaptureAnalyzer::SetReferenceSnapshot(PerfCaptureSnapshot snapshot) {
  ReferenceShot = std::make_shared<PerfCaptureSnapshot>(snapshot);
}

void PerfCaptureAnalyzer::SetComparisonSnapshot(PerfCaptureSnapshot snapshot) {
  ComparisonShot = std::make_shared<PerfCaptureSnapshot>(snapshot);
}

float CalcStdDev(float sample, float mean) {
  float diff = sample - mean;
  return diff * diff;
}
void FinalizeStdDev(float& inOutStddevAvgs, int sampleCount) {
  float stddevSqr = inOutStddevAvgs / sampleCount;
  inOutStddevAvgs = sqrtf(stddevSqr);
}

PerfCaptureAnalyzer::DistilledStats PerfCaptureAnalyzer::DistillPerfStats(
    std::shared_ptr<PerfCaptureSnapshot> snapshot) {
  DistilledStats distStats{};

  if (snapshot->IsLoaded()) {
    size_t numStats = snapshot->GetPerfStatsCount();
    const ovrPerfStats* allStats = snapshot->GetPerfStats();

    // calc means
    {
      int lastAppFrameIdx = -1;
      int lastCompFrameIdx = -1;

      distStats.AppNumFrames = 0;
      distStats.CompNumFrames = 0;

      for (size_t curStatIdx = 0; curStatIdx < numStats; ++curStatIdx) {
        int numFramesInCurStat = allStats[curStatIdx].FrameStatsCount;
        // Go in reverse as frames are added from current to older in a given perfStats sample
        // Doesn't matter right now, but could if we try to process frames in order
        for (int curFrameIdx = numFramesInCurStat - 1; curFrameIdx >= 0; --curFrameIdx) {
          const ovrPerfStatsPerCompositorFrame& frameStats =
              allStats[curStatIdx].FrameStats[curFrameIdx];

          if (frameStats.AppFrameIndex != lastAppFrameIdx) {
            distStats.AppCpuTimeMean += frameStats.AppCpuElapsedTime;
            distStats.AppGpuTimeMean += frameStats.AppGpuElapsedTime;
            distStats.AppLatencyMean += frameStats.AppMotionToPhotonLatency;
            distStats.AppQueueAheadMean += frameStats.AppQueueAheadTime;
            distStats.AppNumDroppedFrames += frameStats.AppDroppedFrameCount;

            ++distStats.AppNumFrames;
          }

          if (frameStats.CompositorFrameIndex != lastCompFrameIdx) {
            distStats.CompCpuTimeMean += frameStats.CompositorCpuElapsedTime;
            distStats.CompGpuTimeMean += frameStats.CompositorGpuElapsedTime;
            distStats.CompVsyncBufferMean += frameStats.CompositorGpuEndToVsyncElapsedTime;
            distStats.CompLatencyMean += frameStats.CompositorLatency;

            distStats.CompNumDroppedFrames = frameStats.CompositorDroppedFrameCount;

            ++distStats.CompNumFrames;
          }

          lastAppFrameIdx = frameStats.AppFrameIndex;
          lastCompFrameIdx = frameStats.CompositorFrameIndex;
        }
      }

      distStats.AppCpuTimeMean /= distStats.AppNumFrames;
      distStats.AppGpuTimeMean /= distStats.AppNumFrames;
      distStats.AppLatencyMean /= distStats.AppNumFrames;
      distStats.AppQueueAheadMean /= distStats.AppNumFrames;

      distStats.CompCpuTimeMean /= distStats.CompNumFrames;
      distStats.CompGpuTimeMean /= distStats.CompNumFrames;
      distStats.CompVsyncBufferMean /= distStats.CompNumFrames;
      distStats.CompLatencyMean /= distStats.CompNumFrames;
    }

    // clang-format off
    {
      distStats.AppDroppedFramePerc = (float)distStats.AppNumDroppedFrames / distStats.AppNumDroppedFrames;
      distStats.CompDroppedFramePerc = (float)distStats.CompNumDroppedFrames / distStats.CompNumDroppedFrames;
    }
    // clang-format on

    // Now calc std dev
    // Could calculate this next to the mean in 1 pass above using Welford's algo
    {
      int lastAppFrameIdx = -1;
      int lastCompFrameIdx = -1;

      for (size_t curStatIdx = 0; curStatIdx < numStats; ++curStatIdx) {
        int numFramesInCurStat = allStats[curStatIdx].FrameStatsCount;
        // Go in reverse as frames are added from current to older in a given perfStats sample
        // Doesn't matter right now, but could if we try to process frames in order
        for (int curFrameIdx = numFramesInCurStat - 1; curFrameIdx >= 0; --curFrameIdx) {
          //
          const ovrPerfStatsPerCompositorFrame& frameStats =
              allStats[curStatIdx].FrameStats[curFrameIdx];

          // clang-format off
          if (frameStats.AppFrameIndex != lastAppFrameIdx) {
            distStats.AppCpuTimeStddev    += CalcStdDev(frameStats.AppCpuElapsedTime,        distStats.AppCpuTimeMean);
            distStats.AppGpuTimeStddev    += CalcStdDev(frameStats.AppGpuElapsedTime,        distStats.AppGpuTimeMean);
            distStats.AppLatencyStddev    += CalcStdDev(frameStats.AppMotionToPhotonLatency, distStats.AppLatencyMean);
            distStats.AppQueueAheadStddev += CalcStdDev(frameStats.AppQueueAheadTime,        distStats.AppQueueAheadMean);
          }

          if (frameStats.CompositorFrameIndex != lastCompFrameIdx) {
            distStats.CompCpuTimeStddev     += CalcStdDev(frameStats.CompositorCpuElapsedTime,           distStats.CompCpuTimeMean);
            distStats.CompGpuTimeStddev     += CalcStdDev(frameStats.CompositorGpuElapsedTime,           distStats.CompGpuTimeMean);
            distStats.CompVsyncBufferStddev += CalcStdDev(frameStats.CompositorGpuEndToVsyncElapsedTime, distStats.CompVsyncBufferMean);
            distStats.CompLatencyStddev     += CalcStdDev(frameStats.CompositorLatency,                  distStats.CompLatencyMean);
          }
          // clang-format on

          lastAppFrameIdx = frameStats.AppFrameIndex;
          lastCompFrameIdx = frameStats.CompositorFrameIndex;
        }
      }

      FinalizeStdDev(distStats.AppCpuTimeStddev, distStats.AppNumFrames);
      FinalizeStdDev(distStats.AppGpuTimeStddev, distStats.AppNumFrames);
      FinalizeStdDev(distStats.AppLatencyStddev, distStats.AppNumFrames);
      FinalizeStdDev(distStats.AppQueueAheadStddev, distStats.AppNumFrames);

      FinalizeStdDev(distStats.CompCpuTimeStddev, distStats.CompNumFrames);
      FinalizeStdDev(distStats.CompGpuTimeStddev, distStats.CompNumFrames);
      FinalizeStdDev(distStats.CompVsyncBufferStddev, distStats.CompNumFrames);
      FinalizeStdDev(distStats.CompLatencyStddev, distStats.CompNumFrames);
    }
  }
  return distStats;
}

void PerfCaptureAnalyzer::UpdateCachedWorseReason(uint32_t reasonBits) {
  OVR_ASSERT(CachedResult.NumWorseReasons < AnalysisResult::MaxNumWorseReasons);
  CachedResult.comparisonResult = ComparisonResult::WorseThanReference;
  CachedResult.WorseReasons[CachedResult.NumWorseReasons] |= reasonBits;
  ++CachedResult.NumWorseReasons;
}

PerfCaptureAnalyzer::AnalysisResult PerfCaptureAnalyzer::Compare() {
  CachedResult.comparisonResult = ComparisonResult::None;

  if (ReferenceShot->IsLoaded() && ReferenceShot->IsLoaded()) {
    DistilledStats refStats = DistillPerfStats(ReferenceShot);
    DistilledStats compStats = DistillPerfStats(ComparisonShot);

    // Compare dropped frames
    if ((refStats.AppDroppedFramePerc * DroppedFrameThreshPerc) < compStats.AppDroppedFramePerc) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::DroppedFrames | ReasonBits::TooHigh);
    }
    if ((refStats.CompDroppedFramePerc * DroppedFrameThreshPerc) < compStats.CompDroppedFramePerc) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::DroppedFrames | ReasonBits::TooHigh);
    }

    // Compare means - app
    if ((refStats.AppCpuTimeMean + MeanThreshTime) < compStats.AppCpuTimeMean) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::CpuTime | ReasonBits::TooHigh);
    }
    if ((refStats.AppGpuTimeMean + MeanThreshTime) < compStats.AppGpuTimeMean) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::GpuTime | ReasonBits::TooHigh);
    }
    if ((refStats.AppLatencyMean + MeanThreshTime) < compStats.AppLatencyMean) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::Latency | ReasonBits::TooHigh);
    }
    if ((refStats.AppQueueAheadMean + MeanThreshTime) < compStats.AppQueueAheadMean) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::Kickoff | ReasonBits::TooHigh);
    }
    // Compare means - comp
    if ((refStats.CompCpuTimeMean + MeanThreshTime) < compStats.CompCpuTimeMean) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::CpuTime | ReasonBits::TooHigh);
    }
    if ((refStats.CompGpuTimeMean + MeanThreshTime) < compStats.CompGpuTimeMean) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::GpuTime | ReasonBits::TooHigh);
    }
    if ((refStats.CompLatencyMean + MeanThreshTime) < compStats.CompLatencyMean) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::Latency | ReasonBits::TooHigh);
    }
    if ((refStats.CompVsyncBufferMean + MeanThreshTime) < compStats.CompVsyncBufferMean) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::Kickoff | ReasonBits::TooHigh);
    }

    // Compare std devs - app
    if ((refStats.AppCpuTimeStddev * StddevThreshPerc) < compStats.AppCpuTimeStddev) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::CpuTime | ReasonBits::TooVariant);
    }
    if ((refStats.AppGpuTimeStddev * StddevThreshPerc) < compStats.AppGpuTimeStddev) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::GpuTime | ReasonBits::TooVariant);
    }
    if ((refStats.AppLatencyStddev * StddevThreshPerc) < compStats.AppLatencyStddev) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::Latency | ReasonBits::TooVariant);
    }
    if ((refStats.AppQueueAheadStddev * StddevThreshPerc) < compStats.AppQueueAheadStddev) {
      UpdateCachedWorseReason(ReasonBits::App | ReasonBits::Kickoff | ReasonBits::TooVariant);
    }
    // Compare std devs - comp
    if ((refStats.CompCpuTimeStddev * StddevThreshPerc) < compStats.CompCpuTimeStddev) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::CpuTime | ReasonBits::TooVariant);
    }
    if ((refStats.CompGpuTimeStddev * StddevThreshPerc) < compStats.CompGpuTimeStddev) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::GpuTime | ReasonBits::TooVariant);
    }
    if ((refStats.CompLatencyStddev * StddevThreshPerc) < compStats.CompLatencyStddev) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::Latency | ReasonBits::TooVariant);
    }
    if ((refStats.CompVsyncBufferStddev * StddevThreshPerc) < compStats.CompVsyncBufferStddev) {
      UpdateCachedWorseReason(ReasonBits::Comp | ReasonBits::Kickoff | ReasonBits::TooVariant);
    }
  } else {
    CachedResult.comparisonResult = ComparisonResult::InvalidComparison;
  }

  return CachedResult;
}
