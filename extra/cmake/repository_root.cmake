# Provide a convenient, absolute reference to the top of the repository.
get_filename_component(REPOSITORY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../.. ABSOLUTE)
message(STATUS "REPOSITORY_ROOT=${REPOSITORY_ROOT}")
