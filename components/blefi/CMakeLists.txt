

if(IDF_TARGET STREQUAL "esp32s3")

    idf_component_register(INCLUDE_DIRS "include")

    add_prebuilt_library(prebuilt "lib-s3/libblefi.a"
                            REQUIRES "freertos" "bt" "log")

    target_link_libraries(${COMPONENT_LIB} INTERFACE prebuilt)

    # set(srcs "src/blefi.cc")
    # set(includes "include")

    # idf_component_register(SRCS "${srcs}"
    #                     INCLUDE_DIRS "${includes}"
    #                     REQUIRES "freertos" "bt"
    #                     )

elseif(IDF_TARGET STREQUAL "esp32c3")

    idf_component_register(INCLUDE_DIRS "include")

    add_prebuilt_library(prebuilt "lib-c3/libblefi.a"
                            REQUIRES "freertos" "bt" "log")

    target_link_libraries(${COMPONENT_LIB} INTERFACE prebuilt)

    # set(srcs "src/blefi.cc")
    # set(includes "include")

    # idf_component_register(SRCS "${srcs}"
    #                     INCLUDE_DIRS "${includes}"
    #                     REQUIRES "freertos" "bt"
    #                     )

else()
    set(srcs "src/blefi.cc")
    set(includes "include")

    idf_component_register(SRCS "${srcs}"
                        INCLUDE_DIRS "${includes}"
                        REQUIRES "freertos" "bt"
                        )
endif()