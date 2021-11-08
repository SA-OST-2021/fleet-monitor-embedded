set(COMPONENT_SRCS "main.c" "file_server.c")
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()

target_add_binary_data(${COMPONENT_TARGET} "favicon.ico" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "upload_script.html" TEXT)

#idf_component_register(SRCS "main.c" "file_server.c"
#                    INCLUDE_DIRS "."
#                    EMBED_FILES "favicon.ico" "upload_script.html")

#FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.*)
#idf_component_register(SRCS ${app_sources})