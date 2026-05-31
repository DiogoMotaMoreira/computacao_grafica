#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <GL/glew.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <math.h>
#include <algorithm>

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "tinyxml2.h"

#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY
#define GL_MAX_TEXTURE_MAX_ANISOTROPY 0x84FE
#endif
#ifndef GL_TEXTURE_MAX_ANISOTROPY
#define GL_TEXTURE_MAX_ANISOTROPY 0x84FD
#endif

void updateCameraVectors();

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
    vector<float*> points;
    mutable float prev_y[3]       = {0.0f, 1.0f, 0.0f};
    mutable GLuint orbitVBO        = 0;
    mutable int    orbitVertCount  = 0;
};

struct ModelInfo {
    string filename;
    string type;
    int id;
    string name;
    GLuint textureID = 0;
    float ambient[4]  = {0.2f, 0.2f, 0.2f, 1.0f};
    float diffuse[4]  = {0.8f, 0.8f, 0.8f, 1.0f};
    float specular[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float emission[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    float shininess   = 0.0f;
};

struct Group {
    vector<Transform> transforms;
    vector<ModelInfo> models;
    vector<Group> children;
};

struct Target {
    string name;
    float x, y, z, radius;
};

struct VBOModel {
    GLuint verticesID, normalsID, texCoordsID;
    int vertexCount;
};

// Luz lida do XML
struct LightInfo {
    string type;
    float posX, posY, posZ;
    float dirX, dirY, dirZ;
    float cutoff;
};

// ==========================================
// GLOBAIS
// ==========================================
Group             sceneRoot;
map<string, VBOModel> modelsVBO;
map<string, GLuint>   loadedTextures;
vector<LightInfo>     sceneLights;

int    globalModelID = 1;
int    selectedID    = 0;
float  selectedX = 0, selectedY = 0, selectedZ = 0;
string selectedName  = "";
map<int, string> idToName;

int   winW = 512, winH = 512;
float projFov = 60, projNear = 1, projFar = 4000;

// Posição da câmara
float camPosX = 0, camPosY = 30, camPosZ = 80;

// Vetores de orientação
float camYaw = -1.5708f;
float camPitch = -0.2f;

// Derivados (recalculados a cada frame)
float camDirX, camDirY, camDirZ;    // Vetor frente
float camRightX, camRightY, camRightZ;  // Vetor direita
float camUpX = 0, camUpY = 1, camUpZ = 0;

// Movimento
float moveSpeed = 0.03f;
bool  keyW = false, keyA = false, keyS = false, keyD = false;
bool  keyUp = false, keyDown = false;

// Rato
bool  mousePressed = false;
int   lastMouseX = 0, lastMouseY = 0;
float mouseSensitivity = 0.0015f;

float lookAtX = 0, lookAtY = 0, lookAtZ = 0;


bool keySpace = false, keyShift = false;

// ==========================================
// TEXTURAS (STB Image)
// ==========================================
GLuint loadTexture(string s) {
    if (loadedTextures.count(s)) return loadedTextures[s];

    stbi_set_flip_vertically_on_load(true);

    int w, h, channels;
    unsigned char* data = stbi_load(s.c_str(), &w, &h, &channels, 4);
    if (!data) {
        cout << "!!! ERRO: Textura nao encontrada: " << s << endl;
        return 0;
    }

    cout << "=> Textura " << s << " (" << w << "x" << h << ")" << endl;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    // === NOVO: Filtragem Agressiva com Mipmapping ===
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

    // Anisotropic filtering (melhora muito a qualidade em ângulos oblíquos)
    GLfloat maxAnisotropy;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, maxAnisotropy);
    // === FIM NOVO ===

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

    // === NOVO: Gera os mipmaps ===
    glGenerateMipmap(GL_TEXTURE_2D);
    // === FIM NOVO ===

    stbi_image_free(data);
    loadedTextures[s] = texID;
    return texID;
}

// ==========================================
// LEITURA DE FICHEIROS .3d
// ==========================================
void load3DFile(const string& filename) {
    if (modelsVBO.count(filename) > 0) return;
    ifstream file(filename);
    if (!file.is_open()) { cout << "# => ERRO: " << filename << endl; return; }
    int numVertices;
    if (!(file >> numVertices)) return;

    vector<float> p, n, t;
    float vx,vy,vz,nx,ny,nz,tx,ty;
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*p.size(), p.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &model.normalsID);
    glBindBuffer(GL_ARRAY_BUFFER, model.normalsID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*n.size(), n.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &model.texCoordsID);
    glBindBuffer(GL_ARRAY_BUFFER, model.texCoordsID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float)*t.size(), t.data(), GL_STATIC_DRAW);
    modelsVBO[filename] = model;
    cout << "i => VBO: " << filename << " (" << numVertices << " v)" << endl;
}

