#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;

// usamos a formula de grau 3 de bernstein
// calculamos o peso de cada um dos 16 pontos de controlo
float bezier(float t, float p0, float p1, float p2, float p3) {
	float it = 1.0f - t;
	return it * it * it * p0 +
		3 * t * it * it * p1 +
		3 * t * t * it * p2 +
		t * t * t * p3;
}

float bezierDeriv(float t, float p0, float p1, float p2, float p3) {
	float it = 1.0f - t;
	return 3 * it * it * (p1 - p0) +
		   6 * it * t * (p2 - p1) +
		   3 * t * t * (p3 - p2);
}

void cross(float* a, float* b, float* res) {
	res[0] = a[1] * b[2] - a[2] * b[1];
	res[1] = a[2] * b[0] - a[0] * b[2];
	res[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize(float* a) {
	float l = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	if (l != 0) { a[0] /= l; a[1] /= l; a[2] /= l; }
}

void getBezierPointAndNormal(float u, float v, float** patchPoints, float* pos, float* normal) {
	float tempU[4][3], tempV[4][3];
	float tangentU[3], tangentV[3];

	for (int i = 0; i < 4; i++) {
		for (int k = 0; k < 3; k++) {
			tempU[i][k] = bezier(u, patchPoints[i * 4][k], patchPoints[i * 4 + 1][k], patchPoints[i * 4 + 2][k], patchPoints[i * 4 + 3][k]);
		}
	}

	for (int i = 0; i < 4; i++) {
		for (int k = 0; k < 3; k++) {
			tempV[i][k] = bezier(v, patchPoints[i][k], patchPoints[i + 4][k], patchPoints[i + 8][k], patchPoints[i + 12][k]);
		}
	}

	for (int k = 0; k < 3; k++) {
		pos[k] = bezier(v, tempU[0][k], tempU[1][k], tempU[2][k], tempU[3][k]);
		tangentU[k] = bezierDeriv(u, tempV[0][k], tempV[1][k], tempV[2][k], tempV[3][k]);
		tangentV[k] = bezierDeriv(v, tempU[0][k], tempU[1][k], tempU[2][k], tempU[3][k]);
	}

	cross(tangentU, tangentV, normal);
	normalize(normal);
}

void generateBezier(string patchFile, int tessellation, string outFile) {
	ifstream file(patchFile);
	if (!file.is_open()) {
		cout << "ERRO: Ficheiro " << patchFile << " nao encontrado!" << endl;
		return;
	}

	// indices
	int numPatches;
	file >> numPatches;
	vector<vector<int>> patchIndices(numPatches, vector<int>(16));
	string line;
	getline(file, line); // Limpar buffer

	for (int i = 0; i < numPatches; i++) {
		getline(file, line);
		stringstream ss(line);
		for (int j = 0; j < 16; j++) {
			string val;
			getline(ss, val, ',');
			patchIndices[i][j] = stoi(val);
		}
	}

	// pontos de controlo
	int numControlPoints;
	file >> numControlPoints;
	vector<float*> controlPoints;
	for (int i = 0; i < numControlPoints; i++) {
		float* p = new float[3];
		file >> p[0]; file.ignore(1); // ignora a virgula
		file >> p[1]; file.ignore(1);
		file >> p[2];
		controlPoints.push_back(p);
	}

	// vertices
	vector<float> vertices;
	float step = 1.0f / tessellation;

	for (int p = 0; p < numPatches; p++) {
		float* currentPatch[16];
		for (int i = 0; i < 16; i++) currentPatch[i] = controlPoints[patchIndices[p][i]];

		for (int i = 0; i < tessellation; i++) {
			for (int j = 0; j < tessellation; j++) {
				float u1 = i * step, u2 = (i + 1) * step;
				float v1 = j * step, v2 = (j + 1) * step;

				float p1[3], p2[3], p3[3], p4[3];
				float n1[3], n2[3], n3[3], n4[3];

				getBezierPointAndNormal(u1, v1, currentPatch, p1, n1);
				getBezierPointAndNormal(u1, v2, currentPatch, p2, n2);
				getBezierPointAndNormal(u2, v1, currentPatch, p3, n3);
				getBezierPointAndNormal(u2, v2, currentPatch, p4, n4);

				// Triângulo 1
				vertices.push_back(p1[0]); vertices.push_back(p1[1]); vertices.push_back(p1[2]);
				vertices.push_back(n1[0]); vertices.push_back(n1[1]); vertices.push_back(n1[2]);
				vertices.push_back(u1); vertices.push_back(v1);

				vertices.push_back(p3[0]); vertices.push_back(p3[1]); vertices.push_back(p3[2]);
				vertices.push_back(n3[0]); vertices.push_back(n3[1]); vertices.push_back(n3[2]);
				vertices.push_back(u2); vertices.push_back(v1);

				vertices.push_back(p2[0]); vertices.push_back(p2[1]); vertices.push_back(p2[2]);
				vertices.push_back(n2[0]); vertices.push_back(n2[1]); vertices.push_back(n2[2]);
				vertices.push_back(u1); vertices.push_back(v2);

				// Triângulo 2
				vertices.push_back(p2[0]); vertices.push_back(p2[1]); vertices.push_back(p2[2]);
				vertices.push_back(n2[0]); vertices.push_back(n2[1]); vertices.push_back(n2[2]);
				vertices.push_back(u1); vertices.push_back(v2);

				vertices.push_back(p3[0]); vertices.push_back(p3[1]); vertices.push_back(p3[2]);
				vertices.push_back(n3[0]); vertices.push_back(n3[1]); vertices.push_back(n3[2]);
				vertices.push_back(u2); vertices.push_back(v1);

				vertices.push_back(p4[0]); vertices.push_back(p4[1]); vertices.push_back(p4[2]);
				vertices.push_back(n4[0]); vertices.push_back(n4[1]); vertices.push_back(n4[2]);
				vertices.push_back(u2); vertices.push_back(v2);
			}
		}
	}

	// save file
	ofstream out(outFile);
	out << vertices.size() / 8 << endl;
	for (int i = 0; i < vertices.size(); i += 8) {
		out << vertices[i] << " " << vertices[i + 1] << " " << vertices[i + 2] << " "
		    << vertices[i + 3] << " " << vertices[i + 4] << " " << vertices[i + 5] << " "
		    << vertices[i + 6] << " " << vertices[i + 7] << endl;
	}
}


void generateBox(float dimension, int divisions, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cout << "# => Erro ao abrir ficheiro!" << endl;
		return;
	}

	// 6 faces * (divisoes x divisoes) quadrados * 6 vértices por pquadrado
	// para perceber as divisions pensar em cubo magico sendo que se fosse uma division, seria umm cubo normal
	int numVertices = 6 * divisions * divisions * 6;
	file << numVertices << "\n";

	float half = dimension / 2.0f;
	float step = dimension / (float) divisions;

	for (int i = 0; i < divisions; ++i) {
		for (int j = 0; j < divisions; ++j) {
			//variaveis para percorrer a grelha de qualquer face
			float c1 = -half + (j * step); //cord 1
			float c2 = -half + (i * step); //cord 2
			float n1 = c1 + step; //prox cord 1
			float n2 = c2 + step; //prox cord 2


			// c1 -> equerda | n1 -> direita 
			// c2 -> baixo   | n2 -> cima

			// ==========================================
			// 1. FACE FRENTE (Z fixo em +half)
			// c1, n1 mapeiam no X | c2, n2 mapeiam no Y
			// ==========================================
			file << c1 << " " << c2 << " " << half << "\n";
			file << n1 << " " << c2 << " " << half << "\n";
			file << n1 << " " << n2 << " " << half << "\n";

			file << c1 << " " << c2 << " " << half << "\n";
			file << n1 << " " << n2 << " " << half << "\n";
			file << c1 << " " << n2 << " " << half << "\n";

			// ==========================================
			// 2. FACE TRÁS (Z fixo em -half)
			// Ordem invertida para os triângulos virarem para "fora"
			// ==========================================
			file << c1 << " " << c2 << " " << -half << "\n";
			file << c1 << " " << n2 << " " << -half << "\n";
			file << n1 << " " << n2 << " " << -half << "\n";

			file << c1 << " " << c2 << " " << -half << "\n";
			file << n1 << " " << n2 << " " << -half << "\n";
			file << n1 << " " << c2 << " " << -half << "\n";

			// ==========================================
			// 3. FACE TOPO (Y fixo em +half)
			// c1, n1 mapeiam no X | c2, n2 mapeiam no Z
			// ==========================================
			file << c1 << " " << half << " " << c2 << "\n";
			file << c1 << " " << half << " " << n2 << "\n";
			file << n1 << " " << half << " " << n2 << "\n";

			file << c1 << " " << half << " " << c2 << "\n";
			file << n1 << " " << half << " " << n2 << "\n";
			file << n1 << " " << half << " " << c2 << "\n";

			// ==========================================
			// 4. FACE FUNDO (Y fixo em -half)
			// ==========================================
			file << c1 << " " << -half << " " << c2 << "\n";
			file << n1 << " " << -half << " " << c2 << "\n";
			file << n1 << " " << -half << " " << n2 << "\n";

			file << c1 << " " << -half << " " << c2 << "\n";
			file << n1 << " " << -half << " " << n2 << "\n";
			file << c1 << " " << -half << " " << n2 << "\n";

			// ==========================================
			// 5. FACE DIREITA (X fixo em +half)
			// c1, n1 mapeiam no Z | c2, n2 mapeiam no Y
			// ==========================================
			file << half << " " << c2 << " " << c1 << "\n";
			file << half << " " << n2 << " " << c1 << "\n";
			file << half << " " << n2 << " " << n1 << "\n";

			file << half << " " << c2 << " " << c1 << "\n";
			file << half << " " << n2 << " " << n1 << "\n";
			file << half << " " << c2 << " " << n1 << "\n";

			// ==========================================
			// 6. FACE ESQUERDA (X fixo em -half)
			// ==========================================
			file << -half << " " << c2 << " " << c1 << "\n";
			file << -half << " " << c2 << " " << n1 << "\n";
			file << -half << " " << n2 << " " << n1 << "\n";

			file << -half << " " << c2 << " " << c1 << "\n";
			file << -half << " " << n2 << " " << n1 << "\n";
			file << -half << " " << n2 << " " << c1 << "\n";
		}
	}

	file.close();
	cout << "i => Caixa gerada com sucesso! " << numVertices << " vertices gravados em: " << filename << endl;
}

void generatePlane(float size, int divisions, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cout << "# => Erro ao abrir ficheiro!" << endl;
		return;
	}

	//numVertices de uma face é o numero de divisions dessa face, logo divisionsdivisions, vezes 6 (3 vertices por triangulo)
	int numVertices = divisions * divisions * 6;
	file << numVertices << "\n";

	float half = size / 2.0f;
	float step = size / (float) divisions;

	for (int i = 0; i < divisions; ++i) {
		for (int j = 0; j < divisions; ++j) {

			// ==========================================
			// 1. Face Plano (Y fixo em 0)
			// z1, z2 mapeiam no Z | x1, x2 mapeiam no X
			// ==========================================
			float x1 = -half + (j * step);
			float z1 = -half + (i * step);
			float x2 = x1 + step;
			float z2 = z1 + step;

			// Plano XZ 
			// Triângulo 1 
			file << x1 << " 0 " << z1 << "\n";
			file << x1 << " 0 " << z2 << "\n";
			file << x2 << " 0 " << z2 << "\n";

			// Triângulo 2 
			file << x1 << " 0 " << z1 << "\n";
			file << x2 << " 0 " << z2 << "\n";
			file << x2 << " 0 " << z1 << "\n";
		}
	}

	file.close();
	cout << "i => Plano gerado: " << numVertices << " vertices em " << filename << endl;
}

