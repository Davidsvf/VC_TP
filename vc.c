//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//           INSTITUTO POLITÉCNICO DO CÁVADO E DO AVE
//                          2022/2023
//             ENGENHARIA DE SISTEMAS INFORMÁTICOS
//                    VISÃO POR COMPUTADOR
//
//             [  DUARTE DUQUE - dduque@ipca.pt  ]
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Desabilita (no MSVC++) warnings de funções não seguras (fopen, sscanf, etc...)
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//            FUNÇÕES: ALOCAR E LIBERTAR UMA IMAGEM
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


// Alocar memória para uma imagem
IVC *vc_image_new(int width, int height, int channels, int levels)
{
	IVC *image = (IVC *) malloc(sizeof(IVC));

	if(image == NULL) return NULL;
	if((levels <= 0) || (levels > 255)) return NULL;

	image->width = width;
	image->height = height;
	image->channels = channels;
	image->levels = levels;
	image->bytesperline = image->width * image->channels;
	image->data = (unsigned char *) malloc(image->width * image->height * image->channels * sizeof(char));

	if(image->data == NULL)
	{
		return vc_image_free(image);
	}

	return image;
}


// Libertar memória de uma imagem
IVC *vc_image_free(IVC *image)
{
	if(image != NULL)
	{
		if(image->data != NULL)
		{
			free(image->data);
			image->data = NULL;
		}

		free(image);
		image = NULL;
	}

	return image;
}


//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//    FUNÇÕES: LEITURA E ESCRITA DE IMAGENS (PBM, PGM E PPM)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


char *netpbm_get_token(FILE *file, char *tok, int len)
{
	char *t;
	int c;
	
	for(;;)
	{
		while(isspace(c = getc(file)));
		if(c != '#') break;
		do c = getc(file);
		while((c != '\n') && (c != EOF));
		if(c == EOF) break;
	}
	
	t = tok;
	
	if(c != EOF)
	{
		do
		{
			*t++ = c;
			c = getc(file);
		} while((!isspace(c)) && (c != '#') && (c != EOF) && (t - tok < len - 1));
		
		if(c == '#') ungetc(c, file);
	}
	
	*t = 0;
	
	return tok;
}


long int unsigned_char_to_bit(unsigned char *datauchar, unsigned char *databit, int width, int height)
{
	int x, y;
	int countbits;
	long int pos, counttotalbytes;
	unsigned char *p = databit;

	*p = 0;
	countbits = 1;
	counttotalbytes = 0;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//*p |= (datauchar[pos] != 0) << (8 - countbits);
				
				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				*p |= (datauchar[pos] == 0) << (8 - countbits);

				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				*p = 0;
				countbits = 1;
				counttotalbytes++;
			}
		}
	}

	return counttotalbytes;
}


void bit_to_unsigned_char(unsigned char *databit, unsigned char *datauchar, int width, int height)
{
	int x, y;
	int countbits;
	long int pos;
	unsigned char *p = databit;

	countbits = 1;

	for(y=0; y<height; y++)
	{
		for(x=0; x<width; x++)
		{
			pos = width * y + x;

			if(countbits <= 8)
			{
				// Numa imagem PBM:
				// 1 = Preto
				// 0 = Branco
				//datauchar[pos] = (*p & (1 << (8 - countbits))) ? 1 : 0;

				// Na nossa imagem:
				// 1 = Branco
				// 0 = Preto
				datauchar[pos] = (*p & (1 << (8 - countbits))) ? 0 : 1;
				
				countbits++;
			}
			if((countbits > 8) || (x == width - 1))
			{
				p++;
				countbits = 1;
			}
		}
	}
}


IVC *vc_read_image(char *filename)
{
	FILE *file = NULL;
	IVC *image = NULL;
	unsigned char *tmp;
	char tok[20];
	long int size, sizeofbinarydata;
	int width, height, channels;
	int levels = 255;
	int v;
	
	// Abre o ficheiro
	if((file = fopen(filename, "rb")) != NULL)
	{
		// Efectua a leitura do header
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	// Se PBM (Binary [0,1])
		else if(strcmp(tok, "P5") == 0) channels = 1;				// Se PGM (Gray [0,MAX(level,255)])
		else if(strcmp(tok, "P6") == 0) channels = 3;				// Se PPM (RGB [0,MAX(level,255)])
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) // PBM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			if((v = fread(tmp, sizeof(unsigned char), sizeofbinarydata, file)) != sizeofbinarydata)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				free(tmp);
				return NULL;
			}

			bit_to_unsigned_char(tmp, image->data, image->width, image->height);

			free(tmp);
		}
		else // PGM ou PPM
		{
			if(sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &width) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &height) != 1 || 
			   sscanf(netpbm_get_token(file, tok, sizeof(tok)), "%d", &levels) != 1 || levels <= 0 || levels > 255)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tFile is not a valid PGM or PPM file.\n\tBad size!\n");
				#endif

				fclose(file);
				return NULL;
			}

			// Aloca memória para imagem
			image = vc_image_new(width, height, channels, levels);
			if(image == NULL) return NULL;

			#ifdef VC_DEBUG
			printf("\nchannels=%d w=%d h=%d levels=%d\n", image->channels, image->width, image->height, levels);
			#endif

			size = image->width * image->height * image->channels;

			if((v = fread(image->data, sizeof(unsigned char), size, file)) != size)
			{
				#ifdef VC_DEBUG
				printf("ERROR -> vc_read_image():\n\tPremature EOF on file.\n");
				#endif

				vc_image_free(image);
				fclose(file);
				return NULL;
			}
		}
		
		fclose(file);
	}
	else
	{
		#ifdef VC_DEBUG
		printf("ERROR -> vc_read_image():\n\tFile not found.\n");
		#endif
	}
	
	return image;
}


