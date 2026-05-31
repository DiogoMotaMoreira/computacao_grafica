#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

// ==========================================
// AUXILIARES MATEMATICOS
// ==========================================
float bezier(float t, float p0, float p1, float p2, float p3) {
	float it = 1.0f - t;
	return it*it*it*p0 + 3*t*it*it*p1 + 3*t*t*it*p2 + t*t*t*p3;
}

float bezierDeriv(float t, float p0, float p1, float p2, float p3) {
	float it = 1.0f - t;
	return 3*it*it*(p1-p0) + 6*it*t*(p2-p1) + 3*t*t*(p3-p2);
}

void cross(float* a, float* b, float* res) {
	res[0] = a[1]*b[2] - a[2]*b[1];
	res[1] = a[2]*b[0] - a[0]*b[2];
	res[2] = a[0]*b[1] - a[1]*b[0];
}

void normalize(float* a) {
	float l = sqrt(a[0]*a[0] + a[1]*a[1] + a[2]*a[2]);
	if (l != 0) { a[0]/=l; a[1]/=l; a[2]/=l; }
}

// ==========================================
// BEZIER (ja tinha normais e UVs)
// ==========================================
void getBezierPointAndNormal(float u, float v, float** patchPoints, float* pos, float* normal) {
	float tempU[4][3], tempV[4][3];
	float tangentU[3], tangentV[3];

	for (int i = 0; i < 4; i++)
		for (int k = 0; k < 3; k++)
			tempU[i][k] = bezier(u, patchPoints[i*4][k], patchPoints[i*4+1][k], patchPoints[i*4+2][k], patchPoints[i*4+3][k]);

	for (int i = 0; i < 4; i++)
		for (int k = 0; k < 3; k++)
			tempV[i][k] = bezier(v, patchPoints[i][k], patchPoints[i+4][k], patchPoints[i+8][k], patchPoints[i+12][k]);

	for (int k = 0; k < 3; k++) {
		pos[k]      = bezier(v,       tempU[0][k], tempU[1][k], tempU[2][k], tempU[3][k]);
		tangentU[k] = bezierDeriv(u,  tempV[0][k], tempV[1][k], tempV[2][k], tempV[3][k]);
		tangentV[k] = bezierDeriv(v,  tempU[0][k], tempU[1][k], tempU[2][k], tempU[3][k]);
	}
	cross(tangentV, tangentU, normal);
	normalize(normal);

	normal[0] = -normal[0];
	normal[1] = -normal[1];
	normal[2] = -normal[2];
}