void generateSphere(float radius, int slices, int stacks, string filename) {
	ofstream file(filename);
	if (!file.is_open()) return;

	file << slices * stacks * 6 << "\n";

	float alphaStep = 2 * M_PI / slices;
	float betaStep = M_PI / stacks;

	for (int i = 0; i < stacks; i++) {
		float beta1 = -M_PI / 2 + i * betaStep;
		float beta2 = -M_PI / 2 + (i + 1) * betaStep;

		for (int j = 0; j < slices; j++) {
			float alpha1 = j * alphaStep;
			float alpha2 = (j + 1) * alphaStep;

			float u1 = (float)j / slices;
			float u2 = (float)(j + 1) / slices;
			float v1 = (float)i / stacks;
			float v2 = (float)(i + 1) / stacks;

			float p[4][3] = {
				{radius * cos(beta1) * sin(alpha1), radius * sin(beta1), radius * cos(beta1) * cos(alpha1)},
				{radius * cos(beta1) * sin(alpha2), radius * sin(beta1), radius * cos(beta1) * cos(alpha2)},
				{radius * cos(beta2) * sin(alpha1), radius * sin(beta2), radius * cos(beta2) * cos(alpha1)},
				{radius * cos(beta2) * sin(alpha2), radius * sin(beta2), radius * cos(beta2) * cos(alpha2)}
			};

			float uv[4][2] = { {u1, v1}, {u2, v1}, {u1, v2}, {u2, v2} };
			int indices[] = { 0, 1, 2, 1, 3, 2 };

			for (int idx : indices) {
				float x = p[idx][0], y = p[idx][1], z = p[idx][2];
				float nx = x / radius, ny = y / radius, nz = z / radius;
				float u = uv[idx][0], v = uv[idx][1];

				file << x << " " << y << " " << z << " "
					<< nx << " " << ny << " " << nz << " "
					<< u << " " << v << "\n";
			}
		}
	}
	file.close();
	cout << "i => Esfera gerada com sucesso! " << endl;

}