int vc_write_image(char *filename, IVC *image)
{
	FILE *file = NULL;
	unsigned char *tmp;
	long int totalbytes, sizeofbinarydata;
	
	if(image == NULL) return 0;

	if((file = fopen(filename, "wb")) != NULL)
	{
		if(image->levels == 1)
		{
			sizeofbinarydata = (image->width / 8 + ((image->width % 8) ? 1 : 0)) * image->height + 1;
			tmp = (unsigned char *) malloc(sizeofbinarydata);
			if(tmp == NULL) return 0;
			
			fprintf(file, "%s %d %d\n", "P4", image->width, image->height);
			
			totalbytes = unsigned_char_to_bit(image->data, tmp, image->width, image->height);
			printf("Total = %ld\n", totalbytes);
			if(fwrite(tmp, sizeof(unsigned char), totalbytes, file) != totalbytes)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				free(tmp);
				return 0;
			}

			free(tmp);
		}
		else
		{
			fprintf(file, "%s %d %d 255\n", (image->channels == 1) ? "P5" : "P6", image->width, image->height);
		
			if(fwrite(image->data, image->bytesperline, image->height, file) != image->height)
			{
				#ifdef VC_DEBUG
				fprintf(stderr, "ERROR -> vc_read_image():\n\tError writing PBM, PGM or PPM file.\n");
				#endif

				fclose(file);
				return 0;
			}
		}
		
		fclose(file);

		return 1;
	}
	
	return 0;
}

int vc_gray_negative(IVC* srcdst)
{
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->width * srcdst->channels;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	// Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Inverte a imagem Gray
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos = y * bytesperline + x * channels;

			data[pos] = 255 - data[pos];
		}
	}

	return 1;
}


int vc_rgb_negative(IVC* srcdst) {
	unsigned char* data = (unsigned char*)srcdst->data;
	int width = srcdst->width;
	int height = srcdst->height;
	int bytesperline = srcdst->bytesperline;
	int channels = srcdst->channels;
	int x, y;
	long int pos;

	//Verificação de erros
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	//Inverte a imagem RGB
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;
	
			data[pos] = 255 - data[pos];
			data[pos + 1] = 255 - data[pos + 1];
			data[pos + 2] = 255 - data[pos + 2];
		}
	}

	return 1;
}

int vc_rgb_to_gray(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float rf, gf, bf;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			rf = (float)datasrc[pos_src];
			gf = (float)datasrc[pos_src + 1];
			bf = (float)datasrc[pos_src + 2];

			datadst[pos_dst] = (unsigned char)((rf * 0.299) + (gf * 0.587) + (bf * 0.114));
		}
	}
	return 1;
}

// CONVERTE IMAGEM BGR EM RGB
int vc_bgr_to_rgb(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->bytesperline;
	int bytesperline_dst = dst->bytesperline;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((channels_src != 3) || (channels_dst != 3)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			datadst[pos_dst] = datasrc[pos_src + 2];       // R
			datadst[pos_dst + 1] = datasrc[pos_src + 1];   // G
			datadst[pos_dst + 2] = datasrc[pos_src];       // B
		}
	}

	return 1;
}

// CONVERTE IMAGEM BINÁRIA EM RGB
int vc_binary_to_rgb(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->bytesperline;
	int bytesperline_dst = dst->bytesperline;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int x, y;
	long int pos_src, pos_dst;

	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((dst->width <= 0) || (dst->height <= 0) || (dst->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((channels_src != 1) || (channels_dst != 3)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			datadst[pos_dst] = datasrc[pos_src];
			datadst[pos_dst + 1] = datasrc[pos_src];
			datadst[pos_dst + 2] = datasrc[pos_src];
		}
	}

	return 1;
}


int vc_rgb_to_hsv(IVC* src, IVC* dst) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float r, g, b;
	float max, min, dif;
	float h, s, v;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 3)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			r = (float)datasrc[pos_src];
			g = (float)datasrc[pos_src + 1];
			b = (float)datasrc[pos_src + 2];

			max = (r > g ? (r > b ? r : b) : (g > b ? g : b));
			min = (r < g ? (r < b ? r : b) : (g < b ? g : b));
			dif = max - min;

			v = max;

			if (v == 0 || dif == 0) {
				s = 0;
			}
			else {
				s = dif / v;
			}

			if (dif == 0) {
				h = 0;
			}
			else if (max == r && g >= b) {
				h = (60.0f * (g - b)) / dif;
			}
			else if (max == r && b > g) {
				h = 360.0f + (60.0f * (g - b)) / dif;
			}
			else if (max == g) {
				h = 120.0f + (60.0f * (b - r)) / dif;
			}
			else if (max == b) {
				h = 240.0f + (60.0f * (r - g)) / dif;
			}
			if (h > 360.0f) {
				h = h - 360.0f;
			}

			datadst[pos_dst] = (unsigned char)((h / 360.0f) * 255.0f);
			datadst[pos_dst + 1] = (unsigned char)(s * 255.0f);
			datadst[pos_dst + 2] = (unsigned char)(v);
		}
	}
	return 1;

}

