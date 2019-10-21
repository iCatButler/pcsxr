message(STATUS "* Configuring peopsxgl")

include(GlibCompileResourcesSupport)

#deps
find_package(GLib REQUIRED)
if(NOT GLib_FOUND)
    message(FATAL_ERROR "GLIB2 library not found")
endif(NOT GLib_FOUND)
include_directories(${GLib_INCLUDE_DIRS})

find_package(GTK3 REQUIRED)
if(NOT GTK3_FOUND)
    message(FATAL_ERROR "GTK3 library not found")
endif(NOT GTK3_FOUND)
include_directories(${GTK3_INCLUDE_DIRS})
set(GTK_LIBRARIES ${GTK3_LIBRARY} ${GDK3_LIBRARY} ${GDKPixbuf_LIBRARY} ${Pango_LIBRARY} ${Cairo_LIBRARY} ${GObject_LIBRARY} ${GLib_LIBRARY} ${GIO_LIBRARY})

find_package(X11 REQUIRED)

set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(FATAL_ERROR "GL library not found")
endif(NOT OPENGL_FOUND)
include_directories(${OPENGL_INCLUDE_DIR})

#defs
add_definitions(-DLOCALE_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/locale/" -DPSEMU_DATA_DIR="${CMAKE_INSTALL_FULL_DATAROOTDIR}/psemu" -DDEF_PLUGIN_DIR="${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu")

#sources
set(PLUGIN_SRCS
    cfg.c
    draw.c
    fps.c
    gpu.c
    gte_accuracy.c
    key.c
    menu.c
    prim.c
    soft.c
    texture.c
    pgxp_gpu.c
)

set(GUI_SRCS
    gpucfg/main.c
)


#resources
set(RESOURCE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/gpucfg/peopsxgl.ui
)


compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE EMBED_C
                   PREFIX /org/pcsxr/peopsxgl
                   SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/gpucfg
                   RESOURCES ${RESOURCE_LIST})

add_custom_target(peopsxgl_resource DEPENDS ${RESOURCE_FILE})

include_directories(${CMAKE_SOURCE_DIR}/libpcsxcore ${X11_X11_INCLUDE_PATH} ${X11_xf86vmode_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})
add_library(peopsxgl MODULE ${PLUGIN_SRCS})
target_link_libraries(peopsxgl ${X11_LIBRARIES} ${X11_Xxf86vm_LIB} ${OPENGL_gl_LIBRARY})


add_executable(cfgpeopsxgl ${GUI_SRCS} ${RESOURCE_FILE})
add_dependencies(cfgpeopsxgl dfinput_resource)
target_link_libraries(cfgpeopsxgl ${GTK_LIBRARIES})

install(TARGETS peopsxgl LIBRARY DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
install(TARGETS cfgpeopsxgl RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/games/psemu)