// ==========================================
// PARSER DO GRAFO DE CENA
// ==========================================
Group parseGroup(XMLElement* groupElement) {
    Group node;

    XMLElement* transformElement = groupElement->FirstChildElement("transform");
    if (transformElement) {
        for (XMLElement* t = transformElement->FirstChildElement(); t; t = t->NextSiblingElement()) {
            Transform trans;
            trans.type = t->Name();
            trans.time = t->FloatAttribute("time", 0.0f);

            if (trans.type == "translate") {
                if (trans.time > 0) {
                    const char* a = t->Attribute("align");
                    trans.align = (a && (string(a)=="True"||string(a)=="true"||string(a)=="1"));
                    for (XMLElement* p = t->FirstChildElement("point"); p; p = p->NextSiblingElement("point")) {
                        float* pt = new float[3];
                        pt[0]=p->FloatAttribute("x"); pt[1]=p->FloatAttribute("y"); pt[2]=p->FloatAttribute("z");
                        trans.points.push_back(pt);
                    }
                } else {
                    XMLElement* p = t->FirstChildElement("point");
                    if (p) { trans.x=p->FloatAttribute("x"); trans.y=p->FloatAttribute("y"); trans.z=p->FloatAttribute("z"); }
                    else   { trans.x=t->FloatAttribute("x"); trans.y=t->FloatAttribute("y"); trans.z=t->FloatAttribute("z"); }
                }
            } else if (trans.type == "rotate") {
                trans.angle = t->FloatAttribute("angle", 0.0f);
                trans.x=t->FloatAttribute("x"); trans.y=t->FloatAttribute("y"); trans.z=t->FloatAttribute("z");
            } else {
                trans.x=t->FloatAttribute("x"); trans.y=t->FloatAttribute("y"); trans.z=t->FloatAttribute("z");
            }
            node.transforms.push_back(trans);
        }
    }

    XMLElement* modelsElement = groupElement->FirstChildElement("models");
    if (modelsElement) {
        for (XMLElement* m = modelsElement->FirstChildElement("model"); m; m = m->NextSiblingElement("model")) {
            ModelInfo info;
            info.filename = m->Attribute("file") ? m->Attribute("file") : "";
            info.type     = m->Attribute("type") ? m->Attribute("type") : "solid";
            info.name     = m->Attribute("name") ? m->Attribute("name") : info.filename;
            info.id = globalModelID++;
            idToName[info.id] = info.name;

            XMLElement* tex = m->FirstChildElement("texture");
            if (tex && tex->Attribute("file")) info.textureID = loadTexture(tex->Attribute("file"));

            XMLElement* color = m->FirstChildElement("color");
            if (color) {
                auto readRGB = [](XMLElement* e, float* arr) {
                    if (!e) return;
                    arr[0]=e->FloatAttribute("R")/255.0f;
                    arr[1]=e->FloatAttribute("G")/255.0f;
                    arr[2]=e->FloatAttribute("B")/255.0f;
                    arr[3]=1.0f;
                };
                readRGB(color->FirstChildElement("ambient"),  info.ambient);
                readRGB(color->FirstChildElement("diffuse"),  info.diffuse);
                readRGB(color->FirstChildElement("specular"), info.specular);
                readRGB(color->FirstChildElement("emissive"), info.emission);
                XMLElement* sh = color->FirstChildElement("shininess");
                if (sh) info.shininess = sh->FloatAttribute("value");
            }
            node.models.push_back(info);
            load3DFile(info.filename);
        }
    }

    for (XMLElement* c = groupElement->FirstChildElement("group"); c; c = c->NextSiblingElement("group"))
        node.children.push_back(parseGroup(c));

    return node;
}

