#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <math.h>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "tinyxml2.h"



using namespace std;
using namespace tinyxml2;


// ==========================================
// ESTRUTURAS DE DADOS
// ==========================================

struct Transform {
    string type;
    float x, y, z;
    float angle;
    float time; // para transla��es e reota��es com tempo
    bool align = false;
    vector<float*> points; // Catmull-Rom
    mutable float prev_y[3] = { 0.0f, 1.0f, 0.0f };
    mutable GLuint orbitVBO = 0;    
    mutable int    orbitVertCount = 0;
};

struct ModelInfo {
    string filename;
    string type;
    int id;
    string name;
};

struct Group {
    vector<Transform> transforms;
    vector<ModelInfo> models;
    vector<Group> children;
};

struct Target {
    string name;
    float x, y, z;
    float radius;
};

struct VBOModel {
    GLuint bufferID; 
    int vertexCount;   
};

std::vector<Target> cameraTargets;
int currentTargetIndex = 0;        // N�mero do astro 

Group sceneRoot;

map<string, vector<float>> modelsData;

map<string, VBOModel> modelsVBO;

// Variaveis para Color Picking
int globalModelID = 1; // 0 e o fundo
int selectedID = 0;
float selectedX = 0.0f, selectedY = 0.0f, selectedZ = 0.0f;
string selectedName = "";
map<int, string> idToName;

// ==========================================
// VARI�VEIS GLOBAIS (C�mara e Janela)
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
// LEITURA DOS FICHEIROS .3d
// ==========================================
void load3DFile(const string& filename) {

    // se o ficheiro ja foi lido n�o vamos carregar de novo
    if (modelsVBO.count(filename) > 0) return;

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

    // vetor tempor�rio so para vertices deste modelo
    vector<float> currentModelVertices;
    float x, y, z;


    // Ler os N v�rtices e adicionar ao nosso vetor global em mem�ria
    for (int i = 0; i < numVertices; ++i) {
        file >> x >> y >> z;
        currentModelVertices.push_back(x);
        currentModelVertices.push_back(y);
        currentModelVertices.push_back(z);
    }

    VBOModel model;
    model.vertexCount = currentModelVertices.size() / 3;

    glGenBuffers(1, &model.bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, model.bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * currentModelVertices.size(), currentModelVertices.data(), GL_STATIC_DRAW);

    modelsVBO[filename] = model;
    cout << "i => VBO criado: " << filename << " (" << model.vertexCount << " vertices)" << endl;
}

