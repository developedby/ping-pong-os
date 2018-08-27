set(CMAKE_CXX_FLAGS "-Wall -ansi -pedantic")

#Defining the executable's sources
set(APP_SOURCES
  testafila.c
  queue.c
)

include_directories(.)

#Creating the executable
set(APPLICATION_NAME testafila)
add_executable(${APPLICATION_NAME} ${APP_SOURCES})