void generateCone(float radius, float height, int slices, int stacks, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cout << "# => Error ao abrir ficheiro!" << endl;
		return;
	}

	int numVerticesBase = slices * 3;

	int numVerticesLaterias = stacks * slices * 6;

	int totalVertices = numVerticesBase + numVerticesLaterias;
	file << totalVertices << '\n';

	float angleStep = (2.0f * M_PI) / slices;
	float heightStep = height / stacks;

	// base
	for (int i = 0; i < slices; ++i) {
		float angle1 = i * angleStep;
		float angle2 = (i + 1) * angleStep;

		file << "0.0 0.0 0.0\n";

		file << (radius * sin(angle2)) << " 0.0 " << (radius * cos(angle2)) << "\n";
		file << (radius * sin(angle1)) << " 0.0 " << (radius * cos(angle1)) << "\n";
	}

	// topo
	for (int i = 0; i < stacks; ++i) {
		float h1 = i * heightStep;
		float r1 = radius * (1.0f - (float)i / stacks);

		float h2 = (i+1) * heightStep;
		float r2 = radius * (1.0f - (float)(i+1) / stacks);

		for (int j = 0; j < slices; ++j) {
			float angle1 = j * angleStep;
			float angle2 = (j + 1) * angleStep;

			float x1 = r1 * sin(angle1); float z1 = r1 * cos(angle1); // Baixo Esquerda
			float x2 = r1 * sin(angle2); float z2 = r1 * cos(angle2); // Baixo Direita
			float x3 = r2 * sin(angle1); float z3 = r2 * cos(angle1); // Cima Esquerda
			float x4 = r2 * sin(angle2); float z4 = r2 * cos(angle2); // Cima Direita

			file << x1 << " " << h1 << " " << z1 << "\n";
			file << x2 << " " << h1 << " " << z2 << "\n";
			file << x3 << " " << h2 << " " << z3 << "\n";

			file << x2 << " " << h1 << " " << z2 << "\n";
			file << x4 << " " << h2 << " " << z4 << "\n";
			file << x3 << " " << h2 << " " << z3 << "\n";
		}
	}
	file.close();
	cout << " Conse gerado com sucesso! " << totalVertices << " verices gravados em: " << filename << endl;

}