// ==========================================
// parser recursivo -> mudan�a em primeiro if para nova fase 3
// ==========================================
Group parseGroup(XMLElement* groupElement) {
    Group node;

    // ler transforma��es
    XMLElement* transformElement = groupElement->FirstChildElement("transform"); // procura o primeiro filho transform na arvore
    if (transformElement) {
        // ler todos os elementos dentro do transform
        for (XMLElement* t = transformElement->FirstChildElement(); t != nullptr; t = t->NextSiblingElement()) {
            Transform trans;
            trans.type = t->Name();
            trans.time = t->FloatAttribute("time", 0.0f); // L� o tempo se existir

            if (trans.type == "translate") {
                if (trans.time > 0) {
                    const char* alignAttr = t->Attribute("align");
                    trans.align = (alignAttr != nullptr &&
                        (string(alignAttr) == "True" ||
                         string(alignAttr) == "true" ||
                         string(alignAttr) == "1"));
                    // L pontos de controlo para Catmull-Rom
                    for (XMLElement* p = t->FirstChildElement("point"); p != nullptr; p = p->NextSiblingElement("point")) {
                        float* pt = new float[3];
                        pt[0] = p->FloatAttribute("x");
                        pt[1] = p->FloatAttribute("y");
                        pt[2] = p->FloatAttribute("z");
                        trans.points.push_back(pt);
                    }
                }
                else {
                    // Translao esttica (time = 0), verificar se tem <point> ou se est no prprio <translate>
                    XMLElement* p = t->FirstChildElement("point");
                    if (p) {
                        trans.x = p->FloatAttribute("x", 0.0f);
                        trans.y = p->FloatAttribute("y", 0.0f);
                        trans.z = p->FloatAttribute("z", 0.0f);
                    } else {
                        trans.x = t->FloatAttribute("x", 0.0f);
                        trans.y = t->FloatAttribute("y", 0.0f);
                        trans.z = t->FloatAttribute("z", 0.0f);
                    }
                }
            }
            else if (trans.type == "rotate") {
                trans.angle = t->FloatAttribute("angle", 0.0f);
                trans.x = t->FloatAttribute("x", 0.0f);
                trans.y = t->FloatAttribute("y", 0.0f);
                trans.z = t->FloatAttribute("z", 0.0f);
            }
            else {
                trans.x = t->FloatAttribute("x", 0.0f);
                trans.y = t->FloatAttribute("y", 0.0f);
                trans.z = t->FloatAttribute("z", 0.0f);
            }
            node.transforms.push_back(trans);
        }
    }

    // ler os modelos .3d
    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        for (XMLElement* m = modelsElement->FirstChildElement("model"); m != nullptr; m = m->NextSiblingElement("model")) {
            const char* fileAttr = m->Attribute("file");
            const char* typeAttr = m->Attribute("type");

            if (fileAttr) {
                ModelInfo info;
                info.filename = fileAttr;
                // Se o atributo type existir no XML, guarda-o. Se n�o, assume que � "solid"
                info.type = (typeAttr != nullptr) ? typeAttr : "solid";
                
                // Nome do modelo e ID incremental
                const char* nameAttr = m->Attribute("name");
                info.name = (nameAttr != nullptr) ? nameAttr : info.filename; 
                info.id = globalModelID++;
                idToName[info.id] = info.name;

                node.models.push_back(info);
                load3DFile(info.filename);
            }
        }
    }

    // ler subgrupos
    for (XMLElement* childGroup = groupElement->FirstChildElement("group"); childGroup != nullptr ; childGroup = childGroup->NextSiblingElement("group")) {
        // chamar fun��o para ler filho (recursividade)
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

    if (camera) {
        XMLElement* waypointsElem = camera->FirstChildElement("waypoints");
        if (waypointsElem) {
            for (XMLElement* target = waypointsElem->FirstChildElement("target"); target != nullptr; target = target->NextSiblingElement("target")) {
                Target t;
                t.name = target->Attribute("name");
                t.x = target->FloatAttribute("x");
                t.y = target->FloatAttribute("y");
                t.z = target->FloatAttribute("z");
                t.radius = 1.0f; // Se falhar, fica a 1.0
                target->QueryFloatAttribute("radius", &t.radius);
                cameraTargets.push_back(t);
            }
        }
    }

    // 3. Ler Modelos - fase 2
    XMLElement* mainGroup = world->FirstChildElement("group");
    if (mainGroup) {
        // sceneRoot � a nossa var global do tipo group
        sceneRoot = parseGroup(mainGroup);
        cout << "i => Arvore de cena carregada com sucesso!" << endl;
    }
    else {
        cout << "i => AVISO: Nenhum <group> principal encontrado no XML" << endl;
    }
}

