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

}

void generateSphere(float radius, int slices, int stacks, const string& filename) {
}

void generateCone(float radius, float height, int slices, int stacks, const string& filename) {

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
	else {
		cout << "# => Erro: Forma geometrica desconhecida ('" << shape << "')." << endl;
		return 1;
	}

	return 0;
}