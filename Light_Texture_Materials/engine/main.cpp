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
#include <IL/il.h>

using namespace std;
using namespace tinyxml2;

// ==========================================
// ESTRUTURAS DE DADOS
// ==========================================

struct Transform {
    string type;
    float x, y, z;
    float angle;
    float time; 
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
    GLuint textureID = 0;
    float ambient[4] = {0.2f, 0.2f, 0.2f, 1.0f};
    float diffuse[4] = {0.8f, 0.8f, 0.8f, 1.0f};
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float emission[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float shininess = 0.0f;
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
    GLuint verticesID; 
    GLuint normalsID; 
    GLuint texCoordsID;
    int vertexCount;   
};

std::vector<Target> cameraTargets;
int currentTargetIndex = 0;        

Group sceneRoot;

map<string, VBOModel> modelsVBO;
map<string, GLuint> loadedTextures;

// Variaveis para Color Picking
int globalModelID = 1; 
int selectedID = 0;
float selectedX = 0.0f, selectedY = 0.0f, selectedZ = 0.0f;
string selectedName = "";
map<int, string> idToName;

// ==========================================
// VARIÁVEIS GLOBAIS
// ==========================================
int winW = 512, winH = 512;

float camPosX = 10.0f, camPosY = 10.0f, camPosZ = 10.0f;
float lookAtX = 0.0f, lookAtY = 0.0f, lookAtZ = 0.0f;
float upX = 0.0f, upY = 1.0f, upZ = 0.0f;
float projFov = 60.0f, projNear = 1.0f, projFar = 1000.0f;

float camAlpha = 0.0f, camBeta = 0.5f, camRadius = 50.0f;
int startX, startY, tracking = 0;

// ==========================================
// CARREGAMENTO DE TEXTURAS (DevIL)
// ==========================================
GLuint loadTexture(string s) {
    if (loadedTextures.count(s)) return loadedTextures[s];

    unsigned int t, tw, th;
    unsigned char *texData;
    unsigned int texID;

    ilGenImages(1, &t);
    ilBindImage(t);
    if (!ilLoadImage((ILstring)s.c_str())) {
        cout << "!!! ERRO: Nao encontrei a imagem em: " << s << endl;
        return 0;
    }
    tw = ilGetInteger(IL_IMAGE_WIDTH);
    th = ilGetInteger(IL_IMAGE_HEIGHT);
    cout << "=> Sucesso: Textura " << s << " carregada (" << tw << "x" << th << ")" << endl;
    ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);
    texData = ilGetData();

    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tw, th, 0, GL_RGBA, GL_UNSIGNED_BYTE, texData);

    loadedTextures[s] = texID;
    return texID;
}

// ==========================================
// LEITURA DOS FICHEIROS .3d
// ==========================================
void load3DFile(const string& filename) {
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

    vector<float> p, n, t;
    float vx, vy, vz, nx, ny, nz, tx, ty;

    for (int i = 0; i < numVertices; ++i) {
        file >> vx >> vy >> vz >> nx >> ny >> nz >> tx >> ty;
        p.push_back(vx); p.push_back(vy); p.push_back(vz);
        n.push_back(nx); n.push_back(ny); n.push_back(nz);
        t.push_back(tx); t.push_back(ty);
    }

    VBOModel model;
    model.vertexCount = numVertices;

    glGenBuffers(1, &model.verticesID);
    glBindBuffer(GL_ARRAY_BUFFER, model.verticesID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * p.size(), p.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &model.normalsID);
    glBindBuffer(GL_ARRAY_BUFFER, model.normalsID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * n.size(), n.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &model.texCoordsID);
    glBindBuffer(GL_ARRAY_BUFFER, model.texCoordsID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * t.size(), t.data(), GL_STATIC_DRAW);

    modelsVBO[filename] = model;
    cout << "i => VBO criado: " << filename << " (" << numVertices << " vertices)" << endl;
}