// ==========================================
// LEITURA DO XML (com luzes)
// ==========================================
void loadConfig(const char* xmlFilename) {
    XMLDocument doc;
    if (doc.LoadFile(xmlFilename) != XML_SUCCESS) { cout << "ERRO XML: " << xmlFilename << endl; exit(1); }
    XMLElement* world = doc.FirstChildElement("world");
    if (!world) exit(1);

    XMLElement* window = world->FirstChildElement("window");
    if (window) { window->QueryIntAttribute("width",&winW); window->QueryIntAttribute("height",&winH); }

    XMLElement* camera = world->FirstChildElement("camera");
    if (camera) {
        XMLElement* pos  = camera->FirstChildElement("position");
        if (pos)  { pos->QueryFloatAttribute("x",&camPosX); pos->QueryFloatAttribute("y",&camPosY); pos->QueryFloatAttribute("z",&camPosZ); }
        XMLElement* proj = camera->FirstChildElement("projection");
        if (proj) { proj->QueryFloatAttribute("fov",&projFov); proj->QueryFloatAttribute("near",&projNear); proj->QueryFloatAttribute("far",&projFar); }
        XMLElement* look = camera->FirstChildElement("lookAt");
        if (look) { look->QueryFloatAttribute("x", &lookAtX); look->QueryFloatAttribute("y", &lookAtY); look->QueryFloatAttribute("z", &lookAtZ); }
    }

    // Parser das luzes
    XMLElement* lightsElem = world->FirstChildElement("lights");
    if (lightsElem) {
        int lightIdx = 0;
        for (XMLElement* l = lightsElem->FirstChildElement("light");
             l && lightIdx < 8;
             l = l->NextSiblingElement("light"), ++lightIdx) {

            LightInfo li;
            const char* typeAttr = l->Attribute("type");
            li.type = typeAttr ? string(typeAttr) : "point";
            transform(li.type.begin(), li.type.end(), li.type.begin(), ::tolower);

            li.posX = l->FloatAttribute("posX", l->FloatAttribute("posx", 0.0f));
            li.posY = l->FloatAttribute("posY", l->FloatAttribute("posy", 0.0f));
            li.posZ = l->FloatAttribute("posZ", l->FloatAttribute("posz", 0.0f));
            li.dirX = l->FloatAttribute("dirX", l->FloatAttribute("dirx", 0.0f));
            li.dirY = l->FloatAttribute("dirY", l->FloatAttribute("diry", 0.0f));
            li.dirZ = l->FloatAttribute("dirZ", l->FloatAttribute("dirz", 0.0f));
            li.cutoff = l->FloatAttribute("cutoff", 45.0f);

            sceneLights.push_back(li);
            cout << "i => Luz " << lightIdx << ": " << li.type << endl;
        }
    } else {
        LightInfo def;
        def.type = "point"; def.posX=def.posY=def.posZ=0; def.cutoff=180;
        sceneLights.push_back(def);
    }

    XMLElement* mainGroup = world->FirstChildElement("group");
    if (mainGroup) sceneRoot = parseGroup(mainGroup);
}