void generateBezier(string patchFile, int tessellation, string outFile) {
	ifstream file(patchFile);
	if (!file.is_open()) { cout << "ERRO: Ficheiro " << patchFile << " nao encontrado!" << endl; return; }

	int numPatches;
	file >> numPatches;
	vector<vector<int>> patchIndices(numPatches, vector<int>(16));
	string line;
	getline(file, line);

	for (int i = 0; i < numPatches; i++) {
		getline(file, line);
		stringstream ss(line);
		for (int j = 0; j < 16; j++) {
			string val; getline(ss, val, ',');
			patchIndices[i][j] = stoi(val);
		}
	}

	int numControlPoints;
	file >> numControlPoints;
	vector<float*> controlPoints;
	for (int i = 0; i < numControlPoints; i++) {
		float* p = new float[3];
		file >> p[0]; file.ignore(1);
		file >> p[1]; file.ignore(1);
		file >> p[2];
		controlPoints.push_back(p);
	}

	vector<float> vertices;
	float step = 1.0f / tessellation;

	for (int p = 0; p < numPatches; p++) {
		float* currentPatch[16];
		for (int i = 0; i < 16; i++) currentPatch[i] = controlPoints[patchIndices[p][i]];

		for (int i = 0; i < tessellation; i++) {
			for (int j = 0; j < tessellation; j++) {
				float u1=i*step, u2=(i+1)*step, v1=j*step, v2=(j+1)*step;
				float p1[3],p2[3],p3[3],p4[3],n1[3],n2[3],n3[3],n4[3];
				getBezierPointAndNormal(u1,v1,currentPatch,p1,n1);
				getBezierPointAndNormal(u1,v2,currentPatch,p2,n2);
				getBezierPointAndNormal(u2,v1,currentPatch,p3,n3);
				getBezierPointAndNormal(u2,v2,currentPatch,p4,n4);
				// Triângulo 1 (P1-P3-P2) — Ordem invertida para CCW correto
				for (int k = 0; k < 3; k++) { vertices.push_back(p1[k]); } for (int k = 0; k < 3; k++) { vertices.push_back(n1[k]); } vertices.push_back(u1); vertices.push_back(v1);
				for (int k = 0; k < 3; k++) { vertices.push_back(p3[k]); } for (int k = 0; k < 3; k++) { vertices.push_back(n3[k]); } vertices.push_back(u2); vertices.push_back(v1);
				for (int k = 0; k < 3; k++) { vertices.push_back(p2[k]); } for (int k = 0; k < 3; k++) { vertices.push_back(n2[k]); } vertices.push_back(u1); vertices.push_back(v2);

				// Triângulo 2 (P2-P3-P4) — Ordem invertida para CCW correto
				for (int k = 0; k < 3; k++) { vertices.push_back(p2[k]); } for (int k = 0; k < 3; k++) { vertices.push_back(n2[k]); } vertices.push_back(u1); vertices.push_back(v2);
				for (int k = 0; k < 3; k++) { vertices.push_back(p3[k]); } for (int k = 0; k < 3; k++) { vertices.push_back(n3[k]); } vertices.push_back(u2); vertices.push_back(v1);
				for (int k = 0; k < 3; k++) { vertices.push_back(p4[k]); } for (int k = 0; k < 3; k++) { vertices.push_back(n4[k]); } vertices.push_back(u2); vertices.push_back(v2);
			}
		}
	}

	ofstream out(outFile);
	out << vertices.size()/8 << endl;
	for (int i = 0; i < (int)vertices.size(); i += 8)
		out << vertices[i] << " " << vertices[i+1] << " " << vertices[i+2] << " "
		    << vertices[i+3] << " " << vertices[i+4] << " " << vertices[i+5] << " "
		    << vertices[i+6] << " " << vertices[i+7] << "\n";
	cout << "i => Bezier gerado: " << vertices.size()/8 << " vertices em " << outFile << endl;
}

// ==========================================
// PLANE  — normal (0,1,0), UV proporcional
// ==========================================
void generatePlane(float size, int divisions, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) { cout << "# => Erro ao abrir ficheiro!" << endl; return; }

	int numVertices = divisions * divisions * 6;
	file << numVertices << "\n";

	float half = size / 2.0f;
	float step = size / (float)divisions;

	// normal fixa para cima
	const float nx=0.0f, ny=1.0f, nz=0.0f;

	for (int i = 0; i < divisions; ++i) {
		for (int j = 0; j < divisions; ++j) {
			float x1 = -half + j*step,  z1 = -half + i*step;
			float x2 = x1 + step,       z2 = z1 + step;

			// UVs normalizadas para [0,1]
			float u1=(x1+half)/size, u2=(x2+half)/size;
			float v1=(z1+half)/size, v2=(z2+half)/size;

			// Triangulo 1: (x1,z1) (x1,z2) (x2,z2)
			file << x1 << " 0 " << z1 << " " << nx << " " << ny << " " << nz << " " << u1 << " " << v1 << "\n";
			file << x1 << " 0 " << z2 << " " << nx << " " << ny << " " << nz << " " << u1 << " " << v2 << "\n";
			file << x2 << " 0 " << z2 << " " << nx << " " << ny << " " << nz << " " << u2 << " " << v2 << "\n";
			// Triangulo 2: (x1,z1) (x2,z2) (x2,z1)
			file << x1 << " 0 " << z1 << " " << nx << " " << ny << " " << nz << " " << u1 << " " << v1 << "\n";
			file << x2 << " 0 " << z2 << " " << nx << " " << ny << " " << nz << " " << u2 << " " << v2 << "\n";
			file << x2 << " 0 " << z1 << " " << nx << " " << ny << " " << nz << " " << u2 << " " << v1 << "\n";
		}
	}
	file.close();
	cout << "i => Plano gerado: " << numVertices << " vertices em " << filename << endl;
}

