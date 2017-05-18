include_directories(${Magick++_INCLUDE_DIRS})
include_directories(${CAIRO_INCLUDE_DIR})
include_directories(${SDL2_INCLUDE_DIR})

add_executable(pixview
  render.cpp
  pixview.cpp
  geo.cpp)

target_link_libraries(pixview
  ${CAIRO_LIBRARIES}
  ${SDL2_LIBRARY}
#  ${SDL2_IMAGE_LIBRARY}
  ${Magick++_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  )
