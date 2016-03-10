CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(ocfgw)

SET(VENDOR "samsung")
SET(PACKAGE ${PROJECT_NAME})
SET(PKGNAME "com.${VENDOR}.${PACKAGE}")
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(BINDIR "${PREFIX}/bin")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/inc 
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../iotivity/resource/include/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../iotivity/resource/csdk/stack/include/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../iotivity/resource/c_common/
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../iotivity/resource/oc_logger/include/
                    )

file(GLOB_RECURSE SRCS
    src/*.cpp
)

INCLUDE(FindPkgConfig)
pkg_check_modules(ocfgw REQUIRED libcurl glib-2.0 openssl eina eet ecore ecore-con json-glib-1.0 sqlite3)

FOREACH(flag ${ocfgw_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
MESSAGE(STATUS "EXTRA_CFLAGS = ${EXTRA_CFLAGS}")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ocfgw_FLAGS} ${EXTRA_CFLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ocfgw_FLAGS} ${EXTRA_CFLAGS}")

FOREACH(flag ${ocfgw_LDFLAGS})
    SET(EXTRA_LDFLAGS "${EXTRA_LDFLAGS} ${flag}")
ENDFOREACH(flag)

message(STATUS ${EXTRA_LDFLAGS})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -g -fpie -O2")
SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -g -Wno-deprecated-declarations -Wreturn-type -fpie -fPIC -fpermissive -Wall -O2 -std=c++0x")
message(STATUS ${CMAKE_C_FLAGS})

LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/../../iotivity/out/linux/x86_64/release/)

# Rules for executable
ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
SET_TARGET_PROPERTIES (${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE "CXX")
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${ocfgw_LDFLAGS} -L. -pie -loc -loctbstack -loc_logger)
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(FILES ${PROJECT_SOURCE_DIR}/config.properties DESTINATION ${BINDIR})
INSTALL(FILES ${PROJECT_SOURCE_DIR}/oic_svr_db_client.json DESTINATION ${BINDIR})
INSTALL(FILES ${PROJECT_SOURCE_DIR}/oic_svr_db_server.json DESTINATION ${BINDIR})

add_custom_target(distclean
       COMMAND make clean
       COMMAND ${CMAKE_COMMAND} -P clean-all.cmake
)
