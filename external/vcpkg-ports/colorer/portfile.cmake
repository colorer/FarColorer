vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO colorer/Colorer-library
    REF 013dcaa2d393e6ce11df4cb9617a37606700703a
    SHA512 0e5172ee4c8c9887a9a35af2d5de0713e946fd75af4b21415d61c8b9e721477218717b62a090c1ef2bbb06fe83cdd0d80283627a3ef104e17376ef65ff496914
)


vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
    OPTIONS
        ${FEATURE_OPTIONS}
        -DCOLORER_BUILD_TOOLS=OFF
)

vcpkg_install_cmake()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
vcpkg_fixup_cmake_targets(CONFIG_PATH lib/cmake/colorer)
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
vcpkg_copy_pdbs()
