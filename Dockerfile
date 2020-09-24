FROM ubuntu:18.04

ARG OPENCV_VERSION=3.4.7

ARG RUNTIME_DEPS='pkg-config build-essential libpng-dev libjpeg-dev libopenblas-dev libx11-dev libwebp-dev libtiff5-dev libopenexr-dev libavcodec-dev libavformat-dev libswscale-dev libv4l-dev libxvidcore-dev libx264-dev libgtk-3-dev libatlas-base-dev gfortran'
ARG BUILD_DEPS='apt-utils wget unzip cmake'

# get opencv source - compile & install opencv - remove all files at the end
RUN apt-get update && apt-get install -y ${BUILD_DEPS} ${RUNTIME_DEPS} --no-install-recommends \
    && mkdir /opt/opencv && cd /opt/opencv \
    && wget -q -O opencv.zip https://github.com/opencv/opencv/archive/${OPENCV_VERSION}.zip \
    && wget -q -O opencv_contrib.zip https://github.com/opencv/opencv_contrib/archive/${OPENCV_VERSION}.zip \
    && unzip -qq opencv.zip && unzip -qq opencv_contrib.zip \
    && mv opencv-${OPENCV_VERSION} opencv && mv opencv_contrib-${OPENCV_VERSION} opencv_contrib \
    && mkdir opencv/build && cd opencv/build \
    && opencv_cmake_flags="-D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D BUILD_DOCS=OFF \
    -D BUILD_TESTS=OFF \
    -D BUILD_EXAMPLES=OFF \
    -D INSTALL_C_EXAMPLES=OFF \
    -D BUILD_PERF_TESTS=OFF \
    -D BUILD_JAVA=OFF \
    -D BUILD_opencv_apps=OFF \
    -D BUILD_opencv_aruco=OFF \
    -D BUILD_opencv_bgsegm=OFF \
    -D BUILD_opencv_bioinspired=OFF \
    -D BUILD_opencv_ccalib=OFF \
    -D BUILD_opencv_datasets=OFF \
    -D BUILD_opencv_dnn_objdetect=OFF \
    -D BUILD_opencv_dpm=OFF \
    -D BUILD_opencv_fuzzy=OFF \
    -D BUILD_opencv_hfs=OFF \
    -D BUILD_opencv_java_bindings_generator=OFF \
    -D BUILD_opencv_js=OFF \
    -D BUILD_opencv_img_hash=OFF \
    -D BUILD_opencv_line_descriptor=OFF \
    -D BUILD_opencv_optflow=OFF \
    -D BUILD_opencv_phase_unwrapping=OFF \
    -D BUILD_opencv_python3=OFF \
    -D BUILD_opencv_python_bindings_generator=OFF \
    -D BUILD_opencv_reg=OFF \
    -D BUILD_opencv_rgbd=OFF \
    -D BUILD_opencv_saliency=OFF \
    -D BUILD_opencv_shape=OFF \
    -D BUILD_opencv_stereo=OFF \
    -D BUILD_opencv_stitching=OFF \
    -D BUILD_opencv_structured_light=OFF \
    -D BUILD_opencv_superres=OFF \
    -D BUILD_opencv_surface_matching=OFF \
    -D BUILD_opencv_ts=OFF \
    -D BUILD_opencv_xobjdetect=OFF \
    -D BUILD_opencv_xphoto=OFF \
    -D OPENCV_EXTRA_MODULES_PATH=/opt/opencv/opencv_contrib/modules" \
    && cmake $opencv_cmake_flags .. \
    && make -j8 && make install \
    && cd / \
    && rm -rf /opt/opencv \
    && ldconfig

# remove build deps & purge apt - leave only runtime deps
RUN apt-get purge -y --auto-remove $BUILD_DEPS \
    && apt-get autoremove -y --purge \
    && apt-get install -y $RUNTIME_DEPS --no-install-recommends \
    && rm -rf /var/lib/apt/lists/* /usr/share/man /usr/local/share/man /tmp/*

WORKDIR /home/magic
CMD ["bash start.sh"]