// ==========================================
// parser recursivo
// ==========================================
Group parseGroup(XMLElement* groupElement) {
    Group node;

    XMLElement* transformElement = groupElement->FirstChildElement("transform");
    if (transformElement) {
        for (XMLElement* t = transformElement->FirstChildElement(); t != nullptr; t = t->NextSiblingElement()) {
            Transform trans;
            trans.type = t->Name();
            trans.time = t->FloatAttribute("time", 0.0f);

            if (trans.type == "translate") {
                if (trans.time > 0) {
                    const char* alignAttr = t->Attribute("align");
                    trans.align = (alignAttr != nullptr && (string(alignAttr) == "True" || string(alignAttr) == "true" || string(alignAttr) == "1"));
                    for (XMLElement* p = t->FirstChildElement("point"); p != nullptr; p = p->NextSiblingElement("point")) {
                        float* pt = new float[3];
                        pt[0] = p->FloatAttribute("x");
                        pt[1] = p->FloatAttribute("y");
                        pt[2] = p->FloatAttribute("z");
                        trans.points.push_back(pt);
                    }
                } else {
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
            } else if (trans.type == "rotate") {
                trans.angle = t->FloatAttribute("angle", 0.0f);
                trans.x = t->FloatAttribute("x", 0.0f);
                trans.y = t->FloatAttribute("y", 0.0f);
                trans.z = t->FloatAttribute("z", 0.0f);
            } else {
                trans.x = t->FloatAttribute("x", 0.0f);
                trans.y = t->FloatAttribute("y", 0.0f);
                trans.z = t->FloatAttribute("z", 0.0f);
            }
            node.transforms.push_back(trans);
        }
    }

    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        for (XMLElement* m = modelsElement->FirstChildElement("model"); m != nullptr; m = m->NextSiblingElement("model")) {
            ModelInfo info;
            info.filename = m->Attribute("file") ? m->Attribute("file") : "";
            info.type = m->Attribute("type") ? m->Attribute("type") : "solid";
            info.name = m->Attribute("name") ? m->Attribute("name") : info.filename;
            info.id = globalModelID++;
            idToName[info.id] = info.name;

            XMLElement* tex = m->FirstChildElement("texture");
            if (tex) info.textureID = loadTexture(tex->Attribute("file"));

            XMLElement* color = m->FirstChildElement("color");
            if (color) {
                XMLElement* child = color->FirstChildElement("ambient");
                if (child) { info.ambient[0]=child->FloatAttribute("R")/255.0f; info.ambient[1]=child->FloatAttribute("G")/255.0f; info.ambient[2]=child->FloatAttribute("B")/255.0f; }
                child = color->FirstChildElement("diffuse");
                if (child) { info.diffuse[0]=child->FloatAttribute("R")/255.0f; info.diffuse[1]=child->FloatAttribute("G")/255.0f; info.diffuse[2]=child->FloatAttribute("B")/255.0f; }
                child = color->FirstChildElement("specular");
                if (child) { info.specular[0]=child->FloatAttribute("R")/255.0f; info.specular[1]=child->FloatAttribute("G")/255.0f; info.specular[2]=child->FloatAttribute("B")/255.0f; }
                child = color->FirstChildElement("emissive");
                if (child) { info.emission[0]=child->FloatAttribute("R")/255.0f; info.emission[1]=child->FloatAttribute("G")/255.0f; info.emission[2]=child->FloatAttribute("B")/255.0f; }
                child = color->FirstChildElement("shininess");
                if (child) info.shininess = child->FloatAttribute("value");
            }

            node.models.push_back(info);
            load3DFile(info.filename);
        }
    }

    for (XMLElement* childGroup = groupElement->FirstChildElement("group"); childGroup != nullptr ; childGroup = childGroup->NextSiblingElement("group")) {
        node.children.push_back(parseGroup(childGroup));
    }

    return node;
}

// ==========================================
// LEITURA DO XML (TinyXML-2)
// ==========================================
void loadConfig(const char* xmlFilename) {
    XMLDocument doc;
    if (doc.LoadFile(xmlFilename) != XML_SUCCESS) exit(1);
    
    XMLElement* world = doc.FirstChildElement("world");
    if (!world) exit(1);

    XMLElement* window = world->FirstChildElement("window");
    if (window) {
        window->QueryIntAttribute("width", &winW);
        window->QueryIntAttribute("height", &winH);
    }

    XMLElement* camera = world->FirstChildElement("camera");
    if (camera) {
        XMLElement* pos = camera->FirstChildElement("position");
        if (pos) { pos->QueryFloatAttribute("x", &camPosX); pos->QueryFloatAttribute("y", &camPosY); pos->QueryFloatAttribute("z", &camPosZ); }
        
        XMLElement* look = camera->FirstChildElement("lookAt");
        if (look) { look->QueryFloatAttribute("x", &lookAtX); look->QueryFloatAttribute("y", &lookAtY); look->QueryFloatAttribute("z", &lookAtZ); }
        
        XMLElement* up = camera->FirstChildElement("up");
        if (up) { up->QueryFloatAttribute("x", &upX); up->QueryFloatAttribute("y", &upY); up->QueryFloatAttribute("z", &upZ); }
        
        XMLElement* proj = camera->FirstChildElement("projection");
        if (proj) { proj->QueryFloatAttribute("fov", &projFov); proj->QueryFloatAttribute("near", &projNear); proj->QueryFloatAttribute("far", &projFar); }
        
        XMLElement* waypointsElem = camera->FirstChildElement("waypoints");
        if (waypointsElem) {
            for (XMLElement* target = waypointsElem->FirstChildElement("target"); target != nullptr; target = target->NextSiblingElement("target")) {
                Target t;
                t.name = target->Attribute("name");
                t.x = target->FloatAttribute("x");
                t.y = target->FloatAttribute("y");
                t.z = target->FloatAttribute("z");
                t.radius = 1.0f;
                target->QueryFloatAttribute("radius", &t.radius);
                cameraTargets.push_back(t);
            }
        }
    }

    XMLElement* mainGroup = world->FirstChildElement("group");
    if (mainGroup) sceneRoot = parseGroup(mainGroup);
}

