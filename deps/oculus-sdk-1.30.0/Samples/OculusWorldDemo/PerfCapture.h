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

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <ostream>
#include <istream>

#include "OVR_CAPI.h"

//////////////////////////////////////////////////////////////////////////
// Implements Oculus performance stats capture serializer
//////////////////////////////////////////////////////////////////////////

class PerfCaptureSerializer {
 public:
  enum class Units { none, ms, s, frames } DurationUnits;

  PerfCaptureSerializer() : PerfCaptureSerializer(0.0, Units::none, "") {}
  PerfCaptureSerializer(double totalDuration, Units units, const std::string& filePath);
  ~PerfCaptureSerializer() {
    Reset();
  }

  enum class Status {
    None, // No state set yet
    Error, // Failure to start capture
    NotStarted, // PerfCapture hasn't started
    Started, // PerfCapture has started at least one step.
    Complete // PerfCapture has completed.
  };

  void Reset();

  // Executes PerfCapture one step at a time. Call this periodically. At least once per frame.
  Status Step(ovrSession session);

  Status GetStatus() const {
    return CurrentStatus;
  }

 protected:
  void StartCapture();
  void EndCapture();
  void SerializeBuffer();

  std::string FilePath;
  double TotalDuration;

  double CompletionValue;
  Status CurrentStatus;

  const int NumBufferedStatsReserve = 16384; // at 90 Hz, enough to hold several minutes worth
  const int NumBufferedStatsSerializeTrigger = NumBufferedStatsReserve - 100;

  // Might benefit from a lockless buffer if the synchronous writer stalls things.
  std::vector<ovrPerfStats> BufferedPerfStats;

  std::ofstream SerializedFile;
  int CurrentFrame;
};

//////////////////////////////////////////////////////////////////////////
// Implements Oculus performance stats capture loader & deserializer
//////////////////////////////////////////////////////////////////////////
class PerfCaptureSnapshot {
 public:
  PerfCaptureSnapshot();

  ~PerfCaptureSnapshot() {
    Reset();
  }

  bool LoadSnapshot(const std::string& filePath);
  void Reset();
  bool IsLoaded() {
    return LoadSuccess;
  }

  const ovrPerfStats* GetPerfStats() {
    return &PerfStats[0];
  }
  size_t GetPerfStatsCount() {
    return PerfStats.size();
  }

 protected:
  std::string FilePath;
  std::vector<ovrPerfStats> PerfStats;
  bool LoadSuccess;
};

//////////////////////////////////////////////////////////////////////////
// Implements Oculus performance stats capture comparator and analyzer
//////////////////////////////////////////////////////////////////////////
class PerfCaptureAnalyzer {
 public:
  PerfCaptureAnalyzer();
  ~PerfCaptureAnalyzer();

  void SetReferenceSnapshot(PerfCaptureSnapshot snapshot);
  void SetComparisonSnapshot(PerfCaptureSnapshot snapshot);

  enum class ComparisonResult {
    None = 0,
    InvalidComparison,
    BetterThanReference,
    WorseThanReference,
  };

  enum ReasonBits {
    None = 0x0,

    // App vs. Compositor
    App = 0x1,
    Comp = 0x2,

    // Specific Stat
    DroppedFrames = 0x4,
    CpuTime = 0x8,
    GpuTime = 0x10,
    Latency = 0x20,
    Kickoff = 0x40, // queue-ahead or kick-off buffer

    // Difference issue
    TooHigh = 0x80,
    TooVariant = 0x100,
  };

  struct AnalysisResult {
    ComparisonResult comparisonResult;

    static const int MaxNumWorseReasons = 32;
    uint32_t WorseReasons[MaxNumWorseReasons];
    int NumWorseReasons;
  };

  AnalysisResult Compare();

 protected:
  const float MeanThreshTime = 0.0002f; // 0.2 ms
  const float StddevThreshPerc = 1.2f; // 20%
  const float DroppedFrameThreshPerc = 1.1f; // 10%

  struct DistilledStats {
    int AppNumFrames;
    int CompNumFrames;
    int AppNumDroppedFrames;
    int CompNumDroppedFrames;
    float AppDroppedFramePerc;
    float CompDroppedFramePerc;

    float AppCpuTimeMean;
    float AppGpuTimeMean;
    float AppLatencyMean;
    float AppQueueAheadMean;

    float CompCpuTimeMean;
    float CompGpuTimeMean;
    float CompLatencyMean;
    float CompVsyncBufferMean;

    float AppCpuTimeStddev;
    float AppGpuTimeStddev;
    float AppLatencyStddev;
    float AppQueueAheadStddev;

    float CompCpuTimeStddev;
    float CompGpuTimeStddev;
    float CompLatencyStddev;
    float CompVsyncBufferStddev;
  };

  DistilledStats PerfCaptureAnalyzer::DistillPerfStats(
      std::shared_ptr<PerfCaptureSnapshot> snapshot);

  void UpdateCachedWorseReason(uint32_t reasonBits);

  std::shared_ptr<PerfCaptureSnapshot> ReferenceShot;
  std::shared_ptr<PerfCaptureSnapshot> ComparisonShot;

  AnalysisResult CachedResult;
};