// ==========================================
// BOX — normais perpendiculares, UV por face
// ==========================================
// Escreve um vertice com posicao, normal e UV
static void writeVert(ofstream& f,
                      float x, float y, float z,
                      float nx, float ny, float nz,
                      float u, float v) {
	f << x <<" "<< y <<" "<< z <<" "<< nx <<" "<< ny <<" "<< nz <<" "<< u <<" "<< v <<"\n";
}

void generateBox(float dimension, int divisions, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) { cout << "# => Erro ao abrir ficheiro!" << endl; return; }

	int numVertices = 6 * divisions * divisions * 6;
	file << numVertices << "\n";

	float half = dimension / 2.0f;
	float step = dimension / (float)divisions;

	for (int i = 0; i < divisions; ++i) {
		for (int j = 0; j < divisions; ++j) {
			float c1 = -half + j*step;  float n1 = c1 + step;   // horizontal local
			float c2 = -half + i*step;  float n2 = c2 + step;   // vertical local

			// UVs locais normalizadas
			float ua = (c1+half)/dimension, ub = (n1+half)/dimension;
			float va = (c2+half)/dimension, vb = (n2+half)/dimension;

			// 1. FRENTE  Z=+half  normal=(0,0,1)
			writeVert(file, c1,c2, half,  0,0,1,  ua,va);
			writeVert(file, n1,c2, half,  0,0,1,  ub,va);
			writeVert(file, n1,n2, half,  0,0,1,  ub,vb);
			writeVert(file, c1,c2, half,  0,0,1,  ua,va);
			writeVert(file, n1,n2, half,  0,0,1,  ub,vb);
			writeVert(file, c1,n2, half,  0,0,1,  ua,vb);

			// 2. TRÁS    Z=-half  normal=(0,0,-1)
			writeVert(file, c1,c2,-half,  0,0,-1,  ub,va);
			writeVert(file, c1,n2,-half,  0,0,-1,  ub,vb);
			writeVert(file, n1,n2,-half,  0,0,-1,  ua,vb);
			writeVert(file, c1,c2,-half,  0,0,-1,  ub,va);
			writeVert(file, n1,n2,-half,  0,0,-1,  ua,vb);
			writeVert(file, n1,c2,-half,  0,0,-1,  ua,va);

			// 3. TOPO    Y=+half  normal=(0,1,0)  c1->X c2->Z
			writeVert(file, c1,half,c2,  0,1,0,  ua,va);
			writeVert(file, c1,half,n2,  0,1,0,  ua,vb);
			writeVert(file, n1,half,n2,  0,1,0,  ub,vb);
			writeVert(file, c1,half,c2,  0,1,0,  ua,va);
			writeVert(file, n1,half,n2,  0,1,0,  ub,vb);
			writeVert(file, n1,half,c2,  0,1,0,  ub,va);

			// 4. FUNDO   Y=-half  normal=(0,-1,0)
			writeVert(file, c1,-half,c2,  0,-1,0,  ua,va);
			writeVert(file, n1,-half,c2,  0,-1,0,  ub,va);
			writeVert(file, n1,-half,n2,  0,-1,0,  ub,vb);
			writeVert(file, c1,-half,c2,  0,-1,0,  ua,va);
			writeVert(file, n1,-half,n2,  0,-1,0,  ub,vb);
			writeVert(file, c1,-half,n2,  0,-1,0,  ua,vb);

			// 5. DIREITA  X=+half  normal=(1,0,0)  c1->Z c2->Y
			writeVert(file, half,c2,c1,  1,0,0,  ua,va);
			writeVert(file, half,n2,c1,  1,0,0,  ua,vb);
			writeVert(file, half,n2,n1,  1,0,0,  ub,vb);
			writeVert(file, half,c2,c1,  1,0,0,  ua,va);
			writeVert(file, half,n2,n1,  1,0,0,  ub,vb);
			writeVert(file, half,c2,n1,  1,0,0,  ub,va);

			// 6. ESQUERDA X=-half  normal=(-1,0,0)
			writeVert(file,-half,c2,c1,  -1,0,0,  ub,va);
			writeVert(file,-half,c2,n1,  -1,0,0,  ua,va);
			writeVert(file,-half,n2,n1,  -1,0,0,  ua,vb);
			writeVert(file,-half,c2,c1,  -1,0,0,  ub,va);
			writeVert(file,-half,n2,n1,  -1,0,0,  ua,vb);
			writeVert(file,-half,n2,c1,  -1,0,0,  ub,vb);
		}
	}
	file.close();
	cout << "i => Caixa gerada: " << numVertices << " vertices em " << filename << endl;
}

