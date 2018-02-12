##
## Auto Generated makefile by CodeLite IDE
## any manual changes will be erased      
##
## Win32_Release
ProjectName            :=FoXBot
ConfigurationName      :=Win32_Release
WorkspacePath          :=E:/Dropbox/src/foxbot
ProjectPath            :=E:/Dropbox/src/foxbot
IntermediateDirectory  :=./Release
OutDir                 := $(IntermediateDirectory)
CurrentFileName        :=
CurrentFilePath        :=
CurrentFileFullPath    :=
User                   :=Ryan
Date                   :=16/08/2016
CodeLitePath           :="C:/Program Files/CodeLite"
LinkerName             :=C:/MinGW/bin/g++.exe
SharedObjectLinkerName :=C:/MinGW/bin/g++.exe -shared -fPIC
ObjectSuffix           :=.o
DependSuffix           :=.o.d
PreprocessSuffix       :=.i
DebugSwitch            :=-g 
IncludeSwitch          :=-I
LibrarySwitch          :=-l
OutputSwitch           :=-o 
LibraryPathSwitch      :=-L
PreprocessorSwitch     :=-D
SourceSwitch           :=-c 
OutputFile             :=$(IntermediateDirectory)/foxbot.dll
Preprocessors          :=$(PreprocessorSwitch)NDEBUG $(PreprocessorSwitch)WIN32 $(PreprocessorSwitch)_WINDOWS $(PreprocessorSwitch)_WIN32 
ObjectSwitch           :=-o 
ArchiveOutputSwitch    := 
PreprocessOnlySwitch   :=-E
ObjectsFileList        :="FoXBot.txt"
PCHCompileFlags        :=
MakeDirCommand         :=makedir
RcCmpOptions           := 
RcCompilerName         :=C:/MinGW/bin/windres.exe
LinkOptions            :=  -Xlinker -shared -s
IncludePath            :=  $(IncludeSwitch). $(IncludeSwitch)../hlsdk-2.3-p4/multiplayer/cl_dll $(IncludeSwitch)../hlsdk-2.3-p4/multiplayer/dlls $(IncludeSwitch)../hlsdk-2.3-p4/multiplayer/common $(IncludeSwitch)../hlsdk-2.3-p4/multiplayer/engine $(IncludeSwitch)../hlsdk-2.3-p4/multiplayer/pm_shared $(IncludeSwitch)../metamod-p-37/metamod 
IncludePCH             := 
RcIncludePath          := 
Libs                   := $(LibrarySwitch)kernel32 $(LibrarySwitch)user32 $(LibrarySwitch)gdi32 $(LibrarySwitch)winspool $(LibrarySwitch)comdlg32 $(LibrarySwitch)advapi32 $(LibrarySwitch)shell32 $(LibrarySwitch)ole32 $(LibrarySwitch)oleaut32 $(LibrarySwitch)uuid 
ArLibs                 :=  "libkernel32.a" "libuser32.a" "libgdi32.a" "libwinspool.a" "libcomdlg32.a" "libadvapi32.a" "libshell32.a" "libole32.a" "liboleaut32.a" "libuuid.a" 
LibPath                := $(LibraryPathSwitch). $(LibraryPathSwitch). $(LibraryPathSwitch)Debug 

##
## Common variables
## AR, CXX, CC, AS, CXXFLAGS and CFLAGS can be overriden using an environment variables
##
AR       := C:/MinGW/bin/ar.exe rcu
CXX      := C:/MinGW/bin/g++.exe
CC       := C:/MinGW/bin/gcc.exe
CXXFLAGS :=  -g -O2 -fexpensive-optimizations -Wall -march=i686 -msse2 -D_strnicmp=strnicmp -Wno-unknown-pragmas -Wno-write-strings $(Preprocessors)
CFLAGS   :=   $(Preprocessors)
ASFLAGS  := 
AS       := C:/MinGW/bin/as.exe


