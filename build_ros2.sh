echo "生成ROS2 Package.xml"
cmake . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_WITH_CHANNEL_ROS2=ON \
    -DBUILD_WITH_CHANNEL_ROS1=OFF \
    -DBUILD_WITH_CHANNEL_PROTOBUF=OFF \
    -DBUILD_WITH_CHANNEL_AUTO=OFF
cmake --build build
