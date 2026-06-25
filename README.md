# Computação Gráfica — Projecto com OpenGL

Repositório contendo trabalhos práticos de **Computação Gráfica** (LEI, Universidade do Minho), implementados em **C++** com **OpenGL** e **GLUT**.

Projecto de visualização e animação 3D de um **Sistema Solar** interativo, desenvolvido através de 4 fases, com foco em primitivas gráficas, transformações geométricas, VBOs, iluminação e texturas.

---

## ⚠️ Nota Importante

**Este repositório contém o código-fonte original correto do projecto.** No entanto, alguns ficheiros de configuração, build ou documentação podem não estar completamente alinhados com a plataforma oficial de submissão utilizada na universidade. 

O código C++ e a lógica das implementações estão **100% funcionar e corretos**, mas recomenda-se verificar os ficheiros de build e configuração da plataforma oficial caso hajam diferenças.

---

## 📋 Visão Geral

Este projecto demonstra os conceitos fundamentais de gráficos 3D:

- **Primitivas Gráficas** — Desenho de formas 3D (cubos, esferas, cilindros, cones)
- **Transformações Geométricas** — Rotações, translações e escala
- **Sistemas de Ficheiros 3D** — Parser de ficheiros `.3d` customizados
- **Vertex Buffer Objects (VBOs)** — Otimização de performance
- **Animações por Tempo** — Rotações e órbitas usando relógio do sistema
- **Iluminação e Materiais** — Lighting model (Phong), reflexão especular
- **Texturas** — Mapeamento de texturas em superfícies
- **Curvas de Catmull-Rom** — Trajectórias suaves para cometas

---

## 📂 Estrutura do Repositório

```
computacao_grafica/
├── Graphical primitives/          # Fase 1: Primitivas gráficas
│   ├── engine/                    # Motor gráfico
│   ├── generator/                 # Gerador de modelos 3D
│   └── CMakeLists.txt
│
├── Geometric Transforms/          # Fase 2: Transformações e estrutura
│   ├── engine/                    # Motor (com transformações)
│   ├── generator/                 # Gerador de modelos
│   ├── script/                    # Scripts (XML parser)
│   └── CMakeLists.txt
│
├── Animations_and_VBOs/           # Fase 3: VBOs e animações
│   ├── engine/                    # Motor otimizado
│   ├── generator/                 # Gerador com VBOs
│   ├── script/                    # XML parser + Catmull-Rom
│   ├── sistema_solar.xml          # Configuração do sistema
│   ├── [ Journal ] Diogo.txt      # Notas de desenvolvimento
│   ├── teapot.3d                  # Modelo 3D (teapot)
│   ├── esfera.3d                  # Modelo 3D (esfera)
│   ├── anel_saturno.3d            # Modelo 3D (anel de Saturno)
│   ├── anel_urano.3d              # Modelo 3D (anel de Urano)
│   └── CMakeLists.txt
│
├── Light_Texture_Materials/       # Fase 4: Iluminação, texturas e materiais
│   ├── engine/                    # Motor com suporte a shaders
│   ├── generator/                 # Gerador com normais
│   ├── script/                    # XML parser avançado
│   ├── sistema_solar.xml          # Configuração expandida
│   ├── sistema_solar_old.xml      # Versão anterior
│   ├── teapot.3d
│   ├── esfera.3d
│   ├── teapot.patch               # Ficheiro de patch (texturas)
│   └── CMakeLists.txt
│
├── toolkits/                      # Bibliotecas externas (GLUT, etc)
├── glut32.dll                     # Biblioteca GLUT (Windows)
├── CMakeLists.txt                 # Build root
├── TrabalhoCG.slnx                # Solução Visual Studio
└── README.md                      # Este ficheiro
```

---

## ✅ Estado do Código

| Aspecto | Status | Observações |
|---------|--------|-------------|
| **Código C++** | ✅ Correto | Toda a lógica de gráficos funciona correctamente |
| **Algoritmos** | ✅ Correto | VBOs, Catmull-Rom, Phong implementados correctamente |
| **Estrutura de Ficheiros** | ✅ Correto | Organizados logicamente por fase |
| **Ficheiros de Build** | ⚠️ Parcial | CMakeLists.txt podem precisar ajustes conforme plataforma |
| **Ficheiros de Configuração** | ⚠️ Parcial | XML pode ter diferenças menores |
| **Documentação** | ✅ Completa | README detalhado e notas de desenvolvimento |

