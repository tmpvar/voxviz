# include(GetGitRevisionDescription)
# get_git_head_revision(GIT_REFSPEC GIT_SHA1)
# git_local_changes(GIT_LOCAL_CHANGES)
# if (${GIT_LOCAL_CHANGES} STREQUAL "DIRTY")
# 	set(COMPILER_MESSAGE "#pragma WARNING(Local changes not committed.)")
# endif()
configure_file("${CMAKE_CURRENT_LIST_DIR}/Version.h.in" "${CMAKE_CURRENT_LIST_DIR}/Version.h" @ONLY)


include_directories(${CMAKE_CURRENT_LIST_DIR}/..)
add_library(PBD_Utilities STATIC
		FileSystem.h
		Hashmap.h
		IndexedFaceMesh.cpp
		IndexedFaceMesh.h
		IndexedTetMesh.cpp
		IndexedTetMesh.h
		Logger.h
		OBJLoader.h
		SceneLoader.cpp
		SceneLoader.h
		StringTools.h
		SystemInfo.h
		TetGenLoader.cpp
		TetGenLoader.h
		Timing.h
		Version.h
		VolumeIntegration.cpp
		VolumeIntegration.h

		CMakeLists.txt
)

############################################################
# GenericParameters
############################################################
include_directories(${ExternalInstallDir}/GenericParameters/include)
add_dependencies(PBD_Utilities Ext_GenericParameters)

find_package( Eigen3 REQUIRED )
include_directories( ${EIGEN3_INCLUDE_DIR} )

# install(TARGETS Utils
#         RUNTIME DESTINATION bin
#         LIBRARY DESTINATION lib
#         ARCHIVE DESTINATION lib)
#
# install(DIRECTORY .
# DESTINATION include/Utils
# FILES_MATCHING PATTERN "*.h")
