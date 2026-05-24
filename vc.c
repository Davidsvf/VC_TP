
#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include "vc.h"





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
	
	
	if((file = fopen(filename, "rb")) != NULL)
	{
		
		netpbm_get_token(file, tok, sizeof(tok));

		if(strcmp(tok, "P4") == 0) { channels = 1; levels = 1; }	
		else if(strcmp(tok, "P5") == 0) channels = 1;				
		else if(strcmp(tok, "P6") == 0) channels = 3;				
		else
		{
			#ifdef VC_DEBUG
			printf("ERROR -> vc_read_image():\n\tFile is not a valid PBM, PGM or PPM file.\n\tBad magic number!\n");
			#endif

			fclose(file);
			return NULL;
		}
		
		if(levels == 1) 
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
		else 
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

	
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 1) return 0;

	
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

	
	if ((srcdst->width <= 0) || (srcdst->height <= 0) || (srcdst->data == NULL)) return 0;
	if (channels != 3) return 0;

	
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

			datadst[pos_dst] = datasrc[pos_src + 2];       
			datadst[pos_dst + 1] = datasrc[pos_src + 1];   
			datadst[pos_dst + 2] = datasrc[pos_src];       
		}
	}

	return 1;
}


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

	
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 3) || (dst->channels != 1)) return 0;

	if ((hmin > 360) || (hmin < 0) || (hmax > 360) || (hmax < 0)) return 0;
	if ((smin > 100) || (smin < 0) || (smax > 100) || (smax < 0)) return 0;
	if ((vmin > 100) || (vmin < 0) || (vmax > 100) || (vmax < 0)) return 0;

	hmin = (hmin / 360.0f) * 255.0f;
	hmax = (hmax / 360.0f) * 255.0f;

	smin = (smin / 100.0f) * 255.0f;
	smax = (smax / 100.0f) * 255.0f;                                                 

	vmin = (vmin / 100.0f) * 255.0f;
	vmax = (vmax / 100.0f) * 255.0f;

	for(y = 0 ; y < height ; y++){
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;

		
			h = datasrc[pos_src];
			s = datasrc[pos_src + 1];
			v = datasrc[pos_src + 2];


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

	
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 3)) return 0;

	for (y = 0; y < src->height; y++) {
		for (x = 0; x < width; x++) {

			pos_src = y * bytesperline_src + x * channels_src;
			pos_dst = y * bytesperline_dst + x * channels_dst;
			
			value = (float)datasrc[pos_src];

			
			if (value < 64.0f) b = 255.0f;
			else if (value < 128.0f) b = (255.0f - ((value - 64.0f) * 4.0f));
			else b = 0;

			
			if (value < 64.0f) g =(value * 4.0f);
			else if (value < 192.0f)g = 255.0f;
			else g = (255.0f - (value - 192.0f) * 4.0f);

			
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

	
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height)) return 0;
	if ((src->channels != 1) || (dst->channels != 1)) return 0;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {

			int vmin = 255;
			int vmax = 0;

			
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

			
			if ((vmax - vmin) < cmin) {
				threshold = 128; 
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

					if (xx < 0 || xx >= width || yy < 0 || yy >= height) {   
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
	return 1;             
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
	int label = 1; 
	int num, tmplabel;
	OVC* blobs; 

	
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if ((src->width != dst->width) || (src->height != dst->height) || (src->channels != dst->channels)) return NULL;
	if (channels != 1) return NULL;

	
	memcpy(datadst, datasrc, bytesperline * height);


	for (i = 0, size = bytesperline * height; i < size; i++)
	{
		if (datadst[i] != 0) datadst[i] = 255;
	}

	
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

	
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			

			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels; 
			posC = (y - 1) * bytesperline + (x + 1) * channels; 
			posD = y * bytesperline + (x - 1) * channels; 
			posX = y * bytesperline + x * channels; 

			
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

					
					if (datadst[posA] != 0) num = labeltable[datadst[posA]];
					
					if ((datadst[posB] != 0) && (labeltable[datadst[posB]] < num)) num = labeltable[datadst[posB]];
					
					if ((datadst[posC] != 0) && (labeltable[datadst[posC]] < num)) num = labeltable[datadst[posC]];
					
					if ((datadst[posD] != 0) && (labeltable[datadst[posD]] < num)) num = labeltable[datadst[posD]];

					
					datadst[posX] = num;
					labeltable[num] = num;

					
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

	
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			posX = y * bytesperline + x * channels; 

			if (datadst[posX] != 0)
			{
				datadst[posX] = labeltable[datadst[posX]];
			}
		}
	}

	
	for (a = 1; a < label - 1; a++)
	{
		for (b = a + 1; b < label; b++)
		{
			if (labeltable[a] == labeltable[b]) labeltable[b] = 0;
		}
	}
	
	*nlabels = 0;
	for (a = 1; a < label; a++)
	{
		if (labeltable[a] != 0)
		{
			labeltable[*nlabels] = labeltable[a]; 
			(*nlabels)++;
		}
	}

	
	if (*nlabels == 0) return NULL;

	
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

	
	if ((src->width <= 0) || (src->height <= 0) || (src->data == NULL)) return 0;
	if (channels != 1) return 0;

	
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
					
					blobs[i].area++;

					
					sumx += x;
					sumy += y;

					
					if (xmin > x) xmin = x;
					if (ymin > y) ymin = y;
					if (xmax < x) xmax = x;
					if (ymax < y) ymax = y;

					
					if ((data[pos - 1] != blobs[i].label) || (data[pos + 1] != blobs[i].label) || (data[pos - bytesperline] != blobs[i].label) || (data[pos + bytesperline] != blobs[i].label))
					{
						blobs[i].perimeter++;
					}
				}
			}
		}

		
		blobs[i].x = xmin;
		blobs[i].y = ymin;
		blobs[i].width = (xmax - xmin) + 1;
		blobs[i].height = (ymax - ymin) + 1;

		
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
			
			pos = ymin * bytesperline + x * channels; 
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

			
			pos = ymax * bytesperline + x * channels;  
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

		
		for (y = ymin; y <= ymax; y++)
		{
			
			pos = y * bytesperline + xmin * channels;  
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

			
			pos = y * bytesperline + xmax * channels; 
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

	for (i = 0; i < l; i++)     
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

	
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			i = y * width + x;

			
			gx =
				-datasrc[(y - 1) * width + (x - 1)] + datasrc[(y - 1) * width + (x + 1)] +
				-datasrc[(y)*width + (x - 1)] + datasrc[(y)*width + (x + 1)] +
				-datasrc[(y + 1) * width + (x - 1)] + datasrc[(y + 1) * width + (x + 1)];

			
			gy =
				-datasrc[(y - 1) * width + (x - 1)] - datasrc[(y - 1) * width + (x)] - datasrc[(y - 1) * width + (x + 1)] +
				datasrc[(y + 1) * width + (x - 1)] + datasrc[(y + 1) * width + (x)] + datasrc[(y + 1) * width + (x + 1)];

			
			magn = (float)sqrt((gx * gx) + (gy * gy));

			
			if (magn > (th * 255))
				datadst[i] = 255; 
			else
				datadst[i] = 0;   
		}
	}

	return 1;
}

