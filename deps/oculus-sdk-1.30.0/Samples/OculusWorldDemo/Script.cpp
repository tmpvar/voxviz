/************************************************************************************
Created     :   Feb 23, 2018
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

#include "Script.h"
#include "OculusWorldDemo.h"
#include "Extras/OVR_Math.h"
#include "../CommonSrc/Util/OptionMenu.h"
#include "../CommonSrc/Util/Logger.h" // WriteLog
#include "../CommonSrc/Render/Render_Device.h"
#include "Util/Util_D3D11_Blitter.h" // From OVRKernel
#include <fstream>
#include <sstream>
#include <string>
#include <cerrno>

#ifdef _MSC_VER
    #pragma warning(disable: 4996) // 'sscanf': This function or variable may be unsafe.
#endif


// Remove any reading and trailing space, tab, newline
static void Trim(std::string& str, const char* whitespace = " \t\r\n")
{
    str.erase(0, str.find_first_not_of(whitespace));
    str.erase(str.find_last_not_of(whitespace)+1); // Works because C++ requires (string::npos == -1)
}

// Removes leading and trailing quotes. If there's only a leading or only a trailing quote 
// then it is removed; matches are not required for the removal to occur.
static void Dequote(std::string& str)
{
    if(!str.empty() && (str[0] == '\"'))
        str.erase(str.begin());
    if(!str.empty() && (str.back() == '\"'))
        str.pop_back();
}

static void TrimAndDequote(std::string& str)
{
    Trim(str);
    Dequote(str);
}


OWDScript::OWDScript(OculusWorldDemoApp* owdApp)
    : ScriptFile(), ScriptState(State::None), Commands(), CurrentCommand(), Status(), OWDApp(owdApp)
{
}


// Does some checking and calls the lower level LoadScriptFile function.
bool OWDScript::LoadScriptFile(const char* filePath)
{
    bool success = false;

    if(ScriptState == State::None) // If not already executing script...
    {
        ScriptFile = filePath; // Save this for possible use during LoadScriptFile.
        TrimAndDequote(ScriptFile);
        success = LoadScriptFile(filePath, Commands);
    }

    return success;
}


// Does some checking and calls the lower level LoadScriptText function.
bool OWDScript::LoadScriptText(const char* scriptText)
{
    if(ScriptState == State::None) // If not already executing script...
        return LoadScriptText(scriptText, Commands);

    return false;
}


void OWDScript::Shutdown()
{
    CleanupOWDState();

    // Return to the newly constructed state.
    ScriptFile.clear();
    Commands.clear();
    CurrentCommand = 0;
    ScriptState = State::None;
    Status = 0;
    // Do not clear OWDApp, as it's set on construction.
}


// Loads the file and calls the lower level LoadScriptText function.
bool OWDScript::LoadScriptFile(const char* filePath, CommandVector& commands)
{
    bool success = false;

    std::string filePathStr(filePath);
    Dequote(filePathStr);

    std::ifstream file(filePathStr.c_str(), std::ios::in);

    if (!file)
    {
        // If the file open failed, let's check to see if the filePath was a 
        // relative file path to the current ScriptFile file path (e.g. script_2.txt).
        // For example, ScriptFile is /somedir/somedir/script_1.txt
        size_t pos = ScriptFile.find_last_of("/\\");

        if(pos != std::string::npos)
        {
            filePathStr = ScriptFile;   // e.g. /somedir/somedir/script_1.txt
            filePathStr.resize(pos+1);  // e.g. /somedir/somedir/
            filePathStr += filePath;    // e.g. /somedir/somedir/script_2.txt
        }

        file.open(filePathStr.c_str(), std::ios::in);
    }

    if (file)
    {
        std::ostringstream scriptText;
        scriptText << file.rdbuf();
        file.close();

        success = LoadScriptText(scriptText.str().c_str(), commands);
    }
    
    return success;
}


bool OWDScript::LoadScriptText(const char* scriptText, CommandVector& commands)
{
    bool success = true;

    if(scriptText)
    {
        std::istringstream stream{scriptText};
        std::string line;

        while (success && std::getline(stream, line))
        {
            Trim(line);

            if (!ReadCommandLine(line, commands))
            {
                success = false;
                commands.clear();
            }
        }
    }

    ScriptState = (success ? State::NotStarted : State::None);

    return success;
}


// The input line is expected to be trimmed of leading and trailing whitespace.
bool OWDScript::ReadCommandLine(const std::string& line, CommandVector& commands)
{
    bool success = true;
    char commandName[32];
    int result = sscanf(line.c_str(), "%31s", commandName); // Get the command name (e.g. "Wait")

    if(result == 1)
    {
        if(OVR_stristr(commandName, "ExecuteMenu") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandExecuteMenu>();
            CommandExecuteMenu* p = static_cast<CommandExecuteMenu*>(commandPtr.get());

            // We have a line like so: " ExecuteMenu   \"parent.child.size\" 100.23"
            p->MenuPath = line;
            p->MenuPath.erase(0, strlen("ExecuteMenu"));
            Trim(p->MenuPath);

            // Now we have a line like so: "\"parent.child.size\" \"some value\""
            // We implement a dequoting scheme that has limitations in its flexibility of handling
            // escaped quotes, but it should work for all our known value string use cases.
            size_t lastLineQuote = p->MenuPath.rfind('\"');
            size_t firstLineQuote = p->MenuPath.find('\"');

            if(lastLineQuote == (p->MenuPath.size() - 1) &&  // If the last argument is quoted...
               (firstLineQuote != lastLineQuote)) // And 
            {
                firstLineQuote = p->MenuPath.rfind('\"', lastLineQuote - 1);
                p->MenuValue.assign(p->MenuPath, firstLineQuote, lastLineQuote + 1 - firstLineQuote);
                p->MenuPath.erase(firstLineQuote);
            }
            else
            {
                size_t lastLineSpace = p->MenuPath.find_last_of(" \t");
                if(lastLineSpace != std::string::npos)
                {  
                    p->MenuValue.assign(p->MenuPath, lastLineSpace);
                    p->MenuPath.erase(lastLineSpace);
                }
            }

            TrimAndDequote(p->MenuPath);
            TrimAndDequote(p->MenuValue);

            if (!p->MenuPath.empty() && !p->MenuValue.empty())
                commands.push_back(commandPtr);
            else
            {
                WriteLog("OWDScript: Invalid ExecuteMenu command: %s. Expect ExecuteMenu <menu path string> <value>\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "ExecuteScriptFile") == commandName)
        {
            std::string filePath = (line.c_str() + strlen("ExecuteScriptFile"));
            TrimAndDequote(filePath);

            CommandVector commandsTemp;

            if (LoadScriptFile(filePath.c_str(), commandsTemp))
                commands.insert(commands.end(), commandsTemp.begin(), commandsTemp.end());
            else
            {
                WriteLog("OWDScript: Invalid or unloadable ExecuteScriptFile command: %s. Expect ExecuteScriptFile <script file path string>\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "SetPose") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandSetPose>();
            CommandSetPose* p = static_cast<CommandSetPose*>(commandPtr.get());
            char buffer[8];

            int argCount = sscanf(line.c_str(), "%*s %f %f %f %f %f %f", &p->X, &p->Y, &p->Z, &p->OriX, &p->OriY, &p->OriZ);
            
            if (argCount == 6)
                commands.push_back(commandPtr);
            else if((sscanf(line.c_str(), "%*s %8s", buffer) == 1) && (OVR_stricmp(buffer, "off") == 0))
            {
                p->stop = true;
                commands.push_back(commandPtr);
            }
            else
            {
                WriteLog("OWDScript: Invalid SetPose command: %s. Expect SetPose [<x pos> <y pos> <z pos> <x ori> <y ori> <z ori>] | off\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "Wait") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandWait>();
            CommandWait* p = static_cast<CommandWait*>(commandPtr.get());

            char waitUnits[16];
            int argCount = sscanf(line.c_str(), "%*s %lf %15s", &p->WaitAmount, waitUnits);
            
            if (argCount == 2)
            {
                if(OVR_stristr(waitUnits, "ms") == waitUnits)
                    p->WaitUnits = CommandWait::ms;
                else if(OVR_stristr(waitUnits, "s") == waitUnits)
                    p->WaitUnits = CommandWait::s;
                else if(OVR_stristr(waitUnits, "frame") == waitUnits) // Covers both "frame" and "frames".
                    p->WaitUnits = CommandWait::frames;
                else
                {
                    WriteLog("OWDScript: Invalid wait units: %s", waitUnits);
                    success = false;
                }

                if ((p->WaitAmount > 0) && (p->WaitUnits != CommandWait::none))
                    commands.push_back(commandPtr);
            }
            else
            {
                WriteLog("OWDScript: Invalid Wait command: %s. Expect Wait <amount> [ms|s|frame|frames]\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "Screenshot") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandScreenshot>();
            CommandScreenshot* p = static_cast<CommandScreenshot*>(commandPtr.get());

            p->ScreenshotFilePath = line.c_str() + strlen("Screenshot");
            TrimAndDequote(p->ScreenshotFilePath);
            if (!p->ScreenshotFilePath.empty())
                commands.push_back(commandPtr);
            else
            {
                WriteLog("OWDScript: Invalid Screenshot command: %s. Expect Screenshot <output file path>\n", line.c_str());
                success = false;
            }
        }
        else if (OVR_stristr(commandName, "PerfCapture") == commandName)
        {
          CommandPtr commandPtr = std::make_shared<CommandPerfCapture>();
          CommandPerfCapture* p = static_cast<CommandPerfCapture*>(commandPtr.get());

          char unitsStr[16];
          int cmdStrLen;
          const char* lineCStr = line.c_str();
          int argCount = sscanf(lineCStr, "%*s %lf %15s%n", &p->CaptureTotalDuration, unitsStr, &cmdStrLen);

          // Read rest of the string into variable. We do this to pull in white spaces.
          int fileNameStartOffset = cmdStrLen + 1;
          if ((int)strlen(lineCStr) > fileNameStartOffset)
          {
            p->CaptureFilePath = lineCStr + fileNameStartOffset;
            TrimAndDequote(p->CaptureFilePath);

            if (!p->CaptureFilePath.empty())
              argCount++;
          }

          if (argCount == 3)
          {
            if (OVR_stristr(unitsStr, "ms") == unitsStr)
              p->CaptureDurationUnits = PerfCaptureSerializer::Units::ms;
            else if (OVR_stristr(unitsStr, "s") == unitsStr)
              p->CaptureDurationUnits = PerfCaptureSerializer::Units::s;
            else if (OVR_stristr(unitsStr, "frame") == unitsStr) // Covers both "frame" and "frames".
              p->CaptureDurationUnits = PerfCaptureSerializer::Units::frames;
            else
            {
              WriteLog("OWDScript: Invalid duration units: %s", unitsStr);
              success = false;
            }

            if ((p->CaptureTotalDuration > 0) && (p->CaptureDurationUnits != PerfCaptureSerializer::Units::none))
            {
              if (p->CaptureCollector == nullptr)
              {
                // since we need the session, we delay the capture 
                p->CaptureCollector = std::make_shared<PerfCaptureSerializer>(
                  p->CaptureTotalDuration, p->CaptureDurationUnits, p->CaptureFilePath);
              }

              commands.push_back(commandPtr);
            }
          }
          else
          {
            WriteLog("OWDScript: Invalid PerfCapture command: %s. Expect PerfCapture <amount> " \
                     "[ms|s|frame|frames] <output file path>\n", line.c_str());
            success = false;
          }
        }
        else if(OVR_stristr(commandName, "WriteLog") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandWriteLog>();
            CommandWriteLog* p = static_cast<CommandWriteLog*>(commandPtr.get());

            p->LogText = line.c_str() + strlen("WriteLog");
            TrimAndDequote(p->LogText); // Unfortunately, we don't have a way of telling if the quote char is intentional.
            if (!p->LogText.empty())
                commands.push_back(commandPtr);
            // Do we print an error that the log text was empty?
        }
        else if(OVR_stristr(commandName, "Exit") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandExit>();
            commands.push_back(commandPtr);
        }
        else if(line.empty() || (line[0] == '/' && line[1] == '/'))
        {
            // Ignore empty or comment lines.
        }
        else
        {
            WriteLog("OWDScript: Invalid command line: %s\n", line.c_str());
            success = false;
        }
    }

    return success;
}

#ifdef _WIN32

// To consider: Move WriteTexture to an OWD member function.
static bool WriteTexture(ID3D11Device* d3dDevice, OVR::Render::D3D11::Texture* texture, const std::string& filePathOutput)
{
    using namespace OVR::D3DUtil;

    std::wstring filePathOutputW = OVR::UTF8StringToUCSString(filePathOutput);

    D3DTextureWriter textureWriter(d3dDevice);

    D3DTextureWriter::Result result = textureWriter.SaveTexture(
          texture->Tex, 0, true, filePathOutputW.c_str(), nullptr, nullptr);

    return (result == D3DTextureWriter::Result::SUCCESS);
}

#endif

void OWDScript::CleanupOWDState()
{
    // We reset anything we may have set in OWD.
    // If animation is to be unilaterally resumed, then we need to execute "Scene Content.Animation Enabled.false"

    // Disabled until we get the ovri_GetTrackingOverride change checked in.
}

OWDScript::State OWDScript::Step()
{
    // Currently we execute no more than one instruction per step. We could possibly execute
    // multiple instructions in a step for instructions that complete during the step.

    if((ScriptState == State::NotStarted) || (ScriptState == State::Started))
    {
        ScriptState = State::Started; // Set it started if not started already.

        if (CurrentCommand < Commands.size())
        {
            Command& command = *Commands[CurrentCommand].get(); // Convert shared_ptr to a reference.

            switch (command.commandType)
            {
                case CommandType::ExecuteMenu:{
                    CommandExecuteMenu& c = static_cast<CommandExecuteMenu&>(command);

                    OptionMenuItem* menuItem = OWDApp->Menu.FindMenuItem(c.MenuPath.c_str());

                    if (menuItem)
                    {
                        bool success = menuItem->SetValue(c.MenuValue);
                        if(success)
                            WriteLog("OWDScript: ExecuteMenu: %s->%s\n", c.MenuPath.c_str(), c.MenuValue.c_str());
                        else
                            WriteLog("OWDScript: ExecuteMenu: set of option failed: %s -> %s. Is the option available with this build or configuration?\n", c.MenuPath.c_str(), c.MenuValue.c_str());
                    }
                    else
                        WriteLog("OWDScript: ExecuteMenu: Menu not found: %s -> %s. Is the menu available with this build or configuration?\n", c.MenuPath.c_str(), c.MenuValue.c_str());

                    CurrentCommand++;

                    break;
                }

                case CommandType::ExecuteScriptFile:
                    // This shouldn't happen because we expanded the script file on load.
                    break;

                case CommandType::SetPose:{
                    CommandSetPose& c = static_cast<CommandSetPose&>(command);

                        WriteLog("OWDScript: SetPose (not supported): x:%f y:%f z:%f OriX:%f OriY:%f OriZ:%f\n", 
                            c.X, c.Y, c.Z, c.OriX, c.OriY, c.OriZ);

                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::Wait:{
                    CommandWait& c = static_cast<CommandWait&>(command);

                    if(c.CompletionValue == 0) // If the wait isn't started yet...
                    {
                        if (c.WaitUnits == CommandWait::ms)
                        {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + (c.WaitAmount / 1000));
                            WriteLog("OWDScript: Wait for %f ms started\n", c.WaitAmount);
                        }
                        else if(c.WaitUnits == CommandWait::s)
                        {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + c.WaitAmount);
                            WriteLog("OWDScript: Wait for %f s started\n", c.WaitAmount);
                        }
                        else // CommandWait::frames
                        {
                            c.CompletionValue = (OWDApp->TotalFrameCounter + c.WaitAmount);
                            WriteLog("OWDScript: Wait for %f frame(s) started\n", c.WaitAmount);
                        }
                    }

                    if ((c.WaitUnits == CommandWait::ms) || 
                        (c.WaitUnits == CommandWait::s))
                    {
                        if(ovr_GetTimeInSeconds() >= c.CompletionValue) // If done waiting
                        {
                            WriteLog("OWDScript: WaitMs wait for %f %s completed\n", c.WaitAmount, 
                                (c.WaitUnits == CommandWait::ms) ? "ms" : "s");
                            CurrentCommand++; // Move onto the next instruction.
                        }
                    }
                    else // CommandWait::frames
                    {
                        if(OWDApp->TotalFrameCounter >= c.CompletionValue) // If done waiting
                        {
                            WriteLog("OWDScript: Wait for %u frame(s) completed\n", c.WaitAmount);
                            CurrentCommand++; // Move onto the next instruction.
                        }
                    }

                    break;
                }

                case CommandType::Screenshot:{
                    CommandScreenshot& c = static_cast<CommandScreenshot&>(command);
                    #ifdef _WIN32
                    bool success = WriteTexture(static_cast<OVR::Render::D3D11::RenderDevice*>(OWDApp->pRender)->Device, 
                        static_cast<OVR::Render::D3D11::Texture*>(OWDApp->MirrorTexture.GetPtr()), c.ScreenshotFilePath);
                    #else
                    bool success = false;
                    #endif

                    WriteLog("OWDScript: Screenshot: Save to %s %s\n", c.ScreenshotFilePath.c_str(), success ? "succeeded" : "failed");
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::PerfCapture:{
                    CommandPerfCapture& c = static_cast<CommandPerfCapture&>(command);

                    OVR_ASSERT(c.CaptureCollector);

                    auto serializerStatus = c.CaptureCollector->Step(OWDApp->Session);

                    if (serializerStatus == PerfCaptureSerializer::Status::Complete)
                    {
                        // TEST: Test deserialization and perf comparison
                        // Requires creation of *.perfref file from a previous execution of script
                        static bool TEST_PERF_COMPARISON = true;
                        if(TEST_PERF_COMPARISON)
                        {
                          PerfCaptureSnapshot snapshotReference, snapshotNew;
                          bool loadSuccess1 = snapshotReference.LoadSnapshot(c.CaptureFilePath);
                          bool loadSuccess2 = snapshotNew.LoadSnapshot(c.CaptureFilePath + "ref");

                          if (loadSuccess1 && loadSuccess2)
                          {
                            PerfCaptureAnalyzer perfAnalyzer;
                            perfAnalyzer.SetReferenceSnapshot(snapshotReference);
                            perfAnalyzer.SetComparisonSnapshot(snapshotNew);
                            PerfCaptureAnalyzer::AnalysisResult analysisResult = perfAnalyzer.Compare();

                            if (analysisResult.comparisonResult == PerfCaptureAnalyzer::ComparisonResult::BetterThanReference)
                            {
                              WriteLog("OWDScript: PerfCapture analysis of %s shows BETTER perf than reference\n", c.CaptureFilePath.c_str());
                            }
                            else if (analysisResult.comparisonResult == PerfCaptureAnalyzer::ComparisonResult::None ||
                                     analysisResult.comparisonResult == PerfCaptureAnalyzer::ComparisonResult::InvalidComparison)
                            {
                              OVR_FAIL();
                              WriteLog("OWDScript: PerfCapture analysis of %s FAILED\n", c.CaptureFilePath.c_str());
                            }
                            else if (analysisResult.comparisonResult == PerfCaptureAnalyzer::ComparisonResult::WorseThanReference)
                            {
                              // worse for one of the reasons captured below
                              WriteLog("OWDScript: PerfCapture analysis of %s shows WORSE perf than reference for following reasons:\n", c.CaptureFilePath.c_str());

                              for (int reasonIdx = 0; reasonIdx < PerfCaptureAnalyzer::AnalysisResult::MaxNumWorseReasons; ++reasonIdx)
                              {
                                uint32_t reasonBits = analysisResult.WorseReasons[reasonIdx];
                                if (reasonBits == 0)
                                  break;

                                std::string perfReasons;

                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::App)            perfReasons += "App ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::Comp)           perfReasons += "Compositor ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::DroppedFrames)  perfReasons += "dropped frames ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::CpuTime)        perfReasons += "CPU time ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::GpuTime)        perfReasons += "GPU time ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::Latency)        perfReasons += "tracking latency ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::Kickoff)        perfReasons += "kick-off ";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::TooHigh)        perfReasons += "too high.";
                                if (reasonBits & PerfCaptureAnalyzer::ReasonBits::TooVariant)     perfReasons += "too variant.";

                                WriteLog("\tReason #%d: %s", reasonIdx, perfReasons.c_str());
                              }
                            }
                            else { OVR_FAIL(); }
                          }
                          else
                          {
                            // Failed to find *.perfref file - ignore and continue
                          }
                        }                    

                        WriteLog("OWDScript: PerfCapture to %s completed\n", c.CaptureFilePath.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if (serializerStatus == PerfCaptureSerializer::Status::Error)
                    {
                        WriteLog("OWDScript: PerfCapture to %s FAILED\n", c.CaptureFilePath.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if (!c.CaptureStartPrinted && serializerStatus == PerfCaptureSerializer::Status::Started)
                    {
                        WriteLog("OWDScript: PerfCapture to %s started\n", c.CaptureFilePath.c_str());
                        c.CaptureStartPrinted = true;
                    }

                    break;
                }

                case CommandType::WriteLog:{
                    CommandWriteLog& c = static_cast<CommandWriteLog&>(command);
                    //WriteLog("OWDScript: WriteLog: %s", c.LogText.c_str()); // This would be redundant.
                    WriteLog("%s", c.LogText.c_str());
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::Exit:{
                    CleanupOWDState();
                    OWDApp->Exit(Status); // The actual exit will occur in the near future.
                    WriteLog("OWDScript: Exit: %d\n", Status);
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }
            }
        }

        if (CurrentCommand == Commands.size())
            ScriptState = State::Complete;
    }

    return ScriptState;
}


OWDScript::State OWDScript::GetState() const
{
    return ScriptState;
}