// ==========================================
// APLICAR LUZES
// ==========================================
void applyLights() {
    float white[4]  = {1.0f, 1.0f, 1.0f, 1.0f};
    float dark[4]   = {0.1f, 0.1f, 0.1f, 1.0f};

    for (int i = 0; i < (int)sceneLights.size() && i < 8; ++i) {
        GLenum lightID = GL_LIGHT0 + i;
        glEnable(lightID);
        glLightfv(lightID, GL_DIFFUSE,  white);
        glLightfv(lightID, GL_SPECULAR, white);
        glLightfv(lightID, GL_AMBIENT,  dark);

        const LightInfo& li = sceneLights[i];

        if (li.type == "directional") {
            float dir[4] = {li.dirX, li.dirY, li.dirZ, 0.0f};
            glLightfv(lightID, GL_POSITION, dir);
        } else if (li.type == "spot") {
            float pos[4] = {li.posX, li.posY, li.posZ, 1.0f};
            float dir[3] = {li.dirX, li.dirY, li.dirZ};
            glLightfv(lightID, GL_POSITION,        pos);
            glLightfv(lightID, GL_SPOT_DIRECTION,  dir);
            glLightf (lightID, GL_SPOT_CUTOFF,     li.cutoff);
            glLightf (lightID, GL_SPOT_EXPONENT,   2.0f);
        } else {
            float pos[4] = {li.posX, li.posY, li.posZ, 1.0f};
            glLightfv(lightID, GL_POSITION, pos);
        }
    }
}