int vc_hsv_segmentation(IVC* src, IVC* dst, int hmin, int hmax, int smin, int smax, int vmin, int vmax) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	int h, s, v;
	long int pos_src, pos_dst;
	int hue_in_range;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	if ((hmin > 360) || (hmin < 0) || (hmax > 360) || (hmax < 0)) return 0;
	if ((smin > 100) || (smin < 0) || (smax > 100) || (smax < 0)) return 0;
	if ((vmin > 100) || (vmin < 0) || (vmax > 100) || (vmax < 0)) return 0;

	hmin = (hmin / 360.0f) * 255.0f;
	hmax = (hmax / 360.0f) * 255.0f;

	smin = (smin / 100.0f) * 255.0f;
	smax = (smax / 100.0f) * 255.0f;                                                 //FALTA CASO DO VERMELHO

	vmin = (vmin / 100.0f) * 255.0f;
	vmax = (vmax / 100.0f) * 255.0f;

	for(y = 0 ; y < height ; y++){
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

		
			h = datasrc[pos_src];
			s = datasrc[pos_src + 1];
			v = datasrc[pos_src + 2];



			/*if (h >= hmin && h <= hmax && s >= smin && s <= smax && v >= vmin && v <= vmax) {
				datadst[pos_dst]=255;
			}
			else {
				datadst[pos_dst] = 0;
			}*/

			if (hmin <= hmax)
				hue_in_range = (h >= hmin && h <= hmax);
			else
				hue_in_range = (h >= hmin || h <= hmax);

			if (hue_in_range && s >= smin && s <= smax && v >= vmin && v <= vmax) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}

		}
	}
	return 1;

}

int vc_count_white(IVC* src, int* white) {
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->width * src->channels;
	int channels = src->channels;
	int x, y;
	long int pos;
	int count = 0;

	//Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x * channels;

			if (data[pos] == 255) count++;
		}
	}
	*white = count;
	return 1;
}

int vc_scale_gray_to_color_palette(IVC* src, IVC* dst) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	float r, g, b;
	float value;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 3)) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;
			
			value = (float)datasrc[pos_src];

			//Azul
			if (value < 64.0f) b = 255.0f;
			else if (value < 128.0f) b = (255.0f - ((value - 64.0f) * 4.0f));
			else b = 0;

			//Verde
			if (value < 64.0f) g =(value * 4.0f);
			else if (value < 192.0f)g = 255.0f;
			else g = (255.0f - (value - 192.0f) * 4.0f);

			//Vermelho
			if (value < 128.0f)r = 0.0f;
			else if (value < 192.0f)r =((value - 128.0f) * 4.0f);
			else r = 255.0f;

			dst->data[pos_dst] = r;
			dst->data[pos_dst+1] = g;
			dst->data[pos_dst+2] = b;
		}
	}
	return 1;
}

int vc_gray_to_binary(IVC* src, IVC* dst, int threshold) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] >= threshold) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}

		}
	}
	return 1;
}

int vc_gray_to_binary_global_mean(IVC* src, IVC* dst) {

	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	int soma = 0;
	int media;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			soma += datasrc[pos_src];
		}
	}

	media = (int)(soma / (width * height));

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_src + x * channels_src;

			if (datasrc[pos_src] >= media) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_midpoint(IVC* src, IVC* dst, int kernel) {

	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y;
	long int pos_src, pos_dst;
	int i, j;
	int min, max, threshold;
	int posk;
	int xx, yy;
	int offset = kernel / 2;

	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;
	if (kernel < 1 || (kernel % 2) == 0) return 0;


	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			min = 255;
			max = 0;

			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {

					xx = x + i;
					yy = y + j;

					if (xx < 0) xx = 0;
					if (yy < 0) yy = 0;
					if (xx >= width) xx = width - 1;
					if (yy >= height) yy = height - 1;

					posk = yy * bytesperline_src + xx * channels_src;

					if (datasrc[posk] < min) min = datasrc[posk];
					if (datasrc[posk] > max) max = datasrc[posk];
				}
			}

			threshold = (min + max) / 2;

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold)
				datadst[pos_dst] = 255;
			else
				datadst[pos_dst] = 0;
		}
	}

	return 1;
}

int vc_gray_to_binary_bersen(IVC* src, IVC* dst, int kernel, int cmin) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, i, j;
	long int pos_src, pos_dst, posk;

	int offset = kernel / 2;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			int vmin = 255;
			int vmax = 0;

			// Percorrer vizinhança
			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {

					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height) {

						posk = yy * bytesperline_src + xx * channels_src;

						if (datasrc[posk] < vmin) vmin = datasrc[posk];
						if (datasrc[posk] > vmax) vmax = datasrc[posk];
					}
				}
			}

			int threshold;

			// Regra Bernsen
			if ((vmax - vmin) < cmin) {
				threshold = 128; // baixo contraste
			}
			else {
				threshold = (vmin + vmax) / 2;
			}

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_gray_to_binary_niblack(IVC* src, IVC* dst, int kernel, float k) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, i, j;
	long int pos_src, pos_dst, posk;

	int offset = kernel / 2;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			int sum = 0;
			int count = 0;
			float mean, stddev;
			float variance = 0;

			
			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {

					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height) {

						posk = yy * bytesperline_src + xx * channels_src;
						sum += datasrc[posk];
						count++;
					}
				}
			}

			mean = (float)sum / count;

			
			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {

					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height) {

						posk = yy * bytesperline_src + xx * channels_src;
						variance += pow(datasrc[posk] - mean, 2);
					}
				}
			}

			variance = variance / count;
			stddev = sqrt(variance);

			// Threshold Niblack
			float threshold = mean + k * stddev;

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (datasrc[pos_src] > threshold) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}
		}
	}
	return 1;
}