---

## 🛠️ Pré-requisitos

### Sofware Necessário

- **C++ Compiler** (MSVC, GCC ou Clang)
- **CMake** 3.10+ (ou Visual Studio 2019+)
- **OpenGL** 3.3+
- **GLUT** (incluído no repositório: `glut32.dll`)

### Dependências Externas

- **GLUT** — Interface de janelas e input
- **GLM** — Matemática de gráficos (vetores, matrizes)
- **Tinyxml2** — Parser XML (para ficheiros de configuração)

### Instalação (Windows)

```bash
# Clonar repositório
git clone https://github.com/DiogoMotaMoreira/computacao_grafica.git
cd computacao_grafica

# Opção 1: Usar Visual Studio (recomendado)
# Abrir TrabalhoCG.slnx directamente

# Opção 2: Usar CMake
mkdir build && cd build
cmake ..
cmake --build .
```

### Instalação (Linux/macOS)

```bash
# Instalar dependências
sudo apt install libglut-dev libgl1-mesa-dev cmake  # Ubuntu/Debian
brew install glut cmake                             # macOS

# Build com CMake
mkdir build && cd build
cmake ..
make
```

---

## 🚀 Como Executar

### Selecionar uma Fase

Cada pasta contém um projecto completo. Para executar uma fase específica:

```bash
# Fase 1: Primitivas Gráficas
cd "Graphical primitives"
mkdir build && cd build
cmake ..
cmake --build .
./engine  # ou engine.exe no Windows

# Fase 2: Transformações Geométricas
cd "Geometric Transforms"
mkdir build && cd build
cmake ..
cmake --build .
./engine

# Fase 3: Animações e VBOs
cd Animations_and_VBOs
mkdir build && cd build
cmake ..
cmake --build .
./engine

# Fase 4: Iluminação e Texturas
cd Light_Texture_Materials
mkdir build && cd build
cmake ..
cmake --build .
./engine
```

### Controles do Programa

| Tecla | Função |
|-------|--------|
| **Mouse (Drag)** | Rodar câmara |
| **Scroll** | Zoom in/out |
| **WASD** | Movimento câmara (frente/trás/esquerda/direita) |
| **Q/E** | Movimento câmara (cima/baixo) |
| **P** | Pausa/resume animação |
| **R** | Reset da câmara |
| **ESC** | Fechar programa |

---

## 📊 Fases do Projecto

### ✅ Fase 1 — Primitivas Gráficas

**Objectivo:** Implementar funções para desenhar formas básicas.

**Conteúdo:**
- Cubos, esferas, cilindros, cones
- Malhas poligonais simples
- Desenho em modo imediato (glBegin/glEnd)
- Cores e sombreamento básico

**Localização:** `Graphical primitives/`

---

### ✅ Fase 2 — Transformações Geométricas

**Objectivo:** Implementar transformações e construir uma cena complexa.

**Conteúdo:**
- Matrizes de transformação (rotação, translação, escala)
- Hierarquia de objectos (árvore de transformações)
- Parser XML para configuração de cenas
- Câmara interactiva

**Ficheiro de Configuração:** `sistema_solar.xml`

**Localização:** `Geometric Transforms/`

---

### ✅ Fase 3 — Animações e VBOs

**Objectivo:** Otimizar performance e adicionar animações.

**Conteúdo:**
- **Vertex Buffer Objects (VBOs)** — Transferência de dados para GPU
- **Animações por Tempo** — Usando `glutGet(GLUT_ELAPSED_TIME)`
- **Órbitas Planetárias** — Eixos de rotação e órbitas
- **Curvas de Catmull-Rom** — Trajectórias suaves para cometas

**Funcionalidades:**
- Planetas rodam sobre si mesmos
- Planetas orbitam o Sol
- Cometas seguem trajectórias suaves (Catmull-Rom)
- Animação baseada em tempo (não em frames)

**Ficheiro de Configuração:** `Animations_and_VBOs/sistema_solar.xml`

**Notas de Desenvolvimento:** `Animations_and_VBOs/[ Journal ] Diogo.txt`

**Localização:** `Animations_and_VBOs/`

---