##
## User defined environment variables
##
CodeLiteDir:=C:\Program Files\CodeLite
Objects0=$(IntermediateDirectory)/bot.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_client.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_combat.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_navigate.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_start.cpp$(ObjectSuffix) $(IntermediateDirectory)/botcam.cpp$(ObjectSuffix) $(IntermediateDirectory)/dll.cpp$(ObjectSuffix) $(IntermediateDirectory)/engine.cpp$(ObjectSuffix) $(IntermediateDirectory)/h_export.cpp$(ObjectSuffix) $(IntermediateDirectory)/linkfunc.cpp$(ObjectSuffix) \
	$(IntermediateDirectory)/meta_api.cpp$(ObjectSuffix) $(IntermediateDirectory)/sdk_util.cpp$(ObjectSuffix) $(IntermediateDirectory)/util.cpp$(ObjectSuffix) $(IntermediateDirectory)/waypoint.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_job_assessors.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_job_functions.cpp$(ObjectSuffix) $(IntermediateDirectory)/bot_job_think.cpp$(ObjectSuffix) $(IntermediateDirectory)/res_fox.rc$(ObjectSuffix) 



Objects=$(Objects0) 

##
## Main Build Targets 
##
.PHONY: all clean PreBuild PrePreBuild PostBuild MakeIntermediateDirs
all: $(OutputFile)

$(OutputFile): $(IntermediateDirectory)/.d $(Objects) 
	@$(MakeDirCommand) $(@D)
	@echo "" > $(IntermediateDirectory)/.d
	@echo $(Objects0)  > $(ObjectsFileList)
	$(SharedObjectLinkerName) $(OutputSwitch)$(OutputFile) @$(ObjectsFileList) $(LibPath) $(Libs) $(LinkOptions)
	@$(MakeDirCommand) "E:\Dropbox\src\foxbot/.build-release"
	@echo rebuilt > "E:\Dropbox\src\foxbot/.build-release/FoXBot"

MakeIntermediateDirs:
	@$(MakeDirCommand) "./Release"


$(IntermediateDirectory)/.d:
	@$(MakeDirCommand) "./Release"

PreBuild:


##
## Objects
##
$(IntermediateDirectory)/bot.cpp$(ObjectSuffix): bot.cpp $(IntermediateDirectory)/bot.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot.cpp$(DependSuffix): bot.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot.cpp$(DependSuffix) -MM bot.cpp

$(IntermediateDirectory)/bot.cpp$(PreprocessSuffix): bot.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot.cpp$(PreprocessSuffix)bot.cpp

$(IntermediateDirectory)/bot_client.cpp$(ObjectSuffix): bot_client.cpp $(IntermediateDirectory)/bot_client.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_client.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_client.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_client.cpp$(DependSuffix): bot_client.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_client.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_client.cpp$(DependSuffix) -MM bot_client.cpp

$(IntermediateDirectory)/bot_client.cpp$(PreprocessSuffix): bot_client.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_client.cpp$(PreprocessSuffix)bot_client.cpp

$(IntermediateDirectory)/bot_combat.cpp$(ObjectSuffix): bot_combat.cpp $(IntermediateDirectory)/bot_combat.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_combat.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_combat.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_combat.cpp$(DependSuffix): bot_combat.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_combat.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_combat.cpp$(DependSuffix) -MM bot_combat.cpp

$(IntermediateDirectory)/bot_combat.cpp$(PreprocessSuffix): bot_combat.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_combat.cpp$(PreprocessSuffix)bot_combat.cpp

$(IntermediateDirectory)/bot_navigate.cpp$(ObjectSuffix): bot_navigate.cpp $(IntermediateDirectory)/bot_navigate.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_navigate.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_navigate.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_navigate.cpp$(DependSuffix): bot_navigate.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_navigate.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_navigate.cpp$(DependSuffix) -MM bot_navigate.cpp