int vc_binary_dilate(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, i, j;
	long int pos_dst, posk;

	int offset = kernel / 2;

	// Verificação de erros
	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;
	if ((src->width <= 0) || (src->height <= 0)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;
	if (kernel < 1 || (kernel % 2) == 0) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			int found = 0;

			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {

					int xx = x + i;
					int yy = y + j;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height) {
						posk = yy * bytesperline_src + xx * channels_src;

						if (datasrc[posk] == 255) {
							found = 1;
						}
					}
				}
			}

			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (found == 1) {
				datadst[pos_dst] = 255;
			}
			else {
				datadst[pos_dst] = 0;
			}
		}
	}

	return 1;
}

int vc_binary_erode(IVC* src, IVC* dst, int kernel) {
	unsigned char* datasrc = (unsigned char*)src->data;
	int bytesperline_src = src->width * src->channels;
	int channels_src = src->channels;
	unsigned char* datadst = (unsigned char*)dst->data;
	int bytesperline_dst = dst->width * dst->channels;
	int channels_dst = dst->channels;
	int width = src->width;
	int height = src->height;
	int x, y, i, j;
	long int pos_dst, posk;

	int offset = kernel / 2;

	// Verificação de erros
	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;
	if ((src->width <= 0) || (src->height <= 0)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;
	if (kernel < 1 || (kernel % 2) == 0) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			int found = 0;

			for (j = -offset; j <= offset; j++) {
				for (i = -offset; i <= offset; i++) {

					int xx = x + i;
					int yy = y + j;

					if (xx < 0 || xx >= width || yy < 0 || yy >= height) {   // Se estiver fora atribui preto, se não vai ver os vizinhos
						found = 1;
					}
					else {
						posk = yy * bytesperline_src + xx * channels_src;

						if (datasrc[posk] == 0) {
							found = 1;
						}
					}
				}
			}

			pos_dst = y * bytesperline_dst + x * channels_dst;

			if (found == 1) {
				datadst[pos_dst] = 0;
			}
			else {
				datadst[pos_dst] = 255;
			}
		}
	}

	return 1;
}

int vc_binary_open(IVC* src, IVC* dst, int kernelErode, int kernelDilate) {
	IVC* temp = NULL;


	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;
	if (kernelErode < 1 || (kernelErode % 2) == 0) return 0;
	if (kernelDilate < 1 || (kernelDilate % 2) == 0) return 0;

	temp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if (vc_binary_erode(src, temp, kernelErode) == 0) {
		vc_image_free(temp);
		return 0;
	}

	if (vc_binary_dilate(temp, dst, kernelDilate) == 0) {
		vc_image_free(temp);
		return 0;
	}

	vc_image_free(temp);
	return 1;             
}

int vc_binary_close(IVC* src, IVC* dst, int kernelDilate, int kernelErode) {
	IVC* temp = NULL;


	if ((src == NULL) || (dst == NULL)) return 0;
	if ((src->data == NULL) || (dst->data == NULL)) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;
	if (kernelErode < 1 || (kernelErode % 2) == 0) return 0;
	if (kernelDilate < 1 || (kernelDilate % 2) == 0) return 0;

	temp = vc_image_new(src->width, src->height, src->channels, src->levels);

	if (vc_binary_dilate(src, temp, kernelDilate) == 0) {
		vc_image_free(temp);
		return 0;
	}

	if (vc_binary_erode(temp, dst, kernelErode) == 0) {
		vc_image_free(temp);
		return 0;
	}

	vc_image_free(temp);
	return 1;             // Caso corra tudo bem
}

int vc_binary_subtract(IVC* src1, IVC* src2, IVC* dst) {
	unsigned char* data1 = (unsigned char*)src1->data;
	unsigned char* data2 = (unsigned char*)src2->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int x, y;
	long int pos;
	int width = src1->width;
	int height = src1->height;
	int bytesperline = src1->width * src1->channels;
	
	if ((src1 == NULL) || (src2 == NULL) || (dst == NULL)) return 0;
	if ((src1->data == NULL) || (src2->data == NULL) || (dst->data == NULL)) return 0;
	if ((src1->width != src2->width) || (src1->height != src2->height)) return 0;
	if ((src1->width != dst->width) || (src1->height != dst->height)) return 0;
	if ((src1->channels != 1) || (src2->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pos = y * bytesperline + x;

			if ((data1[pos] == 255) && data2[pos] == 0) {
				datadst[pos] = 255;
			}
			else {
				datadst[pos] = 0;
			}
		}
	}
	return 1;

}





///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////