// ==========================================
// MATEMATICA CATMULL-ROM
// ==========================================
void cross(float* a, float* b, float* res) {
    res[0]=a[1]*b[2]-a[2]*b[1]; res[1]=a[2]*b[0]-a[0]*b[2]; res[2]=a[0]*b[1]-a[1]*b[0];
}
void normalize(float* a) {
    float l=sqrt(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    if(l!=0){a[0]/=l;a[1]/=l;a[2]/=l;}
}
void buildRotMatrix(float* x, float* y, float* z, float* m) {
    m[0]=x[0];m[1]=x[1];m[2]=x[2];m[3]=0;
    m[4]=y[0];m[5]=y[1];m[6]=y[2];m[7]=0;
    m[8]=z[0];m[9]=z[1];m[10]=z[2];m[11]=0;
    m[12]=0;m[13]=0;m[14]=0;m[15]=1;
}

void getCatmullRomPoint(float t, float* p0, float* p1, float* p2, float* p3, float* pos, float* deriv) {
    float m[4][4]={{-0.5f,1.5f,-1.5f,0.5f},{1.0f,-2.5f,2.0f,-0.5f},{-0.5f,0.0f,0.5f,0.0f},{0.0f,1.0f,0.0f,0.0f}};
    for (int i=0;i<3;++i) {
        float p[4]={p0[i],p1[i],p2[i],p3[i]}, a[4];
        for(int j=0;j<4;++j) a[j]=m[j][0]*p[0]+m[j][1]*p[1]+m[j][2]*p[2]+m[j][3]*p[3];
        pos[i]  =pow(t,3)*a[0]+pow(t,2)*a[1]+t*a[2]+a[3];
        deriv[i]=3*pow(t,2)*a[0]+2*t*a[1]+a[2];
    }
}

void applyCatmullTransform(vector<float*> points, float time_duration, bool doAlign, float* prev_y) {
    float elapsed = glutGet(GLUT_ELAPSED_TIME)/1000.0f;
    float tg = fmod(elapsed, time_duration)/time_duration;
    int n = points.size(), index = (int)floor(tg*n);
    float t = tg*n - index;
    int ids[4]; for(int i=0;i<4;++i) ids[i]=(index+i-1+n)%n;
    float pos[3], deriv[3];
    getCatmullRomPoint(t, points[ids[0]], points[ids[1]], points[ids[2]], points[ids[3]], pos, deriv);
    glTranslatef(pos[0],pos[1],pos[2]);
    if (doAlign) {
        float x[3],y[3],z[3];
        for(int i=0;i<3;i++) x[i]=deriv[i];
        normalize(x); cross(x,prev_y,z); normalize(z); cross(z,x,y); normalize(y);
        for(int i=0;i<3;i++) prev_y[i]=y[i];
        float m[16]; buildRotMatrix(x,y,z,m); glMultMatrixf(m);
    }
}

void renderCatmullRomCurve(const vector<float*>& points, GLuint& vboID, int& vertCount) {
    if (vboID == 0) {
        vector<float> verts;
        float pos[3], deriv[3];
        int n = points.size();
        for (float gt=0.0f; gt<1.0f; gt+=0.001f) {
            float tg=gt*n; int idx=(int)floor(tg); float t=tg-idx;
            int ids[4]; for(int i=0;i<4;++i) ids[i]=(idx+i-1+n)%n;
            getCatmullRomPoint(t, points[ids[0]], points[ids[1]], points[ids[2]], points[ids[3]], pos, deriv);
            verts.push_back(pos[0]); verts.push_back(pos[1]); verts.push_back(pos[2]);
        }
        vertCount = (int)(verts.size()/3);
        glGenBuffers(1,&vboID);
        glBindBuffer(GL_ARRAY_BUFFER,vboID);
        glBufferData(GL_ARRAY_BUFFER, sizeof(float)*verts.size(), verts.data(), GL_STATIC_DRAW);
    }
    glColor3f(0.5f,0.5f,0.5f);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDrawArrays(GL_LINE_LOOP, 0, vertCount);
    glEnableClientState(GL_NORMAL_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}

// ==========================================
// DESENHO RECURSIVO
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
            if (t.time > 0) glRotatef((glutGet(GLUT_ELAPSED_TIME)/1000.0f*360.0f)/t.time, t.x, t.y, t.z);
            else            glRotatef(t.angle, t.x, t.y, t.z);
        } else if (t.type == "scale") glScalef(t.x, t.y, t.z);
    }

    for (const ModelInfo& mod : group.models) {
        if (!modelsVBO.count(mod.filename)) continue;
        VBOModel vbo = modelsVBO[mod.filename];

        if (renderMode == 2) {
            if (mod.id == selectedID) {
                float m[16]; glGetFloatv(GL_MODELVIEW_MATRIX, m);
                selectedX=m[12]; selectedY=m[13]; selectedZ=m[14];
            }
        } else if (renderMode == 1) {
            glColor3ub(mod.id, 0, 0);
        } else {
            glMaterialfv(GL_FRONT, GL_AMBIENT,   mod.ambient);
            glMaterialfv(GL_FRONT, GL_DIFFUSE,   mod.diffuse);
            glMaterialfv(GL_FRONT, GL_SPECULAR,  mod.specular);
            glMaterialfv(GL_FRONT, GL_EMISSION,  mod.emission);
            glMaterialf (GL_FRONT, GL_SHININESS, mod.shininess);
            if (mod.textureID) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, mod.textureID); }
            else                  glDisable(GL_TEXTURE_2D);
        }

        if (renderMode != 2) {
            glBindBuffer(GL_ARRAY_BUFFER, vbo.verticesID);  glVertexPointer(3, GL_FLOAT, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo.normalsID);   glNormalPointer(GL_FLOAT, 0, 0);
            glBindBuffer(GL_ARRAY_BUFFER, vbo.texCoordsID); glTexCoordPointer(2, GL_FLOAT, 0, 0);
            if (mod.type == "line") {
                glDrawArrays(GL_LINE_LOOP, 0, vbo.vertexCount);
            }
            else if (mod.type == "ring") {
                // Anéis: desativa culling para ver das duas faces
                glDisable(GL_CULL_FACE);
                glDrawArrays(GL_TRIANGLES, 0, vbo.vertexCount);
                glEnable(GL_CULL_FACE);
            }
            else {
                glDrawArrays(GL_TRIANGLES, 0, vbo.vertexCount);
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
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp); gluOrtho2D(0, vp[2], 0, vp[3]);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_DEPTH_TEST); glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glColor3f(1,1,1);
    glRasterPos2d(10, vp[3]-25);
    for (char c : text) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    glEnable(GL_LIGHTING); glEnable(GL_DEPTH_TEST);
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