// ==========================================
// SPHERE — ja tinha normais/UVs (mantida)
// ==========================================
void generateSphere(float radius, int slices, int stacks, string filename) {
	ofstream file(filename);
	if (!file.is_open()) return;

	file << slices * stacks * 6 << "\n";

	float alphaStep = 2*M_PI / slices;
	float betaStep  = M_PI / stacks;

	for (int i = 0; i < stacks; i++) {
		float beta1 = -M_PI/2 + i*betaStep;
		float beta2 = -M_PI/2 + (i+1)*betaStep;
		for (int j = 0; j < slices; j++) {
			float alpha1 = j*alphaStep, alpha2 = (j+1)*alphaStep;
			float u1=(float)j/slices, u2=(float)(j+1)/slices;
			float v1=(float)i/stacks, v2=(float)(i+1)/stacks;

			float p[4][3] = {
				{radius*cos(beta1)*sin(alpha1), radius*sin(beta1), radius*cos(beta1)*cos(alpha1)},
				{radius*cos(beta1)*sin(alpha2), radius*sin(beta1), radius*cos(beta1)*cos(alpha2)},
				{radius*cos(beta2)*sin(alpha1), radius*sin(beta2), radius*cos(beta2)*cos(alpha1)},
				{radius*cos(beta2)*sin(alpha2), radius*sin(beta2), radius*cos(beta2)*cos(alpha2)}
			};
			float uv[4][2] = { {u1,v1},{u2,v1},{u1,v2},{u2,v2} };
			int idx[] = {0,1,2,1,3,2};
			for (int k : idx) {
				float x=p[k][0], y=p[k][1], z=p[k][2];
				file << x <<" "<< y <<" "<< z <<" "
				     << x/radius <<" "<< y/radius <<" "<< z/radius <<" "
				     << uv[k][0] <<" "<< uv[k][1] <<"\n";
			}
		}
	}
	file.close();
	cout << "i => Esfera gerada: " << slices*stacks*6 << " vertices em " << filename << endl;
}

