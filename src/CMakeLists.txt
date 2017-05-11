include_directories(
  ${Magick++_INCLUDE_DIRS}
  )

add_executable(pixview pixview.cpp)

target_link_libraries(pixview
  ${CAIRO_LIBRARIES}
  ${SDL2_LIBRARY}
  ${SDL2_IMAGE_LIBRARY}
  ${Magick++_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  )