void picking(int x, int y) {
    glPushAttrib(GL_ENABLE_BIT | GL_POLYGON_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPushMatrix(); glLoadIdentity();
    updateCameraVectors();
    float lx = camPosX + camDirX;
    float ly = camPosY + camDirY;
    float lz = camPosZ + camDirZ;
    gluLookAt(camPosX, camPosY, camPosZ, lx, ly, lz, camUpX, camUpY, camUpZ);
    drawGroup(sceneRoot, 1);
    glPopMatrix();
    GLint vp[4]; glGetIntegerv(GL_VIEWPORT, vp);
    unsigned char res[4];
    glReadPixels(x, vp[3] - y - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, res);
    int pickedID = res[0];
    if (pickedID > 0 && idToName.count(pickedID)) {
        selectedID = pickedID;
        selectedName = idToName[pickedID];
    }
    else {
        selectedID = 0;
        selectedName = "";
    }
    glPopAttrib();
}

// ==========================================
// CAMARA
// ==========================================
void updateCameraVectors() {
    // Recalcula direção a partir de yaw e pitch
    camDirX = cos(camPitch) * cos(camYaw);
    camDirY = sin(camPitch);
    camDirZ = cos(camPitch) * sin(camYaw);

    // Right = Dir x WorldUp
    float wx = 0, wy = 1, wz = 0;
    camRightX = camDirY * wz - camDirZ * wy;
    camRightY = camDirZ * wx - camDirX * wz;
    camRightZ = camDirX * wy - camDirY * wx;
    // Normaliza Right
    float rlen = sqrt(camRightX * camRightX + camRightY * camRightY + camRightZ * camRightZ);
    if (rlen > 0) { camRightX /= rlen; camRightY /= rlen; camRightZ /= rlen; }

    // Up = Right x Dir
    camUpX = camRightY * camDirZ - camRightZ * camDirY;
    camUpY = camRightZ * camDirX - camRightX * camDirZ;
    camUpZ = camRightX * camDirY - camRightY * camDirX;
}

void applyMovement() {
    updateCameraVectors();
    if (keyW) { camPosX += camDirX * moveSpeed; camPosY += camDirY * moveSpeed; camPosZ += camDirZ * moveSpeed; }
    if (keyS) { camPosX -= camDirX * moveSpeed; camPosY -= camDirY * moveSpeed; camPosZ -= camDirZ * moveSpeed; }
    if (keyA) { camPosX -= camRightX * moveSpeed; camPosY -= camRightY * moveSpeed; camPosZ -= camRightZ * moveSpeed; }
    if (keyD) { camPosX += camRightX * moveSpeed; camPosY += camRightY * moveSpeed; camPosZ += camRightZ * moveSpeed; }
    if (keySpace) camPosY += moveSpeed;
    if (keyShift) camPosY -= moveSpeed;
}


void initCamera() {
    float dx = lookAtX - camPosX;
    float dy = lookAtY - camPosY;
    float dz = lookAtZ - camPosZ;
    float length = sqrt(dx * dx + dy * dy + dz * dz);

    if (length > 0.0001f) {
        camPitch = asin(dy / length);
        camYaw = atan2(dz, dx);
    }
    updateCameraVectors();
}

void processNormalKeys(unsigned char key, int x, int y) {
    if (glutGetModifiers() & GLUT_ACTIVE_CTRL) { keyW = keyA = keyS = keyD = false; return; }
    switch (tolower(key)) {
    case 'w': keyW = true; break;
    case 's': keyS = true; break;
    case 'a': keyA = true; break;
    case 'd': keyD = true; break;
    case ' ': keySpace = true; break;
    case 'c': keyShift = true; break;
    case 27:  exit(0); break;
    }
}

void processNormalKeysUp(unsigned char key, int x, int y) {
    switch (tolower(key)) {
    case 'w': keyW = false; break;
    case 's': keyS = false; break;
    case 'a': keyA = false; break;
    case 'd': keyD = false; break;
    case ' ': keySpace = false; break;
    case 'c': keyShift = false; break;
    }
}

void processSpecialKeys(int key, int xx, int yy) {
    if (glutGetModifiers() & GLUT_ACTIVE_SHIFT) keyShift = true;
}

void processSpecialKeysUp(int key, int xx, int yy) {
    keyShift = false;
}

void processMouseButtons(int button, int state, int xx, int yy) {
    // Botão ESQUERDO — rotação da câmara
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { mousePressed = true;  lastMouseX = xx; lastMouseY = yy; }
        else                      mousePressed = false;
    }
    // Botão DIREITO — picking/foco
    if (button == GLUT_RIGHT_BUTTON && state == GLUT_UP) {
        picking(xx, yy);
    }
    // Scroll
    if (button == 3) { camPosX += camDirX * 3; camPosY += camDirY * 3; camPosZ += camDirZ * 3; glutPostRedisplay(); }
    if (button == 4) { camPosX -= camDirX * 3; camPosY -= camDirY * 3; camPosZ -= camDirZ * 3; glutPostRedisplay(); }
}

