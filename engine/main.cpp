#include <iostream>
#include <string>
#include <fstream>

using namespace std;

void generatePlane(float size, int divisions, const string& filename) {

}

void generateBox(float dimension, int divisions, const string& filename) {

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