// ==========================================
// Funções auxiliares matemática
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
    float m[4][4] = { {-0.5f, 1.5f, -1.5f, 0.5f}, {1.0f, -2.5f, 2.0f, -0.5f}, {-0.5f, 0.0f, 0.5f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f} };
    for (int i = 0; i < 3; ++i) {
        float p[4] = { p0[i], p1[i], p2[i], p3[i] };
        float a[4];
        for (int j = 0; j < 4; ++j) {
            a[j] = m[j][0] * p[0] + m[j][1] * p[1] + m[j][2] * p[2] + m[j][3] * p[3];
        }
        pos[i] = pow(t, 3) * a[0] + pow(t, 2) * a[1] + t * a[2] + a[3];
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

    if (doAlign) {
        float x[3], y[3], z[3];
        for (int i = 0; i < 3; i++) x[i] = deriv[i];
        normalize(x);
        cross(x, prev_y, z);
        normalize(z);
        cross(z, x, y);
        normalize(y);
        for (int i = 0; i < 3; i++) prev_y[i] = y[i]; 
        float m[16]; buildRotMatrix(x, y, z, m); glMultMatrixf(m);
    }
}

void renderCatmullRomCurve(const vector<float*>& points, GLuint& vboID, int& vertCount) {
    if (vboID == 0) {
        vector<float> verts;
        float pos[3], deriv[3];
        int n = points.size();
        for (float gt = 0.0f; gt < 1.0f; gt += 0.001f) {
            float tg = gt * n;
            int idx = (int)floor(tg);
            float t = tg - idx;
            int ids[4];
            for (int i = 0; i < 4; ++i) ids[i] = (idx + i - 1 + n) % n;
            getCatmullRomPoint(t, points[ids[0]], points[ids[1]], points[ids[2]], points[ids[3]], pos, deriv);
            verts.push_back(pos[0]); verts.push_back(pos[1]); verts.push_back(pos[2]);
        }
        vertCount = (int)(verts.size() / 3);
        glGenBuffers(1, &vboID);
        glBindBuffer(GL_ARRAY_BUFFER, vboID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float) * verts.size(), verts.data(), GL_STATIC_DRAW);
    }
    glColor3f(0.5f, 0.5f, 0.5f);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_LINE_LOOP, 0, vertCount);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