$(IntermediateDirectory)/bot_navigate.cpp$(PreprocessSuffix): bot_navigate.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_navigate.cpp$(PreprocessSuffix)bot_navigate.cpp

$(IntermediateDirectory)/bot_start.cpp$(ObjectSuffix): bot_start.cpp $(IntermediateDirectory)/bot_start.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_start.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_start.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_start.cpp$(DependSuffix): bot_start.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_start.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_start.cpp$(DependSuffix) -MM bot_start.cpp

$(IntermediateDirectory)/bot_start.cpp$(PreprocessSuffix): bot_start.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_start.cpp$(PreprocessSuffix)bot_start.cpp

$(IntermediateDirectory)/botcam.cpp$(ObjectSuffix): botcam.cpp $(IntermediateDirectory)/botcam.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/botcam.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/botcam.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/botcam.cpp$(DependSuffix): botcam.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/botcam.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/botcam.cpp$(DependSuffix) -MM botcam.cpp

$(IntermediateDirectory)/botcam.cpp$(PreprocessSuffix): botcam.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/botcam.cpp$(PreprocessSuffix)botcam.cpp

$(IntermediateDirectory)/dll.cpp$(ObjectSuffix): dll.cpp $(IntermediateDirectory)/dll.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/dll.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/dll.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/dll.cpp$(DependSuffix): dll.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/dll.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/dll.cpp$(DependSuffix) -MM dll.cpp

$(IntermediateDirectory)/dll.cpp$(PreprocessSuffix): dll.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/dll.cpp$(PreprocessSuffix)dll.cpp

$(IntermediateDirectory)/engine.cpp$(ObjectSuffix): engine.cpp $(IntermediateDirectory)/engine.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/engine.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/engine.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/engine.cpp$(DependSuffix): engine.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/engine.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/engine.cpp$(DependSuffix) -MM engine.cpp

$(IntermediateDirectory)/engine.cpp$(PreprocessSuffix): engine.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/engine.cpp$(PreprocessSuffix)engine.cpp

$(IntermediateDirectory)/h_export.cpp$(ObjectSuffix): h_export.cpp $(IntermediateDirectory)/h_export.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/h_export.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/h_export.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/h_export.cpp$(DependSuffix): h_export.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/h_export.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/h_export.cpp$(DependSuffix) -MM h_export.cpp

$(IntermediateDirectory)/h_export.cpp$(PreprocessSuffix): h_export.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/h_export.cpp$(PreprocessSuffix)h_export.cpp

$(IntermediateDirectory)/linkfunc.cpp$(ObjectSuffix): linkfunc.cpp $(IntermediateDirectory)/linkfunc.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/linkfunc.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/linkfunc.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/linkfunc.cpp$(DependSuffix): linkfunc.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/linkfunc.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/linkfunc.cpp$(DependSuffix) -MM linkfunc.cpp

$(IntermediateDirectory)/linkfunc.cpp$(PreprocessSuffix): linkfunc.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/linkfunc.cpp$(PreprocessSuffix)linkfunc.cpp

$(IntermediateDirectory)/meta_api.cpp$(ObjectSuffix): meta_api.cpp $(IntermediateDirectory)/meta_api.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/meta_api.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/meta_api.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/meta_api.cpp$(DependSuffix): meta_api.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/meta_api.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/meta_api.cpp$(DependSuffix) -MM meta_api.cpp

$(IntermediateDirectory)/meta_api.cpp$(PreprocessSuffix): meta_api.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/meta_api.cpp$(PreprocessSuffix)meta_api.cpp

$(IntermediateDirectory)/sdk_util.cpp$(ObjectSuffix): sdk_util.cpp $(IntermediateDirectory)/sdk_util.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/sdk_util.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/sdk_util.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/sdk_util.cpp$(DependSuffix): sdk_util.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/sdk_util.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/sdk_util.cpp$(DependSuffix) -MM sdk_util.cpp

