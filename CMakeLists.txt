cmake_minimum_required(VERSION 3.8)
project(scorpio_zadanie_rekrutacyjne_software)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(cv_bridge REQUIRED) # Example of importing cv_bridge
find_package(OpenCV REQUIRED)

# task 2 - ilmenite ratio
add_executable(ilmenite_ratio src/ilmenite_ratio.cpp)
target_link_libraries(ilmenite_ratio ${OpenCV_LIBRARIES})
target_include_directories(ilmenite_ratio PRIVATE ${OpenCV_INCLUDE_DIRS})
install(TARGETS
  ilmenite_ratio
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY assets/
  DESTINATION share/${PROJECT_NAME}/assets
)
find_package(ament_index_cpp REQUIRED)
ament_target_dependencies(ilmenite_ratio ament_index_cpp)

include_directories(include)

# Echo node component
add_library(echoNode SHARED src/echo_node.cpp)
ament_target_dependencies(echoNode
  rclcpp
  rclcpp_components
  std_msgs
  geometry_msgs
  sensor_msgs
  # cv_bridge
)

# cube detector component
add_library(cube_detector_component SHARED src/cube_detector_node.cpp)
ament_target_dependencies(cube_detector_component
  rclcpp
  rclcpp_components
  std_msgs
  geometry_msgs
  sensor_msgs
  cv_bridge
)

target_include_directories(cube_detector_component PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(cube_detector_component ${OpenCV_LIBRARIES})


rclcpp_components_register_node(echoNode PLUGIN "example::EchoNode" EXECUTABLE echo)
rclcpp_components_register_node(cube_detector_component PLUGIN "cube_detector::CubeDetector" EXECUTABLE cube_detector_node)

# Install libraries
install(TARGETS
  echoNode
  cube_detector_component
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

# Install config and launch files
install(DIRECTORY
  config
  launch
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
