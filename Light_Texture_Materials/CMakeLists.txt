cmake_minimum_required(VERSION 3.5)
PROJECT(TrabalhoCG)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Executáveis
add_executable(generator generator/main.cpp)
add_executable(engine engine/main.cpp engine/tinyxml2.cpp)

# OpenGL base
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})
link_directories(${OpenGL_LIBRARY_DIRS})
add_definitions(${OpenGL_DEFINITIONS})

if (WIN32)
    # Define a pasta principal (Seleciona C:/.../toolkits no CMake GUI)
    set(TOOLKITS_FOLDER "" CACHE PATH "Caminho para a pasta Toolkits")
    
    # 1. Pastas de Include
    # Adicionamos a pasta devil para que o #include <IL/il.h> funcione
    include_directories(${TOOLKITS_FOLDER}/glut)
    include_directories(${TOOLKITS_FOLDER}/glew)
    include_directories(${TOOLKITS_FOLDER}/devil) # ADICIONADO para DevIL

    # 2. Ligar as bibliotecas (.lib)
    target_link_libraries(engine 
        ${OPENGL_LIBRARIES} 
        ${TOOLKITS_FOLDER}/glut/glut32.lib 
        ${TOOLKITS_FOLDER}/glew/glew32.lib
        ${TOOLKITS_FOLDER}/devil/DevIL.lib # ADICIONADO para DevIL
    )
    
    # 3. Copiar as DLLs para a pasta do executável (.exe)
    # Essencial para o programa correr sem erros de "dll missing"
    if (EXISTS "${TOOLKITS_FOLDER}/glew/glew32.dll")
        file(COPY ${TOOLKITS_FOLDER}/glew/glew32.dll DESTINATION ${CMAKE_BINARY_DIR})
    endif()
    if (EXISTS "${TOOLKITS_FOLDER}/glut/glut32.dll" )
        file(COPY ${TOOLKITS_FOLDER}/glut/glut32.dll DESTINATION ${CMAKE_BINARY_DIR})
    endif()
    if (EXISTS "${TOOLKITS_FOLDER}/devil/devil.dll" )
        file(COPY ${TOOLKITS_FOLDER}/devil/devil.dll DESTINATION ${CMAKE_BINARY_DIR}) # ADICIONADO
    endif()

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT engine)

else (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
    find_package(GLUT REQUIRED)
    include_directories(${GLUT_INCLUDE_DIR})
    # Em Linux/Mac, a DevIL geralmente é instalada no sistema
    target_link_libraries(engine ${OPENGL_LIBRARIES} ${GLUT_LIBRARIES} IL) 
endif(WIN32)