//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


#define VC_DEBUG
#define MAX(a, b) (a>b? a:b)
#define MIN(a, b) (a<b? a:b)

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			// Binário/Cinzentos=1; RGB=3
	int levels;				// Binário=1; Cinzentos [1,255]; RGB [1,255]
	int bytesperline;		// width * channels
} IVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);

// FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);

//FUNÇÕES PARA CALCULAR NEGATIVO (GRAY E RGB)
int vc_gray_negative(IVC* srcdst);
int vc_rgb_negative(IVC* srcdst);

//FUNÇÃO DE CONVERTER RGB EM GRAY
int vc_rgb_to_gray(IVC* src, IVC* dst);

//BGR PARA RGB
int vc_bgr_to_rgb(IVC* src, IVC* dst);

//BINARIO PARA RGB
int vc_binary_to_rgb(IVC* src, IVC* dst);


//FUNCÃO QUE CONVERTE RGB EM HSV
int vc_rgb_to_hsv(IVC* src, IVC* dst);

// SEGMENTAR A IMAGEM  [FUNDO A PRETO, IMAGEM A BRANCO]
int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);

// CONTAR OS PIXEIS BRANCOS DE UMA IMAGEM
int vc_count_white(IVC* src, int* white);

// CONVERTE IMAGEM CINZENTA MUMA IMAGEM COM ESCALA DE CORES RGB
int vc_scale_gray_to_color_palette(IVC* src, IVC* dst);

// CONVERTE IMAGEM GRAY EM IMAGEM BINÁRIA
int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);

// CONVERTE IMAGEM GRAY EM IMAGEM BINÁRIA ATRAVES DA MEDIA GLOBAL
int vc_gray_to_binary_global_mean(IVC* src, IVC* dst);

//GRAY TO BINARY MIDPOINT
int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);

//GRAY TO BINARY BERNSEN
int vc_gray_to_binary_bernsen(IVC* src, IVC* dst, int kernel, int cmin);

//GRAY TO BINARY NIBLACK
int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k);

// DILATAÇÃO NUMA IAGEM BINARIA
int vc_binary_dilate(IVC* src, IVC* dst, int kernel);

// EROSAO NUMA IMAGEM BIN
int vc_binary_erode(IVC* src, IVC* dst, int kernel);

// OPEN EROSÃO -> DILATAÇÃO
int vc_binary_open(IVC* src, IVC* dst, int kernelErode, int kernelDilate);

// CLOSE DILATAÇÃO->DILATAÇÃO
int vc_binary_close(IVC* src, IVC* dst, int kernelDilate, int kernelErode);

// SUBTRAIR UM IMAGME A OUTRA (MESMA IMAGEM COM DIFERENTES THRESHOLD)
int vc_binary_subtract(IVC* src1, IVC* src2, IVC* dst);

/// EQUALIZAR UMA IMAGEM CINZENTA
int vc_gray_histogram_equalization(IVC* src, IVC* dst);

// CALCULAR CONTRNOS CINZENTOS UTILIZANDO OPERADORES PREWITT
int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);

// CALCULAR CONTRNOS CINZENTOS UTILIZANDO OPERADORES SOBEL
int vc_gray_edge_sobel(IVC* src, IVC* dst, float th);

/// FILTRO PASSA-BAIXO MEDIA GRAY
int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize);

/// FILTRO PASSA-BAIXO MEDIANA GRAY
int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize);

///
int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst);


int vc_gray_highpass_filter(IVC* src, IVC* dst);

int vc_gray_highpass_filter_enhance(IVC* src, IVC* dst, int gain);




//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                   ESTRUTURA DE UM BLOB (OBJECTO)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

typedef struct {
	int x, y, width, height;	// Caixa Delimitadora (Bounding Box)
	int area;					// Área
	int xc, yc;					// Centro-de-massa
	int perimeter;				// Perímetro
	int label;					// Etiqueta
} OVC;


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//                    PROTÓTIPOS DE FUNÇÕES
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// ETIQUETAGEM DE UMA IMAGEM
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

int vc_draw_bounding_box(IVC* image, OVC* blobs, int nblobs);

int vc_draw_center_mass(IVC* image, OVC* blobs, int nblobs, int size);


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//        FUNÇÕES: Laranjas - TP
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#define VC_GRUPO_CALIBRE_0_2 0
#define VC_GRUPO_CALIBRE_3_6 1
#define VC_GRUPO_CALIBRE_7_13 2
#define VC_N_GRUPOS_CALIBRE 3

#define MAX_LARANJAS 200
#define DISTANCIA_MESMA_LARANJA 45

typedef struct {
	int ativo;
	int xc, yc;
	float melhor_area;
	float melhor_perimetro;
	float melhor_diametro;
	int melhor_calibre;
} LARANJA_INFO;

int vc_laranja_completa_na_imagem(OVC blob, int width, int height, int margem);

int obter_calibre(float diametro_mm);

int vc_homogeneidade_obter_grupo_calibre(int calibre);

int vc_homogeneidade_obter_limite_grupo(int grupo);

void vc_homogeneidade_inicializar(float minDiam[],float maxDiam[],int countGrupo[]);

void vc_homogeneidade_atualizar(float diametro_mm,int calibre,float minDiam[],float maxDiam[],int countGrupo[]);

int vc_homogeneidade_verificar(float minDiam[],float maxDiam[],int countGrupo[],float* maiorDiferenca);

int encontrar_laranja(LARANJA_INFO laranjas[], int total, int xc, int yc);

float vc_detetar_defeitos(IVC* hsv, OVC blob, int kernel, int c);

int obter_categoria(float percent_defeitos);