// Etiquetagem de blobs
// src		: Imagem binária de entrada
// dst		: Imagem grayscale (irá conter as etiquetas)
// nlabels	: Endereço de memória de uma variável, onde será armazenado o número de etiquetas encontradas.
// OVC*		: Retorna um array de estruturas de blobs (objectos), com respectivas etiquetas. É necessário libertar posteriormente esta memória.
OVC* vc_binary_blob_labelling(IVC* src, IVC* dst, int* nlabels)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, a, b;
	long int i, size;
	long int posX, posA, posB, posC, posD;
	int labeltable[256] = { 0 };
	int labelarea[256] = { 0 };
	int label = 1; // Etiqueta inicial.
	int num, tmplabel;
	OVC* blobs; // Apontador para array de blobs (objectos) que será retornado desta função.

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	// Copia dados da imagem binária para imagem grayscale
	memcpy(datadst, datasrc, bytesperline * height);

	// Todos os pixéis de plano de fundo devem obrigatóriamente ter valor 0
	// Todos os pixéis de primeiro plano devem obrigatóriamente ter valor 255
	// Serão atribuídas etiquetas no intervalo [1,254]
	// Este algoritmo está assim limitado a 254 labels
	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	// Limpa os rebordos da imagem binária
	for (y = 0; y < height; y++)
	{
		datadst[y * bytesperline + 0 * channels] = 0;
		datadst[y * bytesperline + (width - 1) * channels] = 0;
	}
	for (x = 0; x < width; x++)
	{
		datadst[0 * bytesperline + x * channels] = 0;
		datadst[(height - 1) * bytesperline + x * channels] = 0;
	}

	// Efectua a etiquetagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Kernel:
			// A B C
			// D X

			posA = (y - 1) * bytesperline + (x - 1) * channels; // A
			posB = (y - 1) * bytesperline + x * channels; // B
			posC = (y - 1) * bytesperline + (x + 1) * channels; // C
			posD = y * bytesperline + (x - 1) * channels; // D
			posX = y * bytesperline + x * channels; // X

			// Se o pixel foi marcado
			if (datadst[posX] != 0)
			{
				if ((datadst[posA] == 0) && (datadst[posB] == 0) && (datadst[posC] == 0) && (datadst[posD] == 0))
				{
					datadst[posX] = label;
					labeltable[label] = label;
					label++;
				}
				else
				{
					num = 255;

					// Se A está marcado
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					// Se B está marcado, e é menor que a etiqueta "num"
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					// Se C está marcado, e é menor que a etiqueta "num"
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					// Se D está marcado, e é menor que a etiqueta "num"
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					// Atribui a etiqueta ao pixel
					datadst[posX] = num;
					labeltable[num] = num;

					// Actualiza a tabela de etiquetas
					if (datadst[posA] != 0)
					{
						if (labeltable[datadst[posA]] != num)
						{
							for (tmplabel = labeltable[datadst[posA]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posB] != 0)
					{
						if (labeltable[datadst[posB]] != num)
						{
							for (tmplabel = labeltable[datadst[posB]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posC] != 0)
					{
						if (labeltable[datadst[posC]] != num)
						{
							for (tmplabel = labeltable[datadst[posC]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
					if (datadst[posD] != 0)
					{
						if (labeltable[datadst[posD]] != num)
						{
							for (tmplabel = labeltable[datadst[posD]], a = 1; a < label; a++)
							{
								if (labeltable[a] == tmplabel)
								{
									labeltable[a] = num;
								}
							}
						}
					}
				}
			}
		}
	}

	// Volta a etiquetar a imagem
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; // X

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	//printf("\nMax Label = %d\n", label);

	// Contagem do número de blobs
	// Passo 1: Eliminar, da tabela, etiquetas repetidas
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	// Passo 2: Conta etiquetas e organiza a tabela de etiquetas, para que não hajam valores vazios (zero) entre etiquetas
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; // Organiza tabela de etiquetas
			(*nlabels)++; // Conta etiquetas
		}
	}

	// Se não há blobs
	if (*nlabels == 0) return NULL;

	// Cria lista de blobs (objectos) e preenche a etiqueta
	blobs = (OVC*)calloc((*nlabels), sizeof(OVC));
	if (blobs != NULL)
	{
		for (a = 0; a < (*nlabels); a++) blobs[a].label = labeltable[a];
	}
	else return NULL;

	return blobs;
}


int vc_binary_blob_info(IVC* src, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)src->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y, i;
	long int pos;
	int xmin, ymin, xmax, ymax;
	long int sumx, sumy;

	// Verificação de erros
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	// Conta área de cada blob
	for (i = 0; i < nblobs; i++)
	{
		xmin = width - 1;
		ymin = height - 1;
		xmax = 0;
		ymax = 0;

		sumx = 0;
		sumy = 0;

		blobs[i].area = 0;

		for (y = 1; y < height - 1; y++)
		{
			for (x = 1; x < width - 1; x++)
			{
				pos = y * bytesperline + x * channels;

				if (data[pos] == blobs[i].label)
				{
					// Área
					blobs[i].area++;

					// Centro de Gravidade
					sumx += x;
					sumy += y;

					// Bounding Box
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					// Perímetro
					// Se pelo menos um dos quatro vizinhos não pertence ao mesmo label, então é um pixel de contorno
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		// Bounding Box
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		// Centro de Gravidade
		//blobs[i].xc = (xmax - xmin) / 2;
		//blobs[i].yc = (ymax - ymin) / 2;
		blobs[i].xc = sumx / MAX(blobs[i].area, 1);
		blobs[i].yc = sumy / MAX(blobs[i].area, 1);
	}

	return 1;
}

int vc_draw_bounding_box(IVC* image, OVC* blobs, int nblobs)
{
	unsigned char* data = (unsigned char*)image->data;
	int width = image->width;
	int height = image->height;
	int bytesperline = image->bytesperline;
	int channels = image->channels;
	int i, x, y;
	long int pos;

	// Verificação de erros
	if ((image == NULL) || (blobs == NULL)) return 0;
	if ((image->data == NULL) || (nblobs <= 0)) return 0;
	if ((channels != 1) && (channels != 3)) return 0;

	for (i = 0; i < nblobs; i++)
	{
		int xmin = blobs[i].x;
		int ymin = blobs[i].y;
		int xmax = blobs[i].x + blobs[i].width - 1;
		int ymax = blobs[i].y + blobs[i].height - 1;

		if (xmin < 0) xmin = 0;
		if (ymin < 0) ymin = 0;
		if (xmax >= width) xmax = width - 1;
		if (ymax >= height) ymax = height - 1;

		
		for (x = xmin; x <= xmax; x++)
		{
			
			pos = ymin * bytesperline + x * channels; //LINHA DE CIMA
			if (channels == 1)
			{
				data[pos] = 255;
			}
			else
			{
				data[pos] = 255;     
				data[pos + 1] = 0;  
				data[pos + 2] = 0;   
			}

			
			pos = ymax * bytesperline + x * channels;  //LINHA DE BAIXO
			if (channels == 1)
			{
				data[pos] = 255;
			}
			else
			{
				data[pos] = 255;
				data[pos + 1] = 0;
				data[pos + 2] = 0;
			}
		}

		// Linha da esquerda e da direita
		for (y = ymin; y <= ymax; y++)
		{
			
			pos = y * bytesperline + xmin * channels;  //LINHA DA ESQUERDA
			if (channels == 1)
			{
				data[pos] = 255;
			}
			else
			{
				data[pos] = 255;
				data[pos + 1] = 0;
				data[pos + 2] = 0;
			}

			
			pos = y * bytesperline + xmax * channels;  //LINHA DA DIREITA
			if (channels == 1)
			{
				data[pos] = 255;
			}
			else
			{
				data[pos] = 255;
				data[pos + 1] = 0;
				data[pos + 2] = 0;
			}
		}
	}

	return 1;
}


int vc_draw_center_mass(IVC* image, OVC* blobs, int nblobs, int size)
{
	unsigned char* data = (unsigned char*)image->data;
	int width = image->width;
	int height = image->height;
	int bytesperline = image->bytesperline;
	int channels = image->channels;
	int i, k;
	long int pos;
	int x, y;

	// Verificação de erros
	if ((image == NULL) || (blobs == NULL)) return 0;
	if (image->data == NULL) return 0;
	if (nblobs <= 0) return 0;
	if ((image->channels != 1) && (image->channels != 3)) return 0;
	if (size < 1) return 0;


	for (i = 0; i < nblobs; i++)
	{
		int xc = blobs[i].xc;
		int yc = blobs[i].yc;

		for (k = -size; k <= size; k++)
		{
			// Linha horizontal
			x = xc + k;
			y = yc;

			if ((x >= 0) && (x < width) && (y >= 0) && (y < height))
			{
				pos = y * bytesperline + x * channels;

				if (channels == 1)
				{
					data[pos] = 0;
				}
				else
				{
					data[pos] = 0;
					data[pos + 1] = 255;
					data[pos + 2] = 0;
				}
			}

			// Linha vertical
			x = xc;
			y = yc + k;

			if ((x >= 0) && (x < width) && (y >= 0) && (y < height))
			{
				pos = y * bytesperline + x * channels;

				if (channels == 1)
				{
					data[pos] = 0;
				}
				else
				{
					data[pos] = 0;
					data[pos + 1] = 255;
					data[pos + 2] = 0;
				}
			}
		}
	}

	return 1;
}

int vc_gray_histogram_equalization(IVC* src, IVC* dst)
{
	int i;
	long int pixels;
	int hist[256] = { 0 };    
	float pdf[256] = { 0 };     
	float cdf[256] = { 0 };
	int l;
	unsigned char* datasrc, * datadst;

	
	if (src == NULL || dst == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	pixels = src->width * src->height;
	datasrc = src->data;
	datadst = dst->data;

	l = src->levels;
	
	for (i = 0; i < pixels; i++)
	{
		hist[datasrc[i]]++;
	}
	
	for (i = 0; i < l; i++)
	{
		pdf[i] = (float)hist[i] / pixels;
	}

	cdf[0] = pdf[0];
	for (i = 1; i < l; i++)
	{
		cdf[i] = cdf[i - 1] + pdf[i];
	}

	float cdf_min = 0.0f;

	for (i = 0; i < l; i++)     // o cdf_min é o primeiro valor maior que 0
	{
		if (cdf[i] > 0.0f)
		{
			cdf_min = cdf[i];
			break;
		}
	}
	
	for (i = 0; i < pixels; i++)
	{
		float div = (cdf[datasrc[i]] - cdf_min) / (1.0f - cdf_min);
		datadst[i] = (unsigned char)(div * (l - 1));
	}

	return 1;
}

int vc_gray_edge_prewitt(IVC* src, IVC* dst, float th)
{
	int x, y, i;
	int width, height;
	int gx, gy;
	float magn;
	long int size;
	unsigned char* datasrc, * datadst;

	if (src == NULL || dst == NULL) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	width = src->width;
	height = src->height;
	datasrc = src->data;
	datadst = dst->data;
	size = width * height;

	// Percorrer imagem (evitar bordas)
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			i = y * width + x;

			// Gradiente em X (Gx)
			gx =
				-datasrc[(y - 1) * width + (x - 1)] + datasrc[(y - 1) * width + (x + 1)] +
				-datasrc[(y)*width + (x - 1)] + datasrc[(y)*width + (x + 1)] +
				-datasrc[(y + 1) * width + (x - 1)] + datasrc[(y + 1) * width + (x + 1)];

			// Gradiente em Y (Gy)
			gy =
				-datasrc[(y - 1) * width + (x - 1)] - datasrc[(y - 1) * width + (x)] - datasrc[(y - 1) * width + (x + 1)] +
				datasrc[(y + 1) * width + (x - 1)] + datasrc[(y + 1) * width + (x)] + datasrc[(y + 1) * width + (x + 1)];

			// Magnitude do gradiente
			magn = (float)sqrt((gx * gx) + (gy * gy));

			// Aplicar threshold
			if (magn > (th * 255))
				datadst[i] = 255; // contorno
			else
				datadst[i] = 0;   // fundo
		}
	}

	return 1;
}

int vc_gray_edge_sobel(IVC* src, IVC* dst, float th)  // ANALISAR MELHOR ESTA FUNÇÃO
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int pos;
	int gx, gy;
	float magn;

	// Verificações
	if (src == NULL || dst == NULL) return 0;
	if (src->data == NULL || dst->data == NULL) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	// Percorrer imagem (evitar bordas)
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			pos = y * bytesperline + x * channels;

			// Sobel Gx
			gx =
				-datasrc[(y - 1) * bytesperline + (x - 1) * channels] +
				datasrc[(y - 1) * bytesperline + (x + 1) * channels] +
				-2 * datasrc[(y)*bytesperline + (x - 1) * channels] +
				2 * datasrc[(y)*bytesperline + (x + 1) * channels] +
				-datasrc[(y + 1) * bytesperline + (x - 1) * channels] +
				datasrc[(y + 1) * bytesperline + (x + 1) * channels];

			
			gy =
				-datasrc[(y - 1) * bytesperline + (x - 1) * channels] -
				2 * datasrc[(y - 1) * bytesperline + (x)*channels] -
				datasrc[(y - 1) * bytesperline + (x + 1) * channels] +
				datasrc[(y + 1) * bytesperline + (x - 1) * channels] +
				2 * datasrc[(y + 1) * bytesperline + (x)*channels] +
				datasrc[(y + 1) * bytesperline + (x + 1) * channels];

			
			magn = (float)sqrt((gx * gx) + (gy * gy));

			
			if (magn > (th * 255))
				datadst[pos] = 255;
			else
				datadst[pos] = 0;
		}
	}

	return 1;
}

int vc_gray_lowpass_mean_filter(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->bytesperline;
	int bytesperline_dst = dst->bytesperline;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int x, y, i, j;
	long int pos_src, pos_dst, posk;
	int sum, count;
	int offset;

	if (src == NULL || dst == NULL) return 0;
	if (src->data == NULL || dst->data == NULL) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;
	if (kernelsize < 1 || (kernelsize % 2) == 0) return 0;

	offset = kernelsize / 2;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0;
			count = 0;


			for (j = -offset; j <= offset; j++)
			{
				for (i = -offset; i <= offset; i++)
				{
					int xx = x + i;
					int yy = y + j;

					// bordas
					if (xx < 0) xx = 0;
					if (yy < 0) yy = 0;
					if (xx >= width) xx = width - 1;
					if (yy >= height) yy = height - 1;

					posk = yy * bytesperline_src + xx * channels_src;

					sum += datasrc[posk];
					count++;
				}
			}

			pos_dst = y * bytesperline_dst + x * channels_dst;

			datadst[pos_dst] = (unsigned char)(sum / count);
		}
	}

	return 1;
}

