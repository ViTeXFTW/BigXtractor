vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO ViTeXFTW/BigXtractor
    REF "v${VERSION}"
    SHA512 0  # TODO: Update this hash when creating a release (run CI or download and hash the release tarball)
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DINSTALL_STANDALONE=ON
        -DBUILD_TESTING=OFF
        -DBUILD_EXAMPLES=OFF
)

vcpkg_cmake_install()

vcpkg_cmake_config_fixup(PACKAGE_NAME bigx CONFIG_PATH lib/cmake/bigx)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")