// ==========================================
// CONE — normais laterais corretas, base (0,-1,0)
// Normal lateral em angulo theta:
//   n = normalize( h*sin(t), R, h*cos(t) )
// ==========================================
void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) { cout << "# => Error ao abrir ficheiro!" << endl; return; }

	int numVerticesBase    = slices * 3;
	int numVerticesLaterias = stacks * slices * 6;
	int totalVertices = numVerticesBase + numVerticesLaterias;
	file << totalVertices << '\n';

	float angleStep  = (2.0f * M_PI) / slices;
	float heightStep = height / stacks;

	// Comprimento da normal lateral (sem normalizar): (h*sin, R, h*cos)
	// Fator de normalizacao: sqrt(h^2 + R^2) — constante para todo o cone
	float nlen = sqrt(height*height + radius*radius);

	// BASE — normal (0,-1,0)
	for (int i = 0; i < slices; ++i) {
		float a1 = i*angleStep, a2 = (i+1)*angleStep;
		// centro
		file << "0.0 0.0 0.0  0 -1 0  0.5 0.5\n";
		// perimetro — UV circular
		float ux2=0.5f+0.5f*sin(a2), vx2=0.5f+0.5f*cos(a2);
		float ux1=0.5f+0.5f*sin(a1), vx1=0.5f+0.5f*cos(a1);
		file << radius*sin(a2) << " 0.0 " << radius*cos(a2) << "  0 -1 0  " << ux2 << " " << vx2 << "\n";
		file << radius*sin(a1) << " 0.0 " << radius*cos(a1) << "  0 -1 0  " << ux1 << " " << vx1 << "\n";
	}

	// SUPERFICIE LATERAL
	for (int i = 0; i < stacks; ++i) {
		float h1 = i*heightStep,     r1 = radius*(1.0f-(float)i/stacks);
		float h2 = (i+1)*heightStep, r2 = radius*(1.0f-(float)(i+1)/stacks);

		float v1 = (float)i/stacks, v2 = (float)(i+1)/stacks;

		for (int j = 0; j < slices; ++j) {
			float a1 = j*angleStep, a2 = (j+1)*angleStep;
			float u1 = (float)j/slices, u2 = (float)(j+1)/slices;

			float x1=r1*sin(a1), z1=r1*cos(a1);
			float x2=r1*sin(a2), z2=r1*cos(a2);
			float x3=r2*sin(a1), z3=r2*cos(a1);
			float x4=r2*sin(a2), z4=r2*cos(a2);

			// Normais laterais: n(angle) = normalize(h*sin(a), R, h*cos(a))
			float lnx1=height*sin(a1)/nlen, lnz1=height*cos(a1)/nlen, lny=radius/nlen;
			float lnx2=height*sin(a2)/nlen, lnz2=height*cos(a2)/nlen;

			file << x1 <<" "<< h1 <<" "<< z1 <<"  "<< lnx1 <<" "<< lny <<" "<< lnz1 <<"  "<< u1 <<" "<< v1 <<"\n";
			file << x2 <<" "<< h1 <<" "<< z2 <<"  "<< lnx2 <<" "<< lny <<" "<< lnz2 <<"  "<< u2 <<" "<< v1 <<"\n";
			file << x3 <<" "<< h2 <<" "<< z3 <<"  "<< lnx1 <<" "<< lny <<" "<< lnz1 <<"  "<< u1 <<" "<< v2 <<"\n";

			file << x2 <<" "<< h1 <<" "<< z2 <<"  "<< lnx2 <<" "<< lny <<" "<< lnz2 <<"  "<< u2 <<" "<< v1 <<"\n";
			file << x4 <<" "<< h2 <<" "<< z4 <<"  "<< lnx2 <<" "<< lny <<" "<< lnz2 <<"  "<< u2 <<" "<< v2 <<"\n";
			file << x3 <<" "<< h2 <<" "<< z3 <<"  "<< lnx1 <<" "<< lny <<" "<< lnz1 <<"  "<< u1 <<" "<< v2 <<"\n";
		}
	}
	file.close();
	cout << "i => Cone gerado: " << totalVertices << " vertices em " << filename << endl;
}

