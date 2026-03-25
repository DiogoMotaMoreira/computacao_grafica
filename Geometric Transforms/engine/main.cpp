

// ==========================================
//        NOTAS A LER PARA A FASE 2:
// ==========================================
// Problemas a resolver
// ==========================================
// problema do allvertices => aqui metemos todos os vertices das figuras
//      - quando chega a hora de desenhar (renderScene), o OpenGL desenha esses vertices todos de uma vez na origem (0,0,0)
//      - na fase 2 temos que aplicar transformações... 
//      - se todos os vertices tiverem misturados no mesmo vector, não conseguimos mover a esfera sem mover o cone ao mesmo tempo (isto quando temos 2 figuras em allvertices)
// ==========================================
// coisas a mudar para a fase 2
// ==========================================
// - mudar a memória 
//      - precisamos definir c++ structs ou classes que representem um GRUPO
//      - o GRUPO precisa guardar as transformações, os nomes dos modeles, uma lista de GRUPOS filhos (subgrupos)
// - mudar o carregamento de ficheiros
//      - se tivermos 8 planetas a usar o fihceiro sphere.3d, não devemos ler 8x o mesmo ficheiro
//      - podemos usar dicionário (std :: map<string, vector<floar>>) para carregar cada ficheiro .3d apenas uma vez (uma mini cache)
// - mudar para o Parser XML (recursividade)
//      - na fase 1 temos um <group> -> <models> -> <model>  |  isto é um caminho fixo
//      - nesta fase podemos ter grupos dentro de grupos por isso temos de criar uma função de leitura recursiva no tinyxml2 para que ele consiga navegar na árvore
// ==========================================




#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <string>
#include <map>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "tinyxml2.h"

using namespace std;
using namespace tinyxml2;


// ==========================================
// ESTRUTURAS DE DADOS -> fase2
// ==========================================

struct Transform {
    string type;
    float x, y, z;
    float angle;
};

struct ModelInfo {
    string filename;
    string type;
};

struct Group {
    vector<Transform> transforms;
    vector<ModelInfo> models;
    vector<Group> children;
};

Group sceneRoot;

map<string, vector<float>> modelsData;

// ==========================================
// VARIÁVEIS GLOBAIS (Câmara e Janela)
// ==========================================
// Valores por defeito (caso o XML falhe ou omita algo)
int winW = 512, winH = 512;

float camPosX = 10.0f, camPosY = 10.0f, camPosZ = 10.0f;
float lookAtX = 0.0f, lookAtY = 0.0f, lookAtZ = 0.0f;
float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
float projFov = 60.0f, projNear = 1.0f, projFar = 1000.0f;

float camAlpha = 0.0f, camBeta = 0.5f, camRadius = 50.0f;
int startX, startY, tracking = 0;
// ==========================================
// LEITURA DOS FICHEIROS .3D  -> fase2
// ==========================================
void load3DFile(const string& filename) {

    // se o ficheiro ja foi lido não vamos carregar de novo
    if (modelsData.count(filename) > 0) {
        return;
    }

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

    // vetor temporário so para vertices deste modelo
    vector<float> currentModelVertices;
    float x, y, z;


    // Ler os N vértices e adicionar ao nosso vetor global em memória
    for (int i = 0; i < numVertices; ++i) {
        file >> x >> y >> z;
        currentModelVertices.push_back(x);
        currentModelVertices.push_back(y);
        currentModelVertices.push_back(z);
    }

    file.close();
    modelsData[filename] = currentModelVertices;
    cout << "i => Carregado ficheiro: " << filename << " (" << numVertices << " vertices)" << endl;
}

// ==========================================
// parser recursivo - fase2
// ==========================================
Group parseGroup(XMLElement* groupElement) {
    Group node;

    // ler transformações
    XMLElement* transformElement = groupElement->FirstChildElement("transform"); // procura o primeiro filho transform na arvore
    if (transformElement) {
        // ler todos os elementos dentro do transform
        for (XMLElement* t = transformElement->FirstChildElement(); t != nullptr; t = t->NextSiblingElement()) {
            Transform trans;
            trans.type = t->Name(); // guardar se é translate, rotate, ...

            trans.x = t->FloatAttribute("x", 0.0f);
            trans.y = t->FloatAttribute("y", 0.0f);
            trans.z = t->FloatAttribute("z", 0.0f);

            if (trans.type == "rotate") {
                trans.angle = t->FloatAttribute("angle", 0.0f);
            }
            else {
                trans.angle = 0.0f;
            }

            node.transforms.push_back(trans);
        }
    }

    // ler os modelos .3d
    // ler os modelos .3d
    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        for (XMLElement* m = modelsElement->FirstChildElement("model"); m != nullptr; m = m->NextSiblingElement("model")) {
            const char* fileAttr = m->Attribute("file");
            const char* typeAttr = m->Attribute("type");

            if (fileAttr) {
                ModelInfo info;
                info.filename = fileAttr;
                // Se o atributo type existir no XML, guarda-o. Se não, assume que é "solid"
                info.type = (typeAttr != nullptr) ? typeAttr : "solid";

                node.models.push_back(info);
                load3DFile(info.filename);
            }
        }
    }

    // ler subgrupos
    for (XMLElement* childGroup = groupElement->FirstChildElement("group"); childGroup != nullptr ; childGroup = childGroup->NextSiblingElement("group")) {
        // chamar função para ler filho (recursividade)
        Group childNode = parseGroup(childGroup);
        // guardar o resultado na lista dos filhos
        node.children.push_back(childNode);
    }

    return node;
}



