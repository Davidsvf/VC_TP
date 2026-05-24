


#define VC_DEBUG
#define MAX(a, b) (a>b? a:b)
#define MIN(a, b) (a<b? a:b)




typedef struct {
	unsigned char *data;
	int width, height;
	int channels;			
	int levels;				
	int bytesperline;		
} IVC;




IVC *vc_image_new(int width, int height, int channels, int levels);
IVC *vc_image_free(IVC *image);


IVC *vc_read_image(char *filename);
int vc_write_image(char *filename, IVC *image);


int vc_gray_negative(IVC* srcdst);
int vc_rgb_negative(IVC* srcdst);


int vc_rgb_to_gray(IVC* src, IVC* dst);


int vc_bgr_to_rgb(IVC* src, IVC* dst);


int vc_binary_to_rgb(IVC* src, IVC* dst);



int vc_rgb_to_hsv(IVC* src, IVC* dst);


int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax);


int vc_count_white(IVC* src, int* white);


int vc_scale_gray_to_color_palette(IVC* src, IVC* dst);


int vc_gray_to_binary(IVC* src, IVC* dst, int threshold);


int vc_gray_to_binary_global_mean(IVC* src, IVC* dst);


int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel);


int vc_gray_to_binary_bersen(IVC* src, IVC* dst, int kernel, int cmin);


int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k);


int vc_binary_dilate(IVC* src, IVC* dst, int kernel);


int vc_binary_erode(IVC* src, IVC* dst, int kernel);


int vc_binary_open(IVC* src, IVC* dst, int kernelErode, int kernelDilate);


int vc_binary_close(IVC* src, IVC* dst, int kernelDilate, int kernelErode);


int vc_binary_subtract(IVC* src1, IVC* src2, IVC* dst);


int vc_gray_histogram_equalization(IVC* src, IVC* dst);

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th);

int vc_gray_edge_sobel(IVC* src, IVC* dst, float th);

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize);

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize);

int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst);


int vc_gray_highpass_filter(IVC* src, IVC* dst);

int vc_gray_highpass_filter_enhance(IVC* src, IVC* dst, int gain);






typedef struct {
	int x, y, width, height;	
	int area;					
	int xc, yc;					
	int perimeter;				
	int label;					
} OVC;



OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels);
int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs);

int vc_draw_bounding_box(IVC* image, OVC* blobs, int nblobs);

int vc_draw_center_mass(IVC* image, OVC* blobs, int nblobs, int size);






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

int vc_obter_calibre(float diametro_mm);

int vc_homogeneidade_obter_grupo_calibre(int calibre);

int vc_homogeneidade_obter_limite_grupo(int grupo);

void vc_homogeneidade_inicializar(float minDiam[],float maxDiam[],int countGrupo[]);

void vc_homogeneidade_atualizar(float diametro_mm,int calibre,float minDiam[],float maxDiam[],int countGrupo[]);

int vc_homogeneidade_verificar(float minDiam[],float maxDiam[],int countGrupo[],float* maiorDiferenca);

int vc_encontrar_laranja(LARANJA_INFO laranjas[], int total, int xc, int yc);

float vc_detetar_defeitos(IVC* hsv, OVC blob, int kernel, int c);

int vc_obter_categoria(float percent_defeitos);
