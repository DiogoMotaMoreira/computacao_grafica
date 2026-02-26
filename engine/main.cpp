#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <string>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;

// ==========================================
// ESTRUTURAS DE DADOS (Memória RAM)
// ==========================================
// O nosso vetor global que guardará todos os vértices lidos dos ficheiros .3d
// Formato: [X1, Y1, Z1, X2, Y2, Z2, ...]
vector<float> allVertices;

// ==========================================
// VARIÁVEIS GLOBAIS (Câmara e Janela)
// ==========================================
// Valores por defeito (caso o XML falhe ou omita algo)
int winW = 512, winH = 512;

float camPosX = 10.0f, camPosY = 10.0f, camPosZ = 10.0f;
float lookAtX = 0.0f, lookAtY = 0.0f, lookAtZ = 0.0f;
float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
float projFov = 60.0f, projNear = 1.0f, projFar = 1000.0f;

// ==========================================
// LEITURA DOS FICHEIROS .3D
// ==========================================
void load3DFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "# => ERRO: Nao foi possivel abrir o ficheiro 3D: " << filename << endl;
        return;
    }

    int numVertices;
    if (!(file >> numVertices)) {
        cout << "# => ERRO: Ficheiro " << filename << " vazio ou invalido!" << endl;
        return;
    }

    float x, y, z;
    // Ler os N vértices e adicionar ao nosso vetor global em memória
    for (int i = 0; i < numVertices; ++i) {
        file >> x >> y >> z;
        allVertices.push_back(x);
        allVertices.push_back(y);
        allVertices.push_back(z);
    }

    file.close();
    cout << "i => Carregado ficheiro: " << filename << " (" << numVertices << " vertices)" << endl;
}

// ==========================================
// LEITURA DO XML (TinyXML-2)
// ==========================================
void loadConfig(const char* xmlFilename) {
    XMLDocument doc;

    if (doc.LoadFile(xmlFilename) != XML_SUCCESS) {
        cout << "# => ERRO: Falha ao carregar o XML: " << xmlFilename << endl;
        exit(1);
    }

    XMLElement* world = doc.FirstChildElement("world");
    if (!world) {
        cout << "# => ERRO: Tag <world> nao encontrada no XML!" << endl;
        exit(1);
    }

    // 1. Ler Window
    XMLElement* window = world->FirstChildElement("window");
    if (window) {
        window->QueryIntAttribute("width", &winW);
        window->QueryIntAttribute("height", &winH);
    }

    // 2. Ler Camera
    XMLElement* camera = world->FirstChildElement("camera");
    if (camera) {
        XMLElement* pos = camera->FirstChildElement("position");
        if (pos) {
            pos->QueryFloatAttribute("x", &camPosX);
            pos->QueryFloatAttribute("y", &camPosY);
            pos->QueryFloatAttribute("z", &camPosZ);
        }

        XMLElement* look = camera->FirstChildElement("lookAt");
        if (look) {
            look->QueryFloatAttribute("x", &lookAtX);
            look->QueryFloatAttribute("y", &lookAtY);
            look->QueryFloatAttribute("z", &lookAtZ);
        }

        XMLElement* up = camera->FirstChildElement("up");
        if (up) {
            up->QueryFloatAttribute("x", &upX);
            up->QueryFloatAttribute("y", &upY);
            up->QueryFloatAttribute("z", &upZ);
        }

        XMLElement* proj = camera->FirstChildElement("projection");
        if (proj) {
            proj->QueryFloatAttribute("fov", &projFov);
            proj->QueryFloatAttribute("near", &projNear);
            proj->QueryFloatAttribute("far", &projFar);
        }
    }

    // 3. Ler Modelos
    XMLElement* group = world->FirstChildElement("group");
    if (group) {
        XMLElement* models = group->FirstChildElement("models");
        if (models) {
            // Ciclo para iterar sobre TODOS os <model> dentro de <models> (ex: test_1_5.xml tem dois)
            for (XMLElement* mod = models->FirstChildElement("model"); mod != nullptr; mod = mod->NextSiblingElement("model")) {
                const char* fileAttr = mod->Attribute("file");
                if (fileAttr) {
                    load3DFile(fileAttr);
                }
            }
        }
    }
}

// ==========================================
// FUNÇÕES GLUT
// ==========================================
void changeSize(int w, int h) {
    if (h == 0) h = 1;

    float ratio = w * 1.0 / h;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, w, h);

    // Usa os parâmetros dinâmicos lidos do XML!
    gluPerspective(projFov, ratio, projNear, projFar);

    glMatrixMode(GL_MODELVIEW);
}

void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    // Configurar câmara dinâmica
    gluLookAt(camPosX, camPosY, camPosZ,
        lookAtX, lookAtY, lookAtZ,
        upX, upY, upZ);

    // 1. Desenhar Eixos (Idêntico aos testes)
    glBegin(GL_LINES);
    // Eixo X - Vermelho
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-100.0f, 0.0f, 0.0f);
    glVertex3f(100.0f, 0.0f, 0.0f);

    // Eixo Y - Verde
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, -100.0f, 0.0f);
    glVertex3f(0.0f, 100.0f, 0.0f);

    // Eixo Z - Azul
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, -100.0f);
    glVertex3f(0.0f, 0.0f, 100.0f);
    glEnd();

    // 2. Desenhar a Geometria Carregada
    glColor3f(1.0f, 1.0f, 1.0f); // Branco (como nas imagens de teste)

    glBegin(GL_TRIANGLES);
    // Iteramos de 3 em 3 porque cada vértice tem (X, Y, Z)
    for (size_t i = 0; i < allVertices.size(); i += 3) {
        glVertex3f(allVertices[i], allVertices[i + 1], allVertices[i + 2]);
    }
    glEnd();

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    // 1. Validar Argumentos
    if (argc < 2) {
        cout << "# => ERRO: Ficheiro XML nao fornecido!" << endl;
        cout << "# => Uso: engine <ficheiro.xml>" << endl;
        return 1;
    }

    // 2. Leitura (única) dos dados de Configuração
    cout << "--- A INICIAR MOTOR 3D ---" << endl;
    loadConfig(argv[1]);

    // 3. Inicializar o GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);

    // Usar os valores de janela lidos do XML
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Motor 3D - CG Fase 1");

    // 4. Registar Callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);

    // 5. Configurações OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Wireframe obrigatório para a Fase 1 (tal como nas imagens dos testes)
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 6. Arrancar ciclo principal
    glutMainLoop();

    return 0;
}