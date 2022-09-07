# The minimum required version hasn't been tested. Feel free to adjust
# downwards if necessary.
cmake_minimum_required(VERSION 3.0)
project(file-share-system C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

add_subdirectory(facil.io)

include(FindPkgConfig)

pkg_check_modules(OPENSSL REQUIRED openssl)
foreach(FLAG ${OPENSSL_CFLAGS_OTHER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
endforeach()
link_directories(${OPENSSL_LIBRARY_DIRS})

pkg_check_modules(ZDB REQUIRED zdb)
foreach(FLAG ${ZDB_CFLAGS_OTHER})
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}")
endforeach()
link_directories(${ZDB_LIBRARY_DIRS})

include_directories(include src)

add_executable(${PROJECT_NAME} main.c type.c db.c fss.c util.c)

target_include_directories(${PROJECT_NAME} PRIVATE ${ZDB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${ZDB_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})

target_include_directories(${PROJECT_NAME} PRIVATE facil.io)
target_link_libraries(${PROJECT_NAME} PRIVATE facil.io)