int vc_gray_lowpass_median_filter(IVC* src, IVC* dst, int kernelsize)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline_src = src->bytesperline;
	int bytesperline_dst = dst->bytesperline;
	int channels_src = src->channels;
	int channels_dst = dst->channels;
	int x, y, i, j, k;
	long int pos_dst, posk;
	int offset;
	int size;
	unsigned char* kernelvalues;
	unsigned char temp;

	// Verificações
	if (src == NULL || dst == NULL) return 0;
	if (src->data == NULL || dst->data == NULL) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;
	if (kernelsize < 1 || (kernelsize % 2) == 0) return 0;

	offset = kernelsize / 2;
	size = kernelsize * kernelsize;

	kernelvalues = (unsigned char*)malloc(size * sizeof(unsigned char));
	if (kernelvalues == NULL) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			int index = 0;

			// Preencher vetor com vizinhança
			for (j = -offset; j <= offset; j++)
			{
				for (i = -offset; i <= offset; i++)
				{
					int xx = x + i;
					int yy = y + j;

					// Clamp bordas
					if (xx < 0) xx = 0;
					if (yy < 0) yy = 0;
					if (xx >= width) xx = width - 1;
					if (yy >= height) yy = height - 1;

					posk = yy * bytesperline_src + xx * channels_src;

					kernelvalues[index++] = datasrc[posk];
				}
			}

			
			for (i = 0; i < size - 1; i++)
			{
				for (j = i + 1; j < size; j++)
				{
					if (kernelvalues[i] > kernelvalues[j])
					{
						temp = kernelvalues[i];
						kernelvalues[i] = kernelvalues[j];
						kernelvalues[j] = temp;
					}
				}
			}

			// Mediana
			pos_dst = y * bytesperline_dst + x * channels_dst;
			datadst[pos_dst] = kernelvalues[size / 2];
		}
	}

	free(kernelvalues);

	return 1;
}