// ==========================================
// TORUS — normal aponta do centro do tubo para o vertice
// n(theta,phi) = (cos(phi)*cos(theta), sin(phi), cos(phi)*sin(theta))
// ==========================================
void generateTorus(float innerRadius, float outerRadius, int sides, int rings, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) { cout << "# => Erro ao abrir ficheiro!" << endl; return; }

	int numVertices = rings * sides * 6;
	file << numVertices << "\n";

	float ringStep = (2.0f*M_PI) / rings;
	float sideStep = (2.0f*M_PI) / sides;

	for (int i = 0; i < rings; ++i) {
		float theta1 = i*ringStep, theta2 = (i+1)*ringStep;
		float u1 = (float)i/rings, u2 = (float)(i+1)/rings;

		for (int j = 0; j < sides; ++j) {
			float phi1 = j*sideStep, phi2 = (j+1)*sideStep;
			float v1 = (float)j/sides, v2 = (float)(j+1)/sides;

			// 4 vertices do quad
			float x1=(outerRadius+innerRadius*cos(phi1))*cos(theta1), y1=innerRadius*sin(phi1), z1=(outerRadius+innerRadius*cos(phi1))*sin(theta1);
			float x2=(outerRadius+innerRadius*cos(phi1))*cos(theta2), y2=innerRadius*sin(phi1), z2=(outerRadius+innerRadius*cos(phi1))*sin(theta2);
			float x3=(outerRadius+innerRadius*cos(phi2))*cos(theta2), y3=innerRadius*sin(phi2), z3=(outerRadius+innerRadius*cos(phi2))*sin(theta2);
			float x4=(outerRadius+innerRadius*cos(phi2))*cos(theta1), y4=innerRadius*sin(phi2), z4=(outerRadius+innerRadius*cos(phi2))*sin(theta1);

			// Normais: n(theta,phi) = (cos(phi)*cos(theta), sin(phi), cos(phi)*sin(theta))
			float nx1=cos(phi1)*cos(theta1), ny1=sin(phi1), nz1=cos(phi1)*sin(theta1);
			float nx2=cos(phi1)*cos(theta2), ny2=sin(phi1), nz2=cos(phi1)*sin(theta2);
			float nx3=cos(phi2)*cos(theta2), ny3=sin(phi2), nz3=cos(phi2)*sin(theta2);
			float nx4=cos(phi2)*cos(theta1), ny4=sin(phi2), nz4=cos(phi2)*sin(theta1);

			// Triangulo 1 (v1,v2,v3)
			file << x1<<" "<<y1<<" "<<z1<<"  "<<nx1<<" "<<ny1<<" "<<nz1<<"  "<<u1<<" "<<v1<<"\n";
			file << x2<<" "<<y2<<" "<<z2<<"  "<<nx2<<" "<<ny2<<" "<<nz2<<"  "<<u2<<" "<<v1<<"\n";
			file << x3<<" "<<y3<<" "<<z3<<"  "<<nx3<<" "<<ny3<<" "<<nz3<<"  "<<u2<<" "<<v2<<"\n";
			// Triangulo 2 (v1,v3,v4)
			file << x1<<" "<<y1<<" "<<z1<<"  "<<nx1<<" "<<ny1<<" "<<nz1<<"  "<<u1<<" "<<v1<<"\n";
			file << x3<<" "<<y3<<" "<<z3<<"  "<<nx3<<" "<<ny3<<" "<<nz3<<"  "<<u2<<" "<<v2<<"\n";
			file << x4<<" "<<y4<<" "<<z4<<"  "<<nx4<<" "<<ny4<<" "<<nz4<<"  "<<u1<<" "<<v2<<"\n";
		}
	}
	file.close();
	cout << "i => Torus gerado: " << numVertices << " vertices em " << filename << endl;
}


// ==========================================
// ANNULUS — Disco plano com buraco (para anéis)
// ==========================================
void generateAnnulus(float innerRadius, float outerRadius, int segments, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) { cout << "# => Erro ao abrir ficheiro!" << endl; return; }

	int numVertices = segments * 6;
	file << numVertices << "\n";

	// Numero de repeticoes da textura a volta do anel
	float repeats = 32.0f;  // Ajusta este valor a gosto (8, 16, 32...)

	for (int i = 0; i < segments; ++i) {
		float angle1 = (2.0f * M_PI * i) / segments;
		float angle2 = (2.0f * M_PI * (i + 1)) / segments;

		// V = posicao angular (repete N vezes a volta)
		float v1 = (float)i / segments * repeats;
		float v2 = (float)(i + 1) / segments * repeats;

		// U = 0 no raio interno, 1 no raio externo
		float u_in = 0.0f;
		float u_out = 1.0f;

		float x1_in = innerRadius * cos(angle1), z1_in = innerRadius * sin(angle1);
		float x2_in = innerRadius * cos(angle2), z2_in = innerRadius * sin(angle2);
		float x1_out = outerRadius * cos(angle1), z1_out = outerRadius * sin(angle1);
		float x2_out = outerRadius * cos(angle2), z2_out = outerRadius * sin(angle2);

		// T1: inner1, inner2, outer1
		file << x1_in << " 0 " << z1_in << "  0 1 0  " << u_in << " " << v1 << "\n";
		file << x2_in << " 0 " << z2_in << "  0 1 0  " << u_in << " " << v2 << "\n";
		file << x1_out << " 0 " << z1_out << "  0 1 0  " << u_out << " " << v1 << "\n";

		// T2: inner2, outer2, outer1
		file << x2_in << " 0 " << z2_in << "  0 1 0  " << u_in << " " << v2 << "\n";
		file << x2_out << " 0 " << z2_out << "  0 1 0  " << u_out << " " << v2 << "\n";
		file << x1_out << " 0 " << z1_out << "  0 1 0  " << u_out << " " << v1 << "\n";
	}

	file.close();
	cout << "i => Annulus gerado: " << numVertices << " vertices em " << filename << endl;
}

