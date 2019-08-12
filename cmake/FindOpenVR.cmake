include(FindPackageHandleStandardArgs)

find_path(
  OPENVR_INCLUDE_DIRS
  openvr_capi.h
)

get_filename_component(_prefix_path ${OPENVR_INCLUDE_DIRS} PATH)

find_library(
    OPENVR_LIBRARY_DEBUG
    NAMES openvr_api
    PATHS ${_prefix_path}/debug/lib
    NO_DEFAULT_PATH
)

find_library(
    OPENVR_LIBRARY_RELEASE
    NAMES openvr_api
    PATHS ${_prefix_path}/lib
    NO_DEFAULT_PATH
)

unset(_prefix_path)

include(SelectLibraryConfigurations)
select_library_configurations(OPENVR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    OPENVR
    REQUIRED_VARS OPENVR_LIBRARIES OPENVR_INCLUDE_DIRS
)