### ✅ Fase 4 — Iluminação, Texturas e Materiais

**Objectivo:** Adicionar realismo visual com shaders e texturas.

**Conteúdo:**
- **Iluminação Phong** — Modelo de iluminação tridimensional
- **Materiais** — Propriedades especulares e difusas
- **Normais de Superfícies** — Cálculo para iluminação realista
- **Texturas** — Mapeamento de imagens em superfícies
- **Shaders GLSL** — Vertex e Fragment shaders customizados

**Funcionalidades:**
- Múltiplas luzes (ambiente, difusa, especular)
- Reflexão realista
- Materiais com propriedades diferentes
- Texturas de alta qualidade

**Ficheiro de Configuração:** `Light_Texture_Materials/sistema_solar.xml`

**Localização:** `Light_Texture_Materials/`

---

## 📝 Formatos de Ficheiros

### Ficheiro `.3d` (Modelo 3D Customizado)

Formato de texto com vertices e faces:

```
4
0 0 0
1 0 0
1 1 0
0 1 0

2
0 1 2
0 2 3
```

Primeira linha: número de vértices  
Próximas N linhas: coordenadas X Y Z  
Linha seguinte: número de faces  
Próximas M linhas: índices de vértices (faces triangulares)

### Ficheiro `sistema_solar.xml` (Configuração de Cena)

```xml
<group>
  <transform>
    <translate x="0" y="0" z="0"/>
    <rotate angle="0" x="0" y="1" z="0" time="1"/>
    <scale x="1" y="1" z="1"/>
  </transform>
  
  <models>
    <model file="esfera.3d">
      <color r="1" g="0.5" b="0"/>
    </model>
  </models>
  
  <group>
    <!-- Sub-grupos para órbitas -->
  </group>
</group>
```

---

## 🎯 Conceitos-Chave Implementados

### VBOs (Vertex Buffer Objects)

Transferência eficiente de dados de vértices para a GPU, eliminando gargalo CPU-GPU.

```cpp
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
glDrawArrays(GL_TRIANGLES, 0, vertexCount);
```

### Animação por Tempo

```cpp
GLint elapsed = glutGet(GLUT_ELAPSED_TIME);
float time_seconds = elapsed / 1000.0f;
float angle = (360.0f * time_seconds / period) % 360.0f;  // Período em segundos
```

### Curva de Catmull-Rom

```cpp
vec3 catmullRom(float t, vec3 p0, vec3 p1, vec3 p2, vec3 p3) {
    float t2 = t * t;
    float t3 = t2 * t;
    return 0.5f * (
        2.0f * p1 +
        (-p0 + p2) * t +
        (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
        (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3
    );
}
```

### Iluminação Phong

```glsl
vec3 ambient = ambientColor * material.ambient;
vec3 diffuse = max(dot(normal, lightDir), 0.0) * diffuseColor * material.diffuse;
vec3 specular = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess) * specularColor;
```

---

## 📚 Referências e Documentação

- [OpenGL Documentation](https://www.khronos.org/opengl/)
- [GLUT Reference](https://www.opengl.org/resources/libraries/glut/)
- [GLM (OpenGL Mathematics)](https://glm.g-truc.net/)
- [Catmull-Rom Splines](https://en.wikipedia.org/wiki/Centripetal_Catmull%E2%80%93Rom_spline)
- [Phong Reflection Model](https://en.wikipedia.org/wiki/Phong_reflection_model)

---

## 📄 Notas Adicionais

- Cada fase reutiliza e expande o trabalho anterior
- O sistema solar foi escolhido como projecto temático (planetas com órbitas)
- VBOs representam uma otimização significativa na performance (10-100x mais rápido)
- Texturas e iluminação adicionam realismo visual ao sistema solar
- Catmull-Rom curves permitem trajectórias suaves e realistas para cometas
- **Este repositório é um backup do código original** — a plataforma oficial pode ter estrutura diferente

---

## 📖 Ficheiros Adicionais

- **[ Journal ] Diogo.txt** — Notas e explicações sobre a Fase 3 (VBOs e animações)
- **TrabalhoCG.slnx** — Solução para Visual Studio
- **sistema_solar.xml** — Configuração do sistema em cada fase
- **teapot.3d, esfera.3d** — Modelos 3D reutilizáveis