// ==========================================
// DESENHO RECURSIVO DA ÁRVORE
// ==========================================
void drawGroup(const Group& group, int renderMode = 0) {
    glPushMatrix();

    for (const Transform& t : group.transforms) {
        if (t.type == "translate") {
            if (t.time > 0 && t.points.size() >= 4) {
                if (renderMode == 0) {
                    glDisable(GL_LIGHTING);
                    renderCatmullRomCurve(t.points, t.orbitVBO, t.orbitVertCount);
                    glEnable(GL_LIGHTING);
                }
                applyCatmullTransform(t.points, t.time, t.align, t.prev_y);
            } else glTranslatef(t.x, t.y, t.z);
        } else if (t.type == "rotate") {
            if (t.time > 0) glRotatef((glutGet(GLUT_ELAPSED_TIME) / 1000.0f * 360.0f) / t.time, t.x, t.y, t.z);
            else glRotatef(t.angle, t.x, t.y, t.z);
        } else if (t.type == "scale") glScalef(t.x, t.y, t.z);
    }

    for (const ModelInfo& mod : group.models) {
        if (modelsVBO.count(mod.filename) > 0) {
            VBOModel vbo = modelsVBO[mod.filename];
            if (renderMode == 2) {
                if (mod.id == selectedID) {
                    float m[16]; glGetFloatv(GL_MODELVIEW_MATRIX, m);
                    selectedX = m[12]; selectedY = m[13]; selectedZ = m[14];
                }
            } else if (renderMode == 1) glColor3ub(mod.id, 0, 0);
            else {
                glMaterialfv(GL_FRONT, GL_AMBIENT, mod.ambient);
                glMaterialfv(GL_FRONT, GL_DIFFUSE, mod.diffuse);
                glMaterialfv(GL_FRONT, GL_SPECULAR, mod.specular);
                glMaterialfv(GL_FRONT, GL_EMISSION, mod.emission);
                glMaterialf(GL_FRONT, GL_SHININESS, mod.shininess);
                if (mod.textureID) {
                    glEnable(GL_TEXTURE_2D);
                    glBindTexture(GL_TEXTURE_2D, mod.textureID);
                } else {
                    glDisable(GL_TEXTURE_2D);
                }
            }

            if (renderMode != 2) {
                glBindBuffer(GL_ARRAY_BUFFER, vbo.verticesID);
                glVertexPointer(3, GL_FLOAT, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, vbo.normalsID);
                glNormalPointer(GL_FLOAT, 0, 0);
                glBindBuffer(GL_ARRAY_BUFFER, vbo.texCoordsID);
                glTexCoordPointer(2, GL_FLOAT, 0, 0);

                if (mod.type == "line") glDrawArrays(GL_LINE_LOOP, 0, vbo.vertexCount);
                else glDrawArrays(GL_TRIANGLES, 0, vbo.vertexCount);
            }
        }
    }

    for (const Group& child : group.children) drawGroup(child, renderMode);
    glPopMatrix();
}

// ==========================================
// HUD E COLOR PICKING
// ==========================================
void renderText(string text) {
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport); gluOrtho2D(0, viewport[2], 0, viewport[3]);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2d(10, viewport[3] - 25);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

void picking(int x, int y) {
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 
    
    glPushMatrix();
    glLoadIdentity();
    gluLookAt(camPosX, camPosY, camPosZ, lookAtX, lookAtY, lookAtZ, upX, upY, upZ);
    drawGroup(sceneRoot, 1); 
    glPopMatrix();

    GLint viewport[4]; glGetIntegerv(GL_VIEWPORT, viewport); 
    unsigned char res[4];
    glReadPixels(x, viewport[3] - y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, res);

    int pickedID = res[0];
    if (pickedID > 0 && idToName.count(pickedID) > 0) {
        selectedID = pickedID;
        selectedName = idToName[pickedID];
    } else {
        for (auto const& pair : idToName) {
            if (pair.second == "Sol") {
                selectedID = pair.first;
                selectedName = pair.second;
                break;
            }
        }
    }
    glPopAttrib();
}

// ==========================================
// CONTROLO DA CÂMARA
// ==========================================
void updateCameraPos() {
    camPosX = lookAtX + camRadius * cos(camBeta) * sin(camAlpha);
    camPosY = lookAtY + camRadius * sin(camBeta);
    camPosZ = lookAtZ + camRadius * cos(camBeta) * cos(camAlpha);
}

void initCamera() {
    float dx = camPosX - lookAtX, dy = camPosY - lookAtY, dz = camPosZ - lookAtZ;
    camRadius = sqrt(dx * dx + dy * dy + dz * dz);
    if (camRadius == 0) camRadius = 1.0f;
    camBeta = asin(dy / camRadius); camAlpha = atan2(dx, dz);
    
    if (!cameraTargets.empty()) {
        Target alvo = cameraTargets[0]; 
        lookAtX = alvo.x; lookAtY = alvo.y; lookAtZ = alvo.z;
        camRadius = alvo.radius * 10.0f; if (camRadius < 0.5f) camRadius = 0.5f;
        updateCameraPos();
    }
}

void processSpecialKeys(int key, int xx, int yy) {
    if (key == GLUT_KEY_UP) { camRadius -= camRadius * 0.1f; if (camRadius < 0.5f) camRadius = 0.5f; updateCameraPos(); glutPostRedisplay(); return; }
    if (key == GLUT_KEY_DOWN) { camRadius += camRadius * 0.1f; updateCameraPos(); glutPostRedisplay(); return; }
    if (cameraTargets.empty()) return;

    if (key == GLUT_KEY_RIGHT) currentTargetIndex = (currentTargetIndex + 1) % cameraTargets.size();
    else if (key == GLUT_KEY_LEFT) currentTargetIndex = (currentTargetIndex - 1 + cameraTargets.size()) % cameraTargets.size();
    else return;

    Target alvo = cameraTargets[currentTargetIndex];
    lookAtX = alvo.x; lookAtY = alvo.y; lookAtZ = alvo.z; camRadius = alvo.radius * 10.0f; if (camRadius < 0.5f) camRadius = 0.5f;
    updateCameraPos(); glutPostRedisplay();
}