int vc_gray_lowpass_gaussian_filter(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	int x, y, kx, ky;
	long int pos, posk;

	int offset = 2;
	int sum;

	// Kernel do slide (5x5)
	int kernel[5][5] = {
		{1, 4, 7, 4, 1},
		{4,16,26,16, 4},
		{7,26,41,26, 7},
		{4,16,26,16, 4},
		{1, 4, 7, 4, 1}
	};

	// Soma dos pesos = 273
	int weight = 273;

	// Verificações
	if ((src == NULL) || (dst == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if ((channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					int xx = x + kx;
					int yy = y + ky;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						posk = yy * bytesperline + xx * channels;
						sum += datasrc[posk] * kernel[ky + offset][kx + offset];
					}
				}
			}

			pos = y * bytesperline + x * channels;
			datadst[pos] = (unsigned char)(sum / weight);
		}
	}

	return 1;
}

int vc_gray_highpass_filter(IVC* src, IVC* dst)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;

	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;

	int x, y, kx, ky;
	long int pos, posk;

	int kernel[3][3] = {
		{-1, -1, -1},
		{-1,  8, -1},
		{-1, -1, -1}
	};

	int offset = 1;
	int sum;

	// Verificações
	if ((src == NULL) || (dst == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if ((channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			sum = 0;

			for (ky = -offset; ky <= offset; ky++)
			{
				for (kx = -offset; kx <= offset; kx++)
				{
					int xx = x + kx;
					int yy = y + ky;

					if (xx >= 0 && xx < width && yy >= 0 && yy < height)
					{
						posk = yy * bytesperline + xx * channels;
						sum += datasrc[posk] * kernel[ky + offset][kx + offset];
					}
				}
			}

			pos = y * bytesperline + x * channels;

			// Clamp para [0,255]
			if (sum < 0) sum = 0;
			if (sum > 255) sum = 255;

			datadst[pos] = (unsigned char)sum;
		}
	}

	return 1;
}

int vc_gray_highpass_filter_enhance(IVC* src, IVC* dst, int gain)
{
	unsigned char* datasrc = (unsigned char*)src->data;
	unsigned char* datadst = (unsigned char*)dst->data;
	int width = src->width;
	int height = src->height;
	int bytesperline = src->bytesperline;
	int channels = src->channels;
	int x, y;
	long int posA, posB, posC, posD, posX, posE, posF, posG, posH;
	int laplace, value;

	// Verificações
	if ((src == NULL) || (dst == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if (channels != 1 || dst->channels != 1) return 0;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			// Vizinhança 3x3
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			// Máscara Laplaciana (8 vizinhos)
			// -1 -1 -1
			// -1  8 -1
			// -1 -1 -1

			laplace =
				(-datasrc[posA] - datasrc[posB] - datasrc[posC]
					- datasrc[posD] + 8 * datasrc[posX] - datasrc[posE]
					- datasrc[posF] - datasrc[posG] - datasrc[posH]);

			// Realce: original + ganho * laplaciano
			value = datasrc[posX] + gain * laplace;

			// Clamp para [0,255]
			if (value < 0) value = 0;
			if (value > 255) value = 255;

			datadst[posX] = (unsigned char)value;
		}
	}

	return 1;
}

int vc_image_diff(IVC* current, IVC* prev, IVC* diff)
{
	unsigned char* datacurrent = (unsigned char*)current->data;
	unsigned char* dataprev = (unsigned char*)prev->data;
	unsigned char* datadiff = (unsigned char*)diff->data;
	int width, height, channels;
	int x, y, c;
	long int pos;

	if (current == NULL || prev == NULL || diff == NULL) return 0;
	if (current->data == NULL || prev->data == NULL || diff->data == NULL) return 0;
	if (current->width != prev->width || current->width != diff->width) return 0;
	if (current->height != prev->height || current->height != diff->height) return 0;
	if (current->channels != prev->channels || current->channels != diff->channels) return 0;

	width = current->width;
	height = current->height;
	channels = current->channels;

	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			for (c = 0; c < channels; c++)
			{
				pos = y * current->bytesperline + x * channels + c;

				datadiff[pos] = abs((int)datacurrent[pos] - (int)dataprev[pos]);
			}
		}
	}

	return 1;
}

int obter_calibre(float diametro_mm)
{
	if (diametro_mm >= 100) return 0;
	else if (diametro_mm >= 87) return 1;
	else if (diametro_mm >= 84) return 2;
	else if (diametro_mm >= 81) return 3;
	else if (diametro_mm >= 77) return 4;
	else if (diametro_mm >= 73) return 5;
	else if (diametro_mm >= 70) return 6;
	else if (diametro_mm >= 67) return 7;
	else if (diametro_mm >= 64) return 8;
	else if (diametro_mm >= 62) return 9;
	else if (diametro_mm >= 60) return 10;
	else if (diametro_mm >= 58) return 11;
	else if (diametro_mm >= 56) return 12;
	else if (diametro_mm >= 53) return 13;
	else return -1;
}