$(IntermediateDirectory)/sdk_util.cpp$(PreprocessSuffix): sdk_util.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/sdk_util.cpp$(PreprocessSuffix)sdk_util.cpp

$(IntermediateDirectory)/util.cpp$(ObjectSuffix): util.cpp $(IntermediateDirectory)/util.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/util.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/util.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/util.cpp$(DependSuffix): util.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/util.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/util.cpp$(DependSuffix) -MM util.cpp

$(IntermediateDirectory)/util.cpp$(PreprocessSuffix): util.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/util.cpp$(PreprocessSuffix)util.cpp

$(IntermediateDirectory)/waypoint.cpp$(ObjectSuffix): waypoint.cpp $(IntermediateDirectory)/waypoint.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/waypoint.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/waypoint.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/waypoint.cpp$(DependSuffix): waypoint.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/waypoint.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/waypoint.cpp$(DependSuffix) -MM waypoint.cpp

$(IntermediateDirectory)/waypoint.cpp$(PreprocessSuffix): waypoint.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/waypoint.cpp$(PreprocessSuffix)waypoint.cpp

$(IntermediateDirectory)/bot_job_assessors.cpp$(ObjectSuffix): bot_job_assessors.cpp $(IntermediateDirectory)/bot_job_assessors.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_job_assessors.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_job_assessors.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_job_assessors.cpp$(DependSuffix): bot_job_assessors.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_job_assessors.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_job_assessors.cpp$(DependSuffix) -MM bot_job_assessors.cpp

$(IntermediateDirectory)/bot_job_assessors.cpp$(PreprocessSuffix): bot_job_assessors.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_job_assessors.cpp$(PreprocessSuffix)bot_job_assessors.cpp

$(IntermediateDirectory)/bot_job_functions.cpp$(ObjectSuffix): bot_job_functions.cpp $(IntermediateDirectory)/bot_job_functions.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_job_functions.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_job_functions.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_job_functions.cpp$(DependSuffix): bot_job_functions.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_job_functions.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_job_functions.cpp$(DependSuffix) -MM bot_job_functions.cpp

$(IntermediateDirectory)/bot_job_functions.cpp$(PreprocessSuffix): bot_job_functions.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_job_functions.cpp$(PreprocessSuffix)bot_job_functions.cpp

$(IntermediateDirectory)/bot_job_think.cpp$(ObjectSuffix): bot_job_think.cpp $(IntermediateDirectory)/bot_job_think.cpp$(DependSuffix)
	$(CXX) $(IncludePCH) $(SourceSwitch) "E:/Dropbox/src/foxbot/bot_job_think.cpp" $(CXXFLAGS) $(ObjectSwitch)$(IntermediateDirectory)/bot_job_think.cpp$(ObjectSuffix) $(IncludePath)
$(IntermediateDirectory)/bot_job_think.cpp$(DependSuffix): bot_job_think.cpp
	@$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) -MG -MP -MT$(IntermediateDirectory)/bot_job_think.cpp$(ObjectSuffix) -MF$(IntermediateDirectory)/bot_job_think.cpp$(DependSuffix) -MM bot_job_think.cpp

$(IntermediateDirectory)/bot_job_think.cpp$(PreprocessSuffix): bot_job_think.cpp
	$(CXX) $(CXXFLAGS) $(IncludePCH) $(IncludePath) $(PreprocessOnlySwitch) $(OutputSwitch) $(IntermediateDirectory)/bot_job_think.cpp$(PreprocessSuffix)bot_job_think.cpp

$(IntermediateDirectory)/res_fox.rc$(ObjectSuffix): res_fox.rc
	$(RcCompilerName) -i "E:/Dropbox/src/foxbot/res_fox.rc" $(RcCmpOptions)   $(ObjectSwitch)$(IntermediateDirectory)/res_fox.rc$(ObjectSuffix) $(RcIncludePath)

-include $(IntermediateDirectory)/*$(DependSuffix)
##
## Clean
##
clean:
	$(RM) -r ./Release/