void processNormalKeys(unsigned char key, int x, int y) {
    switch (key) {
        case 'a': case 'A': camAlpha -= 0.05f; break;
        case 'd': case 'D': camAlpha += 0.05f; break;
        case 'w': case 'W': camBeta += 0.05f; if (camBeta > 1.5f) camBeta = 1.5f; break;
        case 's': case 'S': camBeta -= 0.05f; if (camBeta < -1.5f) camBeta = -1.5f; break;
    }
    updateCameraPos(); glutPostRedisplay();
}

void processMouseButtons(int button, int state, int xx, int yy) {
    if (state == GLUT_DOWN) {
        startX = xx; startY = yy;
        if (button == GLUT_LEFT_BUTTON) picking(xx, yy);
        else if (button == GLUT_RIGHT_BUTTON) tracking = 2;
        else if (button == 3) { camRadius -= camRadius * 0.1f; if (camRadius < 0.5f) camRadius = 0.5f; updateCameraPos(); glutPostRedisplay(); }
        else if (button == 4) { camRadius += camRadius * 0.1f; updateCameraPos(); glutPostRedisplay(); }
    } else if (state == GLUT_UP) tracking = 0;
}

void processMouseMotion(int xx, int yy) {
    if (!tracking) return;
    if (tracking == 2) { camRadius += (yy - startY) * 0.1f; if (camRadius < 0.1f) camRadius = 0.1f; }
    startX = xx; startY = yy; updateCameraPos(); glutPostRedisplay();
}

// ==========================================
// FUNÇÕES GLUT
// ==========================================
void changeSize(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(projFov, (float)w / h, projNear, projFar);
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (selectedID > 0) { glLoadIdentity(); drawGroup(sceneRoot, 2); lookAtX = selectedX; lookAtY = selectedY; lookAtZ = selectedZ; updateCameraPos(); }
    glLoadIdentity();
    gluLookAt(camPosX, camPosY, camPosZ, lookAtX, lookAtY, lookAtZ, upX, upY, upZ);

    float pos[4] = {0.0f, 0.0f, 0.0f, 1.0f}; glLightfv(GL_LIGHT0, GL_POSITION, pos);
    
    glDisable(GL_LIGHTING); glBegin(GL_LINES);
    glColor3f(1,0,0); glVertex3f(-100,0,0); glVertex3f(100,0,0);
    glColor3f(0,1,0); glVertex3f(0,-100,0); glVertex3f(0,100,0);
    glColor3f(0,0,1); glVertex3f(0,0,-100); glVertex3f(0,0,100); glEnd();
    
    glEnable(GL_LIGHTING); 
    drawGroup(sceneRoot, 0);

    if (selectedID > 0) renderText("Astro Selecionado: " + selectedName);
    else renderText("HUD: Nenhum astro selecionado (Clique em algum)");

    glutSwapBuffers();
}

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowPosition(100, 100); glutInitWindowSize(winW, winH);
    glutCreateWindow("Motor 3D - Sistema Solar");

    if (glewInit() != GLEW_OK) return 1;
    ilInit();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    loadConfig(argv[1]);
    glutReshapeWindow(winW, winH); 
    initCamera();

    glutDisplayFunc(renderScene); 
    glutReshapeFunc(changeSize);
    glutMouseFunc(processMouseButtons); 
    glutMotionFunc(processMouseMotion);
    glutKeyboardFunc(processNormalKeys); 
    glutSpecialFunc(processSpecialKeys);

    glEnable(GL_DEPTH_TEST); 
    glEnable(GL_CULL_FACE); 
    glEnable(GL_LIGHTING); 
    glEnable(GL_LIGHT0); 
    glEnable(GL_RESCALE_NORMAL);

    float dark[4] = {0.2f, 0.2f, 0.2f, 1.0f}, white[4] = {1.0f, 1.0f, 1.0f, 1.0f}, amb[4] = {1,1,1,1};
    glLightfv(GL_LIGHT0, GL_AMBIENT, dark); 
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white); 
    glLightfv(GL_LIGHT0, GL_SPECULAR, white);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, amb);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glutIdleFunc(renderScene);
    glutMainLoop();

    return 0;
}