int vc_gray_edge_sobel(IVC* src, IVC* dst, float th)  
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

	
	if (src == NULL || dst == NULL) return 0;
	if (src->data == NULL || dst->data == NULL) return 0;
	if (src->width <= 0 || src->height <= 0) return 0;
	if (src->width != dst->width || src->height != dst->height) return 0;
	if (src->channels != 1 || dst->channels != 1) return 0;

	
	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			pos = y * bytesperline + x * channels;

			
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

			
			for (j = -offset; j <= offset; j++)
			{
				for (i = -offset; i <= offset; i++)
				{
					int xx = x + i;
					int yy = y + j;

					
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

	
	int kernel[5][5] = {
		{1, 4, 7, 4, 1},
		{4,16,26,16, 4},
		{7,26,41,26, 7},
		{4,16,26,16, 4},
		{1, 4, 7, 4, 1}
	};

	
	int weight = 273;

	
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

	
	if ((src == NULL) || (dst == NULL)) return 0;
	if ((width != dst->width) || (height != dst->height)) return 0;
	if (channels != 1 || dst->channels != 1) return 0;

	for (y = 1; y < height - 1; y++)
	{
		for (x = 1; x < width - 1; x++)
		{
			
			posA = (y - 1) * bytesperline + (x - 1) * channels;
			posB = (y - 1) * bytesperline + x * channels;
			posC = (y - 1) * bytesperline + (x + 1) * channels;
			posD = y * bytesperline + (x - 1) * channels;
			posX = y * bytesperline + x * channels;
			posE = y * bytesperline + (x + 1) * channels;
			posF = (y + 1) * bytesperline + (x - 1) * channels;
			posG = (y + 1) * bytesperline + x * channels;
			posH = (y + 1) * bytesperline + (x + 1) * channels;

			

			laplace =
				(-datasrc[posA] - datasrc[posB] - datasrc[posC]
					- datasrc[posD] + 8 * datasrc[posX] - datasrc[posE]
					- datasrc[posF] - datasrc[posG] - datasrc[posH]);

			
			value = datasrc[posX] + gain * laplace;

			
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





int vc_laranja_completa_na_imagem(OVC blob, int width, int height, int margem)
{
	if (blob.x <= margem) return 0;
	if (blob.y <= margem) return 0;

	if ((blob.x + blob.width) >= width - margem) return 0;
	if ((blob.y + blob.height) >= height - margem) return 0;

	return 1;
}

int vc_obter_calibre(float diametro_mm)
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

int vc_homogeneidade_obter_grupo_calibre(int calibre)
{
	if (calibre >= 0 && calibre <= 2) return VC_GRUPO_CALIBRE_0_2;
	if (calibre >= 3 && calibre <= 6) return VC_GRUPO_CALIBRE_3_6;
	if (calibre >= 7 && calibre <= 13) return VC_GRUPO_CALIBRE_7_13;

	return -1;
}

int vc_homogeneidade_obter_limite_grupo(int grupo)
{
	if (grupo == VC_GRUPO_CALIBRE_0_2) return 11;
	if (grupo == VC_GRUPO_CALIBRE_3_6) return 9;
	if (grupo == VC_GRUPO_CALIBRE_7_13) return 7;

	return 0;
}

void vc_homogeneidade_inicializar(float minDiam[], float maxDiam[], int countGrupo[])
{
	int i;

	for (i = 0; i < VC_N_GRUPOS_CALIBRE; i++)
	{
		minDiam[i] = 9999.0f;
		maxDiam[i] = 0.0f;
		countGrupo[i] = 0;
	}
}

void vc_homogeneidade_atualizar(float diametro_mm, int calibre, float minDiam[], float maxDiam[], int countGrupo[])
{
	int grupo = vc_homogeneidade_obter_grupo_calibre(calibre);

	if (grupo == -1) return;

	if (diametro_mm < minDiam[grupo])
	{
		minDiam[grupo] = diametro_mm;
	}

	if (diametro_mm > maxDiam[grupo])
	{
		maxDiam[grupo] = diametro_mm;
	}

	countGrupo[grupo]++;
}

int vc_homogeneidade_verificar(float minDiam[], float maxDiam[], int countGrupo[], float* maiorDiferenca)
{
	int grupo;
	int homogeneidade = 1;

	*maiorDiferenca = 0.0f;

	for (grupo = 0; grupo < VC_N_GRUPOS_CALIBRE; grupo++)
	{
		if (countGrupo[grupo] >= 2)
		{
			float diferenca = maxDiam[grupo] - minDiam[grupo];

			if (diferenca > *maiorDiferenca)
			{
				*maiorDiferenca = diferenca;
			}

			if (diferenca > vc_homogeneidade_obter_limite_grupo(grupo))
			{
				homogeneidade = 0;
			}
		}
	}

	return homogeneidade;
}

int vc_encontrar_laranja(LARANJA_INFO laranjas[], int total, int xc, int yc)
{
	for (int i = 0; i < total; i++)
	{
		int dx = laranjas[i].xc - xc;
		int dy = laranjas[i].yc - yc;

		float distancia = sqrt((float)(dx * dx + dy * dy));

		if (distancia < DISTANCIA_MESMA_LARANJA)
		{
			return i;
		}
	}

	return -1;
}

float vc_detetar_defeitos(IVC* hsv, OVC blob, int kernel, int c)
{
	int x, y, xx, yy;
	int offset;
	long int pos_hsv, posk;
	long int soma;
	int count;
	int media;
	float raio, raio_util;
	float dx, dy;
	unsigned char v;
	int total_pixels = 0;
	int total_defeitos = 0;

	if (hsv == NULL) return 0.0f;
	if (hsv->data == NULL) return 0.0f;
	if (hsv->channels != 3) return 0.0f;
	if (kernel < 3 || (kernel % 2) == 0) return 0.0f;

	offset = kernel / 2;

	raio = (blob.width + blob.height) / 4.0f;
	raio_util = raio * 0.95f;

	for (y = blob.y; y < blob.y + blob.height; y++)
	{
		for (x = blob.x; x < blob.x + blob.width; x++)
		{
			if (x < 0 || y < 0 || x >= hsv->width || y >= hsv->height)
				continue;

			dx = (float)(x - blob.xc);
			dy = (float)(y - blob.yc);

			if ((dx * dx + dy * dy) > (raio_util * raio_util))
				continue;

			pos_hsv = y * hsv->bytesperline + x * hsv->channels;
			v = hsv->data[pos_hsv + 2];

			soma = 0;
			count = 0;

			for (yy = y - offset; yy <= y + offset; yy++)
			{
				for (xx = x - offset; xx <= x + offset; xx++)
				{
					if (xx < 0 || yy < 0 || xx >= hsv->width || yy >= hsv->height)
						continue;

					posk = yy * hsv->bytesperline + xx * hsv->channels;
					soma += hsv->data[posk + 2];
					count++;
				}
			}

			if (count == 0)
				continue;

			media = (int)(soma / count);

			total_pixels++;

			if (v < media - c)
				total_defeitos++;
				
		}
	}

	if (total_pixels == 0) return 0.0f;

	return ((float)total_defeitos / (float)total_pixels) * 100.0f;
}

int vc_obter_categoria(float percent_defeitos)
{
	if (percent_defeitos <= 0.5f)
		return 0;

	if (percent_defeitos <= 3.0f)
		return 1;

	if (percent_defeitos <= 8.0f)
		return 2;

	return 3;
}