void generateTorus(float innerRadius, float outerRadius, int sides, int rings, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cout << "# => Erro ao abrir ficheiro!" << endl;
		return;
	}

	// rings (voltas do anel principal) * sides (faces do tubo) * 2 triângulos * 3 vértices
	int numVertices = rings * sides * 6;
	file << numVertices << "\n";

	float ringStep = (2.0f * M_PI) / rings;
	float sideStep = (2.0f * M_PI) / sides;

	for (int i = 0; i < rings; ++i) {
		float theta1 = i * ringStep;
		float theta2 = (i + 1) * ringStep;

		for (int j = 0; j < sides; ++j) {
			float phi1 = j * sideStep;
			float phi2 = (j + 1) * sideStep;

			// ==========================================
			// Calcular os 4 cantos da "face" atual
			// x = (R + r * cos(phi)) * cos(theta)
			// y = r * sin(phi)
			// z = (R + r * cos(phi)) * sin(theta)
			// ==========================================

			// Vértice 1: (theta1, phi1) - Canto Inferior Esquerdo
			float x1 = (outerRadius + innerRadius * cos(phi1)) * cos(theta1);
			float y1 = innerRadius * sin(phi1);
			float z1 = (outerRadius + innerRadius * cos(phi1)) * sin(theta1);

			// Vértice 2: (theta2, phi1) - Canto Inferior Direito
			float x2 = (outerRadius + innerRadius * cos(phi1)) * cos(theta2);
			float y2 = innerRadius * sin(phi1);
			float z2 = (outerRadius + innerRadius * cos(phi1)) * sin(theta2);

			// Vértice 3: (theta2, phi2) - Canto Superior Direito
			float x3 = (outerRadius + innerRadius * cos(phi2)) * cos(theta2);
			float y3 = innerRadius * sin(phi2);
			float z3 = (outerRadius + innerRadius * cos(phi2)) * sin(theta2);

			// Vértice 4: (theta1, phi2) - Canto Superior Esquerdo
			float x4 = (outerRadius + innerRadius * cos(phi2)) * cos(theta1);
			float y4 = innerRadius * sin(phi2);
			float z4 = (outerRadius + innerRadius * cos(phi2)) * sin(theta1);

			// Escrever Triângulo 1 (v1, v2, v3)
			file << x1 << " " << y1 << " " << z1 << "\n";
			file << x2 << " " << y2 << " " << z2 << "\n";
			file << x3 << " " << y3 << " " << z3 << "\n";

			// Escrever Triângulo 2 (v1, v3, v4)
			file << x1 << " " << y1 << " " << z1 << "\n";
			file << x3 << " " << y3 << " " << z3 << "\n";
			file << x4 << " " << y4 << " " << z4 << "\n";
		}
	}

	file.close();
	cout << "i => Torus gerado com sucesso! " << numVertices << " vertices gravados em: " << filename << endl;
}

