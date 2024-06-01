add_subdirectory(parser)
add_subdirectory(synthesis)

include_directories(${PARSER_INCLUDE_PATH} ${SYNTHESIS_INCLUDE_PATH} ${EXT_INCLUDE_PATH})
add_executable(MtSyft MtSyft.cpp)
add_executable(cb-MtSyft common-core-MtSyft.cpp)
add_executable(conj-MtSyft refining-envs-MtSyft.cpp)
# add_executable(counter-benchmark counter-benchmark.cpp)


target_link_libraries(MtSyft ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})
target_link_libraries(cb-MtSyft ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})
target_link_libraries(conj-MtSyft ${PARSER_LIB_NAME} ${SYNTHESIS_LIB_NAME})

install(TARGETS MtSyft cb-MtSyft conj-MtSyft
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	)

INSTALL (
  DIRECTORY ${CMAKE_SOURCE_DIR}/src/
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*")
