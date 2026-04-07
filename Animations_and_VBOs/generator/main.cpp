#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>

using namespace std;

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

void generateSphere(float radius, int slices, int stacks, const string& filename) {
	ofstream file(filename);
	if (!file.is_open()) {
		cout << "# => Error ao abrir ficheiro!" << endl;
		return;
	}

	// i -> stacks quadrados
	int numVertices = slices * stacks * 6;
	file << numVertices << "\n";

	// angulo a andar a cada setp
	float sliceStep = (2 * M_PI) / slices;

	// angulo do topo até a parte de baixo
	float stackStep = M_PI / stacks;

	for (int i = 0; i < stacks; ++i) {
		float stackAngle1 = (-M_PI / 2.0f) + (i * stackStep);
		float stackAngle2 = (-M_PI / 2.0f) + ((i + 1) * stackStep);

		for (int j = 0; j < slices; ++j) {
			float sliceAngle1 = j * sliceStep;
			float sliceAngle2 = (j + 1) * sliceStep;

			// baixo esquerda
			float x1 = radius * cos(stackAngle1) * sin(sliceAngle1);
			float y1 = radius * sin(stackAngle1);
			float z1 = radius * cos(stackAngle1) * cos(sliceAngle1);
			// baixo direita
			float x2 = radius * cos(stackAngle1) * sin(sliceAngle2);
			float y2 = radius * sin(stackAngle1);
			float z2 = radius * cos(stackAngle1) * cos(sliceAngle2);
			// cima esquerda 
			float x3 = radius * cos(stackAngle2) * sin(sliceAngle1);
			float y3 = radius * sin(stackAngle2);
			float z3 = radius * cos(stackAngle2) * cos(sliceAngle1);
			// cima direita
			float x4 = radius * cos(stackAngle2) * sin(sliceAngle2);
			float y4 = radius * sin(stackAngle2);
			float z4 = radius * cos(stackAngle2) * cos(sliceAngle2);

			file << x1 << " " << y1 << " " << z1 << '\n';
			file << x2 << " " << y2 << " " << z2 << '\n';
			file << x3 << " " << y3 << " " << z3 << '\n';

			file << x2 << " " << y2 << " " << z2 << '\n';
			file << x4 << " " << y4 << " " << z4 << '\n';
			file << x3 << " " << y3 << " " << z3 << '\n';
		}
	}
	file.close();
	cout << "i => Esfera gerada com sucesso! " << numVertices << " vertices gravados em: " << filename << endl;

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

	if (shape == "plane") {
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