// ==========================================
// Fun��es auxiliares sugeridas nos slides -< fase 3 parte da matematica C-R
// ==========================================
void cross(float* a, float* b, float* res) {
    res[0] = a[1] * b[2] - a[2] * b[1];
    res[1] = a[2] * b[0] - a[0] * b[2];
    res[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize(float* a) {
    float l = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    if (l != 0) { a[0] /= l; a[1] /= l; a[2] /= l; }
}

void buildRotMatrix(float* x, float* y, float* z, float* m) {
    m[0] = x[0]; m[1] = x[1]; m[2] = x[2]; m[3] = 0;
    m[4] = y[0]; m[5] = y[1]; m[6] = y[2]; m[7] = 0;
    m[8] = z[0]; m[9] = z[1]; m[10] = z[2]; m[11] = 0;
    m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

// ==========================================
// Fun��es para Catmull
// ==========================================
void getCatmullRomPoint(float t, float* p0, float* p1, float* p2, float* p3, float* pos, float* deriv) {
    // matriz igual a gui�o
    float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f},
                      { 1.0f, -2.5f,  2.0f, -0.5f},
                      {-0.5f,  0.0f,  0.5f,  0.0f},
                      { 0.0f,  1.0f,  0.0f,  0.0f} };

    for (int i = 0; i < 3; ++i) {
        float p[4] = { p0[i], p1[i], p2[i], p3[i] };
        float a[4];

        // multiplicar o a = M * P
        for (int j = 0; j < 4; ++j) {
            a[j] = m[j][0] * p[0] + m[j][1] * p[1] + m[j][2] * p[2] + m[j][3] * p[3];
        }

        // pos = T * A
        pos[i] = pow(t, 3) * a[0] + pow(t, 2) * a[1] + t * a[2] + a[3];

        // deriv = T' * A
        deriv[i] = 3 * pow(t, 2) * a[0] + 2 * t * a[1] + a[2];
    }
}




void applyCatmullTransform(vector<float*> points, float time_duration, bool doAlign, float* prev_y) {
    float elapsed_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float t_global = fmod(elapsed_time, time_duration) / time_duration;

    int num_points = points.size();
    float t = t_global * num_points;
    int index = floor(t);
    t = t - index;

    int indices[4];
    for (int i = 0; i < 4; ++i) indices[i] = (index + i - 1 + num_points) % num_points;

    float pos[3], deriv[3];
    getCatmullRomPoint(t, points[indices[0]], points[indices[1]], points[indices[2]], points[indices[3]], pos, deriv);

    glTranslatef(pos[0], pos[1], pos[2]);

    // só aplica o referencial de Frenet se align="True"
    if (doAlign) {
        float x[3], y[3], z[3];
        for (int i = 0; i < 3; i++) x[i] = deriv[i];
        normalize(x);

        cross(x, prev_y, z);
        normalize(z);
        cross(z, x, y);
        normalize(y);

        for (int i = 0; i < 3; i++) prev_y[i] = y[i]; // atualiza o prev_y DESTE transform

        float m[16];
        buildRotMatrix(x, y, z, m);
        glMultMatrixf(m);
    }
}

// ==========================================
// Desenhar a linha para orbitas
// ==========================================
void renderCatmullRomCurve(const vector<float*>& points, GLuint& vboID, int& vertCount) {

    // Gerar o VBO apenas na primeira chamada (lazy init)
    if (vboID == 0) {
        vector<float> verts;
        float pos[3], deriv[3];
        int n = points.size();

        for (float gt = 0.0f; gt < 1.0f; gt += 0.001f) {
            float tg = gt * n;
            int idx = (int)floor(tg);
            float t = tg - idx;

            int ids[4];
            for (int i = 0; i < 4; ++i)
                ids[i] = (idx + i - 1 + n) % n;

            getCatmullRomPoint(t,
                points[ids[0]], points[ids[1]],
                points[ids[2]], points[ids[3]],
                pos, deriv);

            verts.push_back(pos[0]);
            verts.push_back(pos[1]);
            verts.push_back(pos[2]);
        }

        vertCount = (int)(verts.size() / 3);
        glGenBuffers(1, &vboID);
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        glBufferData(GL_ARRAY_BUFFER,
            sizeof(float) * verts.size(),
            verts.data(),
            GL_STATIC_DRAW);
    }

    // Desenhar com VBO
    glColor3f(0.5f, 0.5f, 0.5f);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glDrawArrays(GL_LINE_LOOP, 0, vertCount);
}


// ==========================================
// DESENHO RECURSIVO DA �RVORE - fase 3
// ==========================================
void drawGroup(const Group& group, int renderMode = 0) {
    // guardar o estado atual do mundo 
    glPushMatrix();

    // aplicar as transforma��es deste grupo   -> fase 3 (mudan�a)
    for (const Transform& t : group.transforms) {
        if (t.type == "translate") {
            if (t.time > 0 && t.points.size() >= 4) {
                // Desenha a linha da rbita apenas no render normal (renderMode == 0)
                if (renderMode == 0) {
                    renderCatmullRomCurve(t.points, t.orbitVBO, t.orbitVertCount);
                }
                // Depois move o objeto para a sua posi��o atual na curva
                applyCatmullTransform(t.points, t.time, t.align,t.prev_y);
            }
            else {
                glTranslatef(t.x, t.y, t.z);
            }
        }
        else if (t.type == "rotate") {
            if (t.time > 0) {
                // Rota��o cont�nua baseada no tempo: (tempo_atual * 360) / tempo_total
                float elapsed = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
                float angle = (elapsed * 360.0f) / t.time;
                glRotatef(angle, t.x, t.y, t.z);
            }
            else {
                glRotatef(t.angle, t.x, t.y, t.z);
            }
        }
        else if (t.type == "scale") {
            glScalef(t.x, t.y, t.z);
        }
    }

    // desenhar os modelos pertencentes a este grupo
    for (const ModelInfo& mod : group.models) {
        if (modelsVBO.count(mod.filename) > 0) {
            VBOModel vbo = modelsVBO[mod.filename];

            if (renderMode == 2) {
                // Extrair coordenadas reais do mundo ANTES da camara ser aplicada
                if (mod.id == selectedID) {
                    float m[16];
                    glGetFloatv(GL_MODELVIEW_MATRIX, m);
                    selectedX = m[12];
                    selectedY = m[13];
                    selectedZ = m[14];
                }
            } else if (renderMode == 1) {
                glColor3ub(mod.id, 0, 0); // Pintar com o ID para deteção
            } else {
                glColor3f(1.0f, 1.0f, 1.0f); // Default para geometria visivel
            }

            if (renderMode != 2) {
                // 1. Indicar qual o VBO ativo
                glBindBuffer(GL_ARRAY_BUFFER, vbo.bufferID);
                // 2. Definir a semantica: 3 floats por vertice
                glVertexPointer(3, GL_FLOAT, 0, 0);

                // Se o tipo for "line", desenhamos uma linha continua
                if (mod.type == "line") {
                    glDrawArrays(GL_LINE_LOOP, 0, vbo.vertexCount);
                } else {
                    glDrawArrays(GL_TRIANGLES, 0, vbo.vertexCount);
                }
            }
        }
    }

    // desenhar os subgrupos (filhos)
    for (const Group& child : group.children) {
        drawGroup(child, renderMode);
    }

    glPopMatrix(); // isto serve para garantir que quando formos desenhar o proximo grupo, ele n�o herdar as tranforma��es deste
}

// ==========================================
// HUD E COLOR PICKING
// ==========================================
void renderText(string text) {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    gluOrtho2D(0, viewport[2], 0, viewport[3]);
    
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    
    glDisable(GL_DEPTH_TEST);
    glColor3f(1.0f, 1.0f, 1.0f);
    
    glRasterPos2d(10, viewport[3] - 25); // HUD no canto superior esquerdo
    for (char c : text) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }
    
    glEnable(GL_DEPTH_TEST);
    
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void picking(int x, int y) {
    // Guarda os estados que vamos alterar (Luzes, texturas e modo de polígono)
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    // Força desenhar polígonos preenchidos. Tentar clicar numa "linha" de wireframe é quase impossível!
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Limpar cor E PROFUNDIDADE! Como os planetas mexem (Cena Animada), 
    // usar o Depth do frame anterior causaria bugs de oclusão nas posições novas.
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    glDepthFunc(GL_LESS);

    glPushMatrix();
    glLoadIdentity();
    gluLookAt(camPosX, camPosY, camPosZ, lookAtX, lookAtY, lookAtZ, upX, upY, upZ);

    drawGroup(sceneRoot, 1); // Pass 1: Desenho para picking (escondido)

    glPopMatrix();

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    unsigned char res[4];
    // A coordenada Y do rato no glut começa em cima, no glReadPixels começa em baixo. O -1 previne "off-by-one"
    glReadPixels(x, viewport[3] - y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, res);

    int pickedID = res[0];
    if (pickedID > 0 && idToName.count(pickedID) > 0) {
        selectedID = pickedID;
        selectedName = idToName[pickedID];
    }
    else {
        // Clicou no fundo do espaco, seleciona o Sol por defeito
        for (auto const& pair : idToName) {
            if (pair.second == "Sol") {
                selectedID = pair.first;
                selectedName = pair.second;
                break;
            }
        }
    }

    // Restaura o estado anterior (desativa luzes caso estivessem desligadas e volta ao GL_LINE)
    glPopAttrib();
}


// ==========================================
// CONTROLO DA C�MARA ORBITAL
// ==========================================
void updateCameraPos() {
    // Calcular o X, Y, Z com base nos �ngulos (alpha, beta) e no raio
    camPosX = lookAtX + camRadius * cos(camBeta) * sin(camAlpha);
    camPosY = lookAtY + camRadius * sin(camBeta);
    camPosZ = lookAtZ + camRadius * cos(camBeta) * cos(camAlpha);
}

void initCamera() {
    // Converter o XYZ lido do XML para coordenadas esf�ricas iniciais
    float dx = camPosX - lookAtX;
    float dy = camPosY - lookAtY;
    float dz = camPosZ - lookAtZ;

    camRadius = sqrt(dx * dx + dy * dy + dz * dz);
    if (camRadius == 0) camRadius = 1.0f; // Prevenir divis�o por zero

    camBeta = asin(dy / camRadius);
    camAlpha = atan2(dx, dz);

    // For�ar o zoom din�mico no arranque
    if (!cameraTargets.empty()) {
        Target alvo = cameraTargets[0]; // O �ndice 0 � o Sol

        // Centrar no Sol
        lookAtX = alvo.x;
        lookAtY = alvo.y;
        lookAtZ = alvo.z;

        // Aplicar a nossa Regra de 3 para o Zoom
        camRadius = alvo.radius * 10.0f;
        if (camRadius < 0.5f) camRadius = 0.5f;

        updateCameraPos();
    }
}

void processSpecialKeys(int key, int xx, int yy) {
    // Controlos para aproximar (Zoom In) e afastar (Zoom Out)
    if (key == GLUT_KEY_UP) {
        camRadius -= camRadius * 0.1f; // Aproxima 10%
        if (camRadius < 0.5f) camRadius = 0.5f;
        updateCameraPos();
        glutPostRedisplay();
        return;
    }
    else if (key == GLUT_KEY_DOWN) {
        camRadius += camRadius * 0.1f; // Afasta 10%
        updateCameraPos();
        glutPostRedisplay();
        return;
    }

    if (cameraTargets.empty()) return;

    if (key == GLUT_KEY_RIGHT) {
        currentTargetIndex = (currentTargetIndex + 1) % cameraTargets.size();
    }
    else if (key == GLUT_KEY_LEFT) {
        currentTargetIndex = (currentTargetIndex - 1 + cameraTargets.size()) % cameraTargets.size();
    }
    else {
        return; // Ignora outras teclas especiais para não quebrar o foco atual
    }

    Target alvo = cameraTargets[currentTargetIndex];
    cout << "A focar em: " << alvo.name << " (Raio lido: " << alvo.radius << ")" << endl;

    // 1. O centro da nossa rota��o passa a ser o planeta!
    lookAtX = alvo.x;
    lookAtY = alvo.y;
    lookAtZ = alvo.z;

    // 2. A Magia da Proporcionalidade: 
    // A dist�ncia da c�mara ser� 10 vezes o tamanho do astro.
    camRadius = alvo.radius * 10.0f;

    // As luas de Marte s�o muito muito pequenas. Impomos uma dist�ncia m�nima absoluta 
    // para n�o entrarmos dentro da geometria do modelo:
    if (camRadius < 0.5f) camRadius = 0.5f;

    // 3. Atualizamos a c�mara para refletir esta nova �ncora e zoom
    updateCameraPos();
    glutPostRedisplay();
}

// Controlo da camara usando o teclado (WASD)
void processNormalKeys(unsigned char key, int x, int y) {
    switch (key) {
        case 'a': case 'A':
            camAlpha -= 0.05f;
            break;
        case 'd': case 'D':
            camAlpha += 0.05f;
            break;
        case 'w': case 'W':
            camBeta += 0.05f;
            if (camBeta > 1.5f) camBeta = 1.5f;
            break;
        case 's': case 'S':
            camBeta -= 0.05f;
            if (camBeta < -1.5f) camBeta = -1.5f;
            break;
    }
    updateCameraPos();
    glutPostRedisplay();
}

void processMouseButtons(int button, int state, int xx, int yy) {
    if (state == GLUT_DOWN) {
        startX = xx;
        startY = yy;
        if (button == GLUT_LEFT_BUTTON) {
            picking(xx, yy); // Efetuar color picking ao clicar
        }
        else if (button == GLUT_RIGHT_BUTTON) {
            tracking = 2; // Zoom
        }
        else if (button == 3) { // Scroll Up (Roda do rato para a frente - Zoom In)
            camRadius -= camRadius * 0.1f;
            if (camRadius < 0.5f) camRadius = 0.5f;
            updateCameraPos();
            glutPostRedisplay();
        }
        else if (button == 4) { // Scroll Down (Roda do rato para trás - Zoom Out)
            camRadius += camRadius * 0.1f;
            updateCameraPos();
            glutPostRedisplay();
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

    if (tracking == 2) { // Boto Direito: Zoom
        camRadius += deltaY * 0.1f;
        if (camRadius < 0.1f) camRadius = 0.1f;
    }

    startX = xx;
    startY = yy;

    updateCameraPos();
    glutPostRedisplay();
}

// ==========================================
// FUN��ES GLUT
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

    // --- 0. Atualizar posições reais do mundo dos astros (Passo 2, SEM Câmera) ---
    if (selectedID > 0) {
        glLoadIdentity(); // Limpa matrizes (aplica APENAS as orbitas para ficar em World Space)
        drawGroup(sceneRoot, 2);

        lookAtX = selectedX;
        lookAtY = selectedY;
        lookAtZ = selectedZ;
        updateCameraPos(); // Para que a câmara se mova suavemente sem feedback loop
    }

    glLoadIdentity();


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

    drawGroup(sceneRoot, 0); // Passo 0: Render normal

    // 3. Atualizar o lookAt da camara e o HUD
    if (selectedID > 0) {
        renderText("Astro Selecionado: " + selectedName);
    } else {
        renderText("HUD: Nenhum astro selecionado (Clique em algum)");
    }

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    // 1. Validar Argumentos
    if (argc < 2) {
        cout << "# => ERRO: Ficheiro XML nao fornecido!" << endl;
        cout << "# => Uso: engine <ficheiro.xml>" << endl;
        return 1;
    }


    // 2. Inicializar o GLUT e a Janela PRIMEIRO
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Motor 3D - Sistema Solar");

    GLenum err = glewInit();
    if (GLEW_OK != err) return 1;

    glEnableClientState(GL_VERTEX_ARRAY);

    // 3. Leitura dos dados de Configura��o (O XML)
    cout << "--- A INICIAR MOTOR 3D ---" << endl;
    loadConfig(argv[1]);

    // Atualiza o tamanho da janela caso o XML tenha alterado os defaults
    glutReshapeWindow(winW, winH);
    
    initCamera();

    // 4. Registar Callbacks (AGORA o GLUT j� tem uma janela para os associar)
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);

    // Callbacks do Rato
    glutMouseFunc(processMouseButtons);
    glutMotionFunc(processMouseMotion);
    glutKeyboardFunc(processNormalKeys); // Intercecao das teclas WASD

    // Callbacks do Teclado Especial (Setas, F1-F12)
    glutSpecialFunc(processSpecialKeys);

    // 5. Configura��es OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glutIdleFunc(renderScene);

    // 6. Arrancar ciclo principal
    glutMainLoop();

    return 0;
}