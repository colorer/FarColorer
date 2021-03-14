vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO colorer/Colorer-library
    REF 9c15945ce6b55cbbdfadc0de6608eb001599a846
    SHA512 332b200e7c047eb9a5df40cfd6e74d6814b7b4d3e1a628922a6c5a3d70706f9cce6926acf205588b2ea827bf8aede813e31c9fe922f0956b8f3fe354396a9e09
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