void generateOrbit(float radius, int slices, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cout << "# => Erro ao abrir ficheiro!" << endl;
		return;
	}

	// Numa órbita (linha), o número de vértices é exatamente igual ao número de slices
	file << slices << "\n";

	float step = (2.0f * M_PI) / slices;

	// Gerar apenas os pontos da circunferência no plano XZ
	for (int i = 0; i < slices; ++i) {
		float angle = i * step;
		float x = radius * sin(angle);
		float z = radius * cos(angle);

		file << x << " 0.0 " << z << "\n";
	}

	file.close();
	cout << "i => Orbita gerada com sucesso! " << slices << " vertices gravados em: " << filename << endl;
}


int main(int argc, char* argv[]) {
	if (argc < 2) {
		cout << "# => Erro: Faltam argumentos" << endl;
		cout << "# => Uso: generator <forma> [parametros] <ficheiro_saida.3d" << endl;
		return 1;
	}



	string shape = argv[1];
	if (shape == "patch") {
		generateBezier(argv[2], atoi(argv[3]), argv[4]);
	}

	else if (shape == "plane") {
		if (argc != 5) {
			cout << "# => Erro: generator plane <tamanho> <divisoes> <ficheiro.3d" << endl;
			return 1;
		}
		float size = stof(argv[2]);
		int divisions = stoi(argv[3]);
		string filename = argv[4];

		generatePlane(size, divisions, filename);
	}


	else if (shape == "box") {
		if (argc != 5) {
			cout << "# => Erro: generator box <dimensao> <divisoes> <ficheiro.3d" << endl;
			return 1;
		}
		float dimension = stof(argv[2]);
		int divisions = stoi(argv[3]);
		string filename = argv[4];

		generateBox(dimension, divisions, filename);
	}
	else if (shape == "sphere") {
		if (argc != 6) {
			cout << "# => Erro: generator sphere <raio> <slices> <stacks> <ficheiro.3d" << endl;
			return 1;
		}
		float radius = stof(argv[2]);
		int slices = stoi(argv[3]);
		int stacks = stoi(argv[4]);
		string filename = argv[5];

		generateSphere(radius, slices, stacks, filename);
	}
	else if (shape == "cone") {
		if (argc != 7) {
			cout << "# => Erro: generator cone <raio_base> <altura> <slices> <stacks> <ficheiro.3d" << endl;
			return 1;
		}
		float radius = stof(argv[2]);
		float height = stof(argv[3]);
		int slices = stoi(argv[4]);
		int stacks = stoi(argv[5]);
		string filename = argv[6];

		generateCone(radius, height, slices, stacks, filename);
	}
	else if (shape == "torus") {
		if (argc != 7) {
			cout << "# => Erro: generator torus <raio_interno> <raio_externo> <lados> <aneis> <ficheiro.3d>" << endl;
			return 1;
		}
		float innerRadius = stof(argv[2]);
		float outerRadius = stof(argv[3]);
		int sides = stoi(argv[4]);
		int rings = stoi(argv[5]);
		string filename = argv[6];

		generateTorus(innerRadius, outerRadius, sides, rings, filename);
	}
	else if (shape == "orbit") {
		if (argc != 5) {
			cout << "# => Erro: generator orbit <raio> <slices> <ficheiro.3d>" << endl;
			return 1;
		}
		float radius = stof(argv[2]);
		int slices = stoi(argv[3]);
		string filename = argv[4];

		generateOrbit(radius, slices, filename);
	}
	else {
		cout << "# => Erro: Forma geometrica desconhecida ('" << shape << "')." << endl;
		return 1;
	}

	return 0;
}