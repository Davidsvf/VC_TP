# Sistema de Visão por Computador para Avaliação de Qualidade de Laranjas

Projeto de visão por computador desenvolvido para detetar, medir e classificar automaticamente laranjas a partir de vídeo, recorrendo a técnicas de processamento digital de imagem implementadas em C/C++ com OpenCV.

O sistema realiza segmentação por cor, filtragem morfológica, análise de componentes conexos, medição geométrica, deteção de defeitos e classificação de qualidade. Inclui ainda mecanismos para reduzir a contagem duplicada de objetos entre frames e para avaliar a homogeneidade dimensional do lote processado.

## Visão Geral

A aplicação processa um vídeo frame a frame e extrai informação visual e geométrica relevante de cada laranja detetada.

O pipeline principal é composto pelas seguintes etapas:

```text
Entrada de Vídeo
      |
      v
Aquisição do Frame
      |
      v
Conversão BGR / RGB
      |
      v
Conversão para HSV
      |
      v
Segmentação por Cor
      |
      v
Filtragem Morfológica
      |
      v
Connected Component Labelling
      |
      v
Filtragem e Análise dos Objetos
      |
      v
Estimativa de Diâmetro e Calibre
      |
      v
Análise de Defeitos
      |
      v
Classificação de Qualidade
      |
      v
Avaliação da Homogeneidade do Lote
```

## Funcionalidades

- Processamento de vídeo frame a frame
- Conversão entre espaços de cor RGB e HSV
- Segmentação de objetos com base na cor
- Processamento de imagens binárias
- Operações morfológicas de erosão e dilatação
- Operações de opening e closing
- Connected Component Labelling
- Análise de blobs
- Filtragem de objetos com base em propriedades geométricas
- Cálculo de bounding boxes
- Determinação do centro de massa
- Cálculo de área e perímetro
- Estimativa do diâmetro
- Conversão de pixels para unidades físicas
- Classificação por calibre
- Deteção de defeitos superficiais
- Classificação por categoria de qualidade
- Associação de objetos entre frames consecutivos
- Redução de contagens duplicadas
- Avaliação da homogeneidade de calibre do lote
- Visualização dos resultados sobre o vídeo processado

## Tecnologias

- C
- C++
- OpenCV
- Visual Studio
- MSVC
- Processamento Digital de Imagem
- Visão por Computador

## Processamento de Imagem

### Segmentação por Cor

Cada frame do vídeo é convertido para o espaço de cor HSV.

A utilização de HSV permite separar a informação de tonalidade, saturação e luminosidade, facilitando a segmentação das laranjas relativamente ao fundo da imagem.

A segmentação utiliza intervalos definidos para:

- Hue
- Saturation
- Value

O resultado desta etapa é uma imagem binária que identifica as regiões candidatas a laranjas.

## Operações Morfológicas

Após a segmentação, são aplicadas operações morfológicas para melhorar a qualidade da imagem binária.

O projeto utiliza:

- Erosão
- Dilatação
- Opening
- Closing

Estas operações permitem reduzir ruído, remover pequenas regiões indesejadas e preencher descontinuidades nos objetos segmentados.

## Análise de Componentes Conexos

Após o processamento morfológico, é aplicado Connected Component Labelling para identificar individualmente os objetos presentes na imagem binária.

Para cada componente são calculadas propriedades como:

- Área
- Perímetro
- Largura
- Altura
- Bounding box
- Centro de massa

São posteriormente aplicados critérios geométricos para excluir componentes que não correspondam às características esperadas de uma laranja.

## Medição dos Objetos

As dimensões de cada laranja são estimadas a partir da respetiva bounding box.

O diâmetro aproximado é determinado através da largura e altura do objeto:

```text
diâmetro ≈ (largura + altura) / 2
```

Posteriormente, é utilizado um fator de conversão para transformar os valores em pixels numa aproximação das dimensões físicas do objeto.

Entre as propriedades calculadas encontram-se:

- Área
- Perímetro
- Diâmetro
- Calibre

## Classificação por Calibre

Após a estimativa do diâmetro, cada laranja é associada ao respetivo calibre.

O sistema utiliza ainda diferentes grupos de calibres para avaliar a homogeneidade dimensional do conjunto processado.

Os grupos considerados incluem:

```text
Calibres 0–2
Calibres 3–6
Calibres 7–13
```

## Deteção de Defeitos

A região correspondente a cada laranja é analisada individualmente para identificar possíveis defeitos visuais na superfície.

O sistema calcula uma percentagem de defeito associada ao objeto, utilizada posteriormente na classificação da qualidade da laranja.

## Classificação de Qualidade

Com base na análise dos defeitos detetados, cada laranja é classificada numa das seguintes categorias:

| Categoria | Descrição |
|---|---|
| EXTRA | Qualidade superior |
| I | Categoria I |
| II | Categoria II |
| REJ | Objeto rejeitado |

Esta classificação é apresentada diretamente sobre o vídeo durante o processamento.

## Seguimento entre Frames

Como uma mesma laranja pode permanecer visível durante vários frames consecutivos, o sistema inclui uma lógica de associação entre deteções.

