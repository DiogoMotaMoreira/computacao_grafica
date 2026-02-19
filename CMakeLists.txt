cmake_minimum_required(VERSION 3.5)

# Nome do teu projeto
PROJECT(TrabalhoCG)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# -------------------------------------------------------------------
# 1. DEFINIR AS DUAS APLICAÇÕES
# -------------------------------------------------------------------
add_executable(generator generator/main.cpp)
add_executable(engine engine/main.cpp)

# -------------------------------------------------------------------
# 2. CONFIGURAR OPENGL (Igual às tuas aulas)
# -------------------------------------------------------------------
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif()

# -------------------------------------------------------------------
# 3. CONFIGURAR GLUT (E ligar apenas ao Motor!)
# -------------------------------------------------------------------
if (WIN32)
    message(STATUS "Toolkits_DIR set to: " ${TOOLKITS_FOLDER})
    # Variável que vai aparecer na interface do CMake GUI para tu preencheres
    set(TOOLKITS_FOLDER "" CACHE PATH "Path to Toolkits folder")
    
    if (NOT EXISTS "${TOOLKITS_FOLDER}/glut/GL/glut.h" OR NOT EXISTS "${TOOLKITS_FOLDER}/glut/glut32.lib")
        message(WARNING ": GLUT not found. Verifica a pasta TOOLKITS_FOLDER no CMake GUI.")
    endif ()    
    
    include_directories(${TOOLKITS_FOLDER}/glut)
    
    # ATENÇÃO: Ligamos o OpenGL e o GLUT APENAS à aplicação 'engine'
    target_link_libraries(engine ${OPENGL_LIBRARIES} ${TOOLKITS_FOLDER}/glut/glut32.lib)
    
    # Copia a DLL do GLUT para junto do executável do engine para ele poder correr
    if (EXISTS "${TOOLKITS_FOLDER}/glut/glut32.dll" )
        file(COPY ${TOOLKITS_FOLDER}/glut/glut32.dll DESTINATION ${CMAKE_BINARY_DIR})
    endif()    
    
    # Define o 'engine' como o projeto principal a arrancar no Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)
    
else (WIN32) # Linux e Mac
    set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    link_directories(${GLUT_LIBRARY_DIRS})
    add_definitions(${GLUT_DEFINITIONS})
    
    # Ligamos o OpenGL e o GLUT APENAS à aplicação 'engine'
    target_link_libraries(engine ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} )
    if(NOT GLUT_FOUND)
       message(ERROR ": GLUT not found!")
    endif()
endif(WIN32)