// ==========================================
// LEITURA DO XML (TinyXML-2) - fase 2
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

    // 3. Ler Modelos - fase 2
    XMLElement* mainGroup = world->FirstChildElement("group");
    if (mainGroup) {
        // sceneRoot é a nossa var global do tipo group
        sceneRoot = parseGroup(mainGroup);
        cout << "i => Arvore de cena carregada com sucesso!" << endl;
    }
    else {
        cout << "i => AVISO: Nenhum <group> principal encontrado no XML" << endl;
    }
}

// ==========================================
// DESENHO RECURSIVO DA ÁRVORE - fase 2
// ==========================================
void drawGroup(const Group& group) {
    // guardar o estado atual do mundo 
    glPushMatrix();

    // aplicar as transformações deste grupo
    for (const Transform& t : group.transforms) {
        if (t.type == "translate") {
            glTranslatef(t.x, t.y, t.z);
        }
        else if (t.type == "rotate") {
            glRotatef(t.angle, t.x, t.y, t.z);
        }
        else if (t.type == "scale") {
            glScalef(t.x, t.y, t.z);
        }
    }

    // desenhar os modelos pertencentes a este grupo
    for (const ModelInfo& mod : group.models) {
        if (modelsData.count(mod.filename) > 0) {
            const vector<float>& vertices = modelsData[mod.filename];

            // VERIFICA O TIPO PARA DECIDIR COMO DESENHAR
            if (mod.type == "line") {
                glBegin(GL_LINE_LOOP); // Desenha a ligar os pontos em anel
                glColor3f(0.3f, 0.3f, 0.3f); // Pinta a órbita de cinzento
            }
            else {
                glBegin(GL_TRIANGLES); // Desenho normal
                glColor3f(1.0f, 1.0f, 1.0f); // Pinta o planeta de branco
            }

            for (size_t i = 0; i < vertices.size(); i += 3) {
                glVertex3f(vertices[i], vertices[i + 1], vertices[i + 2]);
            }
            glEnd();
        }
    }

    // desenhar os subgrupos (filhos)
    for (const Group& child : group.children) {
        drawGroup(child);
    }

    glPopMatrix(); // isto serve para garantir que quando formos desenhar o proximo grupo, ele não herdar as tranformações deste
}

// ==========================================
// CONTROLO DA CÂMARA ORBITAL
// ==========================================
void updateCameraPos() {
    // Calcular o X, Y, Z com base nos ângulos (alpha, beta) e no raio
    camPosX = lookAtX + camRadius * cos(camBeta) * sin(camAlpha);
    camPosY = lookAtY + camRadius * sin(camBeta);
    camPosZ = lookAtZ + camRadius * cos(camBeta) * cos(camAlpha);
}

void initCamera() {
    // Converter o XYZ lido do XML para coordenadas esféricas iniciais
    float dx = camPosX - lookAtX;
    float dy = camPosY - lookAtY;
    float dz = camPosZ - lookAtZ;

    camRadius = sqrt(dx * dx + dy * dy + dz * dz);
    if (camRadius == 0) camRadius = 1.0f; // Prevenir divisão por zero

    camBeta = asin(dy / camRadius);
    camAlpha = atan2(dx, dz);
}

void processMouseButtons(int button, int state, int xx, int yy) {
    if (state == GLUT_DOWN) {
        startX = xx;
        startY = yy;
        if (button == GLUT_LEFT_BUTTON) {
            tracking = 1; // Rotação (Orbit)
        }
        else if (button == GLUT_RIGHT_BUTTON) {
            tracking = 2; // Zoom
        }
        else {
            tracking = 0;
        }
    }
    else if (state == GLUT_UP) {
        tracking = 0;
    }
}

void processMouseMotion(int xx, int yy) {
    if (!tracking) return;

    int deltaX = xx - startX;
    int deltaY = yy - startY;

    if (tracking == 1) { // Botão Esquerdo: Rodar
        camAlpha -= deltaX * 0.01f;
        camBeta += deltaY * 0.01f;

        if (camBeta > 1.5f) camBeta = 1.5f;
        else if (camBeta < -1.5f) camBeta = -1.5f;
    }
    else if (tracking == 2) { // Botão Direito: Zoom
        camRadius += deltaY * 0.1f;
        if (camRadius < 1.0f) camRadius = 1.0f;
    }

    startX = xx;
    startY = yy;

    updateCameraPos();
    glutPostRedisplay();
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

    gluPerspective(projFov, ratio, projNear, projFar);

    glMatrixMode(GL_MODELVIEW);
}

// fase 2
void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glLoadIdentity();

    // Configurar câmara dinâmica
    gluLookAt(camPosX, camPosY, camPosZ,
        lookAtX, lookAtY, lookAtZ,
        upX, upY, upZ);

    // 1. Desenhar Eixos
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
    glColor3f(1.0f, 1.0f, 1.0f); // Branco

    drawGroup(sceneRoot);

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
    
    initCamera();

    // 3. Inicializar o GLUT
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);

    glutInitWindowSize(winW, winH);
    glutCreateWindow("Motor 3D - CG Fase 1");

    // 4. Registar Callbacks
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);

    glutMouseFunc(processMouseButtons);
    glutMotionFunc(processMouseMotion);

    // 5. Configurações OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 6. Arrancar ciclo principal
    glutMainLoop();

    return 0;
}