void processMouseMotion(int xx, int yy) {
    if (!mousePressed) return;
    int dx = xx - lastMouseX;
    int dy = yy - lastMouseY;
    camYaw += dx * mouseSensitivity;
    camPitch -= dy * mouseSensitivity;
    // Clamp pitch
    if (camPitch > 1.5f) camPitch = 1.5f;
    if (camPitch < -1.5f) camPitch = -1.5f;
    lastMouseX = xx;
    lastMouseY = yy;
    glutPostRedisplay();
}


// ==========================================
// GLUTcamYaw = atan2(dz, dx);
// ==========================================
void changeSize(int w, int h) {
    if(h==0) h=1;
    glViewport(0,0,w,h);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(projFov,(float)w/h,projNear,projFar);
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(void) {
    applyMovement();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    updateCameraVectors();
    float lookX = camPosX + camDirX;
    float lookY = camPosY + camDirY;
    float lookZ = camPosZ + camDirZ;

    glLoadIdentity();
    gluLookAt(camPosX, camPosY, camPosZ,
        lookX, lookY, lookZ,
        camUpX, camUpY, camUpZ);

    applyLights();

    // Eixos
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(-100, 0, 0); glVertex3f(100, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, -100, 0); glVertex3f(0, 100, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, -100); glVertex3f(0, 0, 100);
    glEnd();
    glEnable(GL_LIGHTING);

    drawGroup(sceneRoot, 0);

    glutSwapBuffers();
    glutPostRedisplay();
}

int main(int argc, char** argv) {
    if (argc < 2) { cout << "Uso: engine <ficheiro.xml>" << endl; return 1; }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA | GLUT_MULTISAMPLE);
    glutInitWindowPosition(100,100); glutInitWindowSize(winW, winH);
    glutCreateWindow("Motor 3D - G-Engine Fase 4 (STB)");

    glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        cout << "# => ERRO FATAL DO GLEW: " << glewGetErrorString(err) << endl;
        system("pause");
        return 1;
    }

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
    glutKeyboardUpFunc(processNormalKeysUp);
    glutSpecialFunc(processSpecialKeys);
    glutSpecialUpFunc(processSpecialKeysUp);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_MULTISAMPLE);

    float globalAmb[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmb);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glutIdleFunc(renderScene);
    glutMainLoop();
    return 0;
}