// ==========================================
// ORBIT — linha, sem normais/UVs (nao e carregada pelo engine como modelo)
// ==========================================
void generateOrbit(float radius, int slices, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) { cout << "# => Erro ao abrir ficheiro!" << endl; return; }
	file << slices << "\n";
	float step = (2.0f*M_PI) / slices;
	for (int i = 0; i < slices; ++i) {
		float angle = i*step;
		// Orbitas sao renderizadas pelo engine como GL_LINE_LOOP
		// O engine usa o mesmo load3DFile e espera 8 valores — escrevemos normais/UV a zero
		file << radius*sin(angle) << " 0.0 " << radius*cos(angle) << "  0 1 0  0 0\n";
	}
	file.close();
	cout << "i => Orbita gerada: " << slices << " vertices em " << filename << endl;
}

// ==========================================
// MAIN
// ==========================================
int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "# => Uso: generator <forma> [params] <ficheiro.3d>" << endl;
		return 1;
	}

	string shape = argv[1];

	if (shape == "patch") {
		generateBezier(argv[2], atoi(argv[3]), argv[4]);
	} else if (shape == "plane") {
		if (argc != 5) { cout << "generator plane <tam> <div> <file.3d>" << endl; return 1; }
		generatePlane(stof(argv[2]), stoi(argv[3]), argv[4]);
	} else if (shape == "box") {
		if (argc != 5) { cout << "generator box <dim> <div> <file.3d>" << endl; return 1; }
		generateBox(stof(argv[2]), stoi(argv[3]), argv[4]);
	} else if (shape == "sphere") {
		if (argc != 6) { cout << "generator sphere <r> <sl> <st> <file.3d>" << endl; return 1; }
		generateSphere(stof(argv[2]), stoi(argv[3]), stoi(argv[4]), argv[5]);
	} else if (shape == "cone") {
		if (argc != 7) { cout << "generator cone <r> <h> <sl> <st> <file.3d>" << endl; return 1; }
		generateCone(stof(argv[2]), stof(argv[3]), stoi(argv[4]), stoi(argv[5]), argv[6]);
	} else if (shape == "torus") {
		if (argc != 7) { cout << "generator torus <rIn> <rOut> <sides> <rings> <file.3d>" << endl; return 1; }
		generateTorus(stof(argv[2]), stof(argv[3]), stoi(argv[4]), stoi(argv[5]), argv[6]);
	}
	else if (shape == "annulus") {
		if (argc != 6) { cout << "generator annulus <rIn> <rOut> <segments> <file.3d>" << endl; return 1; }
		generateAnnulus(stof(argv[2]), stof(argv[3]), stoi(argv[4]), argv[5]);
	} else if (shape == "orbit") {
		if (argc != 5) { cout << "generator orbit <r> <sl> <file.3d>" << endl; return 1; }
		generateOrbit(stof(argv[2]), stoi(argv[3]), argv[4]);
	} else {
		cout << "# => Forma desconhecida: " << shape << endl;
		return 1;
	}
	return 0;
}
