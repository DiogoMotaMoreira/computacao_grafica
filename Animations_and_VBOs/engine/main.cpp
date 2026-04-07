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
    float time; // para translações e reotações com tempo
    vector<float*> points; // Catmull-Rom
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
int currentTargetIndex = 0;        // Número do astro 

Group sceneRoot;

map<string, vector<float>> modelsData;

map<string, VBOModel> modelsVBO;

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
// LEITURA DOS FICHEIROS .3d
// ==========================================
void load3DFile(const string& filename) {

    // se o ficheiro ja foi lido não vamos carregar de novo
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

    VBOModel model;
    model.vertexCount = currentModelVertices.size() / 3;

    glGenBuffers(1, &model.bufferID);
    glBindBuffer(GL_ARRAY_BUFFER, model.bufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * currentModelVertices.size(), currentModelVertices.data(), GL_STATIC_DRAW);

    modelsVBO[filename] = model;
    cout << "i => VBO criado: " << filename << " (" << model.vertexCount << " vertices)" << endl;
}

// ==========================================
// parser recursivo -> mudança em primeiro if para nova fase 3
// ==========================================
Group parseGroup(XMLElement* groupElement) {
    Group node;

    // ler transformações
    XMLElement* transformElement = groupElement->FirstChildElement("transform"); // procura o primeiro filho transform na arvore
    if (transformElement) {
        // ler todos os elementos dentro do transform
        for (XMLElement* t = transformElement->FirstChildElement(); t != nullptr; t = t->NextSiblingElement()) {
            Transform trans;
            trans.type = t->Name();
            trans.time = t->FloatAttribute("time", 0.0f); // Lê o tempo se existir

            if (trans.type == "translate" && trans.time > 0) {
                // Lê pontos de controlo para Catmull-Rom
                for (XMLElement* p = t->FirstChildElement("point"); p != nullptr; p = p->NextSiblingElement("point")) {
                    float* pt = new float[3];
                    pt[0] = p->FloatAttribute("x");
                    pt[1] = p->FloatAttribute("y");
                    pt[2] = p->FloatAttribute("z");
                    trans.points.push_back(pt);
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
        // sceneRoot é a nossa var global do tipo group
        sceneRoot = parseGroup(mainGroup);
        cout << "i => Arvore de cena carregada com sucesso!" << endl;
    }
    else {
        cout << "i => AVISO: Nenhum <group> principal encontrado no XML" << endl;
    }
}

// ==========================================
// Funções auxiliares sugeridas nos slides -< fase 3 parte da matematica C-R
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
// Funções para Catmull
// ==========================================
void getCatmullRomPoint(float t, float* p0, float* p1, float* p2, float* p3, float* pos, float* deriv) {
    // matriz igual a guião
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

// integrar o tempo e a transformação
float prev_y[3] = { 0, 1, 0 };

void applyCatmullTransform(vector<float*> points, float time_duration) {
    // calcular o t global
    float elapsed_time = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float t_global = fmod(elapsed_time, time_duration) / time_duration;

    int num_points = points.size();
    float t = t_global * num_points;
    int index = floor(t);
    t = t - index; // t local entre 0 e 1

    // indices dos 4 pontos de controlo para curvas
    int indices[4];
    for (int i = 0; i < 4; ++i) indices[i] = (index + i - 1 + num_points) % num_points;
    float pos[3], deriv[3];
    getCatmullRomPoint(t, points[indices[0]], points[indices[1]], points[indices[2]], points[indices[3]], pos, deriv);

    // translação para o ponto da curva
    glTranslatef(pos[0], pos[1], pos[2]);

    // rotação que temos que fazer para alinhar a tangente
    float x[3], y[3], z[3];
    for (int i = 0; i < 3; i++) x[i] = deriv[i];
    normalize(x); // X = P'(t)

    cross(x, prev_y, z);
    normalize(z); // Z = X x Y_prev
    cross(z, x, y);
    normalize(y); // Y = Z x X

    for (int i = 0; i < 3; i++) prev_y[i] = y[i]; // Guardar para a proxima frame

    float m[16];
    buildRotMatrix(x, y, z, m);
    glMultMatrixf(m); // aplicar a matriz a rotação
}

// ==========================================
// Desenhar a linha para orbitas
// ==========================================
void renderCatmullRomCurve(const vector<float*>& points) {
    float pos[3], deriv[3];
    int num_points = points.size();

    glBegin(GL_LINE_LOOP);
    glColor3f(0.5f, 0.5f, 0.5f); // Cor cinzenta para a órbita

    // Nível de tesselação de 0.01 conforme sugerido (100 segmentos por ponto) 
    for (float gt = 0; gt < 1.0f; gt += 0.01f) {
        float t_global = gt * num_points;
        int index = floor(t_global);
        float t = t_global - index;

        int indices[4];
        for (int i = 0; i < 4; ++i)
            indices[i] = (index + i - 1 + num_points) % num_points;

        getCatmullRomPoint(t, points[indices[0]], points[indices[1]],
            points[indices[2]], points[indices[3]], pos, deriv);

        glVertex3f(pos[0], pos[1], pos[2]);
    }
    glEnd();
}


// ==========================================
// DESENHO RECURSIVO DA ÁRVORE - fase 3
// ==========================================
void drawGroup(const Group& group) {
    // guardar o estado atual do mundo 
    glPushMatrix();

    // aplicar as transformações deste grupo   -> fase 3 (mudança)
    for (const Transform& t : group.transforms) {
        if (t.type == "translate") {
            if (t.time > 0 && t.points.size() >= 4) {
                // Desenha a linha da órbita primeiro
                renderCatmullRomCurve(t.points);
                // Depois move o objeto para a sua posição atual na curva
                applyCatmullTransform(t.points, t.time);
            }
            else {
                glTranslatef(t.x, t.y, t.z);
            }
        }
        else if (t.type == "rotate") {
            if (t.time > 0) {
                // Rotação contínua baseada no tempo: (tempo_atual * 360) / tempo_total
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

            // 1. Indicar qual o VBO ativo [cite: 934, 945]
            glBindBuffer(GL_ARRAY_BUFFER, vbo.bufferID);

            // 2. Definir a semântica: 3 floats por vértice [cite: 945, 952]
            glVertexPointer(3, GL_FLOAT, 0, 0);

            // 3. Mandar desenhar tudo de uma vez [cite: 834, 946]
            glDrawArrays(GL_TRIANGLES, 0, vbo.vertexCount);
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

    // Forçar o zoom dinâmico no arranque
    if (!cameraTargets.empty()) {
        Target alvo = cameraTargets[0]; // O índice 0 é o Sol

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
    if (cameraTargets.empty()) return;

    if (key == GLUT_KEY_RIGHT) {
        currentTargetIndex = (currentTargetIndex + 1) % cameraTargets.size();
    }
    else if (key == GLUT_KEY_LEFT) {
        currentTargetIndex = (currentTargetIndex - 1 + cameraTargets.size()) % cameraTargets.size();
    }

    Target alvo = cameraTargets[currentTargetIndex];
    cout << "A focar em: " << alvo.name << " (Raio lido: " << alvo.radius << ")" << endl;

    // 1. O centro da nossa rotação passa a ser o planeta!
    lookAtX = alvo.x;
    lookAtY = alvo.y;
    lookAtZ = alvo.z;

    // 2. A Magia da Proporcionalidade: 
    // A distância da câmara será 10 vezes o tamanho do astro.
    camRadius = alvo.radius * 10.0f;

    // As luas de Marte são muito muito pequenas. Impomos uma distância mínima absoluta 
    // para não entrarmos dentro da geometria do modelo:
    if (camRadius < 0.5f) camRadius = 0.5f;

    // 3. Atualizamos a câmara para refletir esta nova âncora e zoom
    updateCameraPos();
    glutPostRedisplay();
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
        if (camRadius < 0.1f) camRadius = 0.1f;
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


    // 2. Inicializar o GLUT e a Janela PRIMEIRO
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(winW, winH);
    glutCreateWindow("Motor 3D - Sistema Solar");

    GLenum err = glewInit();
    if (GLEW_OK != err) return 1;

    glEnableClientState(GL_VERTEX_ARRAY);

    // 3. Leitura dos dados de Configuração (O XML)
    cout << "--- A INICIAR MOTOR 3D ---" << endl;
    loadConfig(argv[1]);

    // Atualiza o tamanho da janela caso o XML tenha alterado os defaults
    glutReshapeWindow(winW, winH);
    
    initCamera();

    // 4. Registar Callbacks (AGORA o GLUT já tem uma janela para os associar)
    glutDisplayFunc(renderScene);
    glutReshapeFunc(changeSize);

    // Callbacks do Rato
    glutMouseFunc(processMouseButtons);
    glutMotionFunc(processMouseMotion);

    // Callbacks do Teclado Especial (Setas, F1-F12)
    glutSpecialFunc(processSpecialKeys);

    // 5. Configurações OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glutIdleFunc(renderScene);

    // 6. Arrancar ciclo principal
    glutMainLoop();

    return 0;
}