São armazenadas informações relativas aos objetos anteriormente identificados, incluindo a posição do centro de massa e as melhores medições registadas.

```cpp
typedef struct {
    int ativo;
    int xc, yc;
    float melhor_area;
    float melhor_perimetro;
    float melhor_diametro;
    int melhor_calibre;
} LARANJA_INFO;
```

Esta abordagem permite reduzir a probabilidade de contabilizar várias vezes o mesmo objeto.

## Avaliação da Homogeneidade

O sistema analisa também a distribuição dos calibres das laranjas detetadas.

Esta informação permite determinar se o lote apresenta consistência relativamente às dimensões dos frutos processados.

## Informação Apresentada

Durante a execução, o sistema apresenta informação individual e global sobre o processamento.

Para cada laranja podem ser apresentados:

- Área
- Perímetro
- Diâmetro
- Calibre
- Percentagem de defeitos
- Categoria de qualidade
- Bounding box
- Centro de massa

A aplicação apresenta também indicadores globais como:

- Número total de frames
- Frame atual
- Número de laranjas no frame
- Número total de laranjas identificadas
- Estado da homogeneidade do lote
- Diferença máxima entre diâmetros

## Estrutura do Projeto

```text
VC_tp/
│
├── Frames/
│
├── main.cpp
├── vc.c
├── vc.h
├── video.avi
│
├── ProjetoOpenVC.sln
├── ProjetoOpenVC.vcxproj
├── ProjetoOpenVC.vcxproj.filters
│
├── opencv_world4130.dll
├── .gitignore
└── .gitattributes
```

### Ficheiros Principais

`main.cpp`

Responsável pelo fluxo principal da aplicação, leitura do vídeo, processamento dos frames e apresentação dos resultados.

`vc.c`

Contém a implementação das funções de processamento digital de imagem utilizadas pelo sistema.

`vc.h`

Define as estruturas de dados e os protótipos das funções utilizadas no projeto.

## Principais Funções

### Conversão de Espaços de Cor e Segmentação

```cpp
vc_bgr_to_rgb()
vc_rgb_to_hsv()
vc_hsv_segmentation()
```

### Operações Morfológicas

```cpp
vc_binary_dilate()
vc_binary_erode()
vc_binary_open()
vc_binary_close()
```

### Análise de Objetos

```cpp
vc_binary_blob_labelling()
vc_binary_blob_info()
vc_draw_bounding_box()
vc_draw_center_mass()
```

### Processamento das Laranjas

```cpp
vc_laranja_completa_na_imagem()
vc_obter_calibre()
vc_encontrar_laranja()
vc_detetar_defeitos()
vc_obter_categoria()
```

### Homogeneidade

```cpp
vc_homogeneidade_inicializar()
vc_homogeneidade_atualizar()
vc_homogeneidade_verificar()
```

## Requisitos

O projeto foi desenvolvido para ambiente Windows e utiliza:

- Visual Studio 2022
- Toolset MSVC v143
- OpenCV 4.13.0
- Arquitetura x64

A configuração atual do projeto assume uma instalação do OpenCV em:

```text
C:\opencv
```

e utiliza a biblioteca:

```text
opencv_world4130.lib
```

## Instalação

Clonar o repositório:

```bash
git clone https://github.com/RafaelCosta06/VC_tp.git
```

Aceder à diretoria:

```bash
cd VC_tp
```

Abrir a solução:

```text
ProjetoOpenVC.sln
```

No Visual Studio, selecionar uma configuração x64 e compilar o projeto.

Caso o OpenCV esteja instalado numa localização diferente de `C:\opencv`, é necessário atualizar os caminhos de include e das bibliotecas nas propriedades do projeto.

## Execução

O programa utiliza por defeito o ficheiro:

```text
video.avi
```

O caminho encontra-se definido no código:

```cpp
char videofile[20] = "video.avi";
```

Para utilizar outro vídeo, é necessário substituir o ficheiro ou alterar o caminho configurado no código.

Durante a execução, a tecla:

```text
q
```

permite terminar o processamento.

## Competências Demonstradas

Este projeto demonstra conhecimentos práticos em:

- Desenvolvimento em C e C++
- Utilização da biblioteca OpenCV
- Processamento de vídeo
- Processamento digital de imagem
- Segmentação baseada em cor
- Operações morfológicas
- Connected Component Labelling
- Extração de características geométricas
- Deteção e classificação de objetos
- Análise de defeitos
- Seguimento simplificado de objetos
- Conversão entre coordenadas de imagem e medidas físicas
- Desenvolvimento de pipelines de visão por computador

## Contexto

Projeto desenvolvido no âmbito da unidade curricular de Visão por Computador, com foco na aplicação prática de técnicas clássicas de processamento digital de imagem a um problema de inspeção visual e controlo de qualidade.

O objetivo consiste em automatizar a análise de produtos agrícolas através de características visuais e geométricas extraídas diretamente de vídeo.

## Licença

Projeto desenvolvido para fins académicos e de demonstração técnica.
