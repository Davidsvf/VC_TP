#include <iostream>
#include <string>
#include <chrono>
#include <cmath>
#include <opencv2\opencv.hpp>
#include <opencv2\core.hpp>
#include <opencv2\highgui.hpp>
#include <opencv2\videoio.hpp>

extern "C" {
#include "vc.h"
}

void vc_timer(void) {
	static bool running = false;
	static std::chrono::steady_clock::time_point previousTime = std::chrono::steady_clock::now();

	if (!running) {
		running = true;
	}
	else {
		auto currentTime = std::chrono::steady_clock::now();
		std::chrono::duration<double> time_span = currentTime - previousTime;

		std::cout << "Tempo decorrido: " << time_span.count() << " segundos" << std::endl;
		std::cout << "Pressione qualquer tecla para continuar...\n";
		std::cin.get();
	}
}

int main(void) {
	char videofile[20] = "video.avi";
	cv::VideoCapture capture;

	struct
	{
		int width, height;
		int ntotalframes;
		int fps;
		int nframe;
	} video;

	std::string str;
	int key = 0;

	capture.open(videofile);

	if (!capture.isOpened())
	{
		std::cerr << "Erro ao abrir o ficheiro de vídeo!\n";
		return 1;
	}

	video.ntotalframes = (int)capture.get(cv::CAP_PROP_FRAME_COUNT);
	video.fps = (int)capture.get(cv::CAP_PROP_FPS);
	video.width = (int)capture.get(cv::CAP_PROP_FRAME_WIDTH);
	video.height = (int)capture.get(cv::CAP_PROP_FRAME_HEIGHT);

	cv::namedWindow("VC-VIDEO", cv::WINDOW_AUTOSIZE);

	vc_timer();

	cv::Mat frame;

	IVC* image = vc_image_new(video.width, video.height, 3, 255);
	IVC* rgb = vc_image_new(video.width, video.height, 3, 255);
	IVC* hsv = vc_image_new(video.width, video.height, 3, 255);

	IVC* seg_laranja = vc_image_new(video.width, video.height, 1, 255);
	IVC* seg_temp = vc_image_new(video.width, video.height, 1, 255);
	IVC* labels = vc_image_new(video.width, video.height, 1, 255);

	int total_laranjas = 0;
	float escala_mm = 55.0f / 280.0f;

	LARANJA_INFO laranjas[MAX_LARANJAS];

	for (int i = 0; i < MAX_LARANJAS; i++)
	{
		laranjas[i].ativo = 0;
		laranjas[i].xc = 0;
		laranjas[i].yc = 0;
		laranjas[i].melhor_area = 0.0f;
		laranjas[i].melhor_perimetro = 0.0f;
		laranjas[i].melhor_diametro = 0.0f;
		laranjas[i].melhor_calibre = -1;
	}

	float minDiamGlobal[VC_N_GRUPOS_CALIBRE];
	float maxDiamGlobal[VC_N_GRUPOS_CALIBRE];
	int countGrupoGlobal[VC_N_GRUPOS_CALIBRE];

	vc_homogeneidade_inicializar(minDiamGlobal, maxDiamGlobal, countGrupoGlobal);

	while (key != 'q') {
		capture.read(frame);

		if (frame.empty()) break;

		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		memcpy(image->data, frame.data, video.width * video.height * 3);

		vc_bgr_to_rgb(image, rgb);
		vc_rgb_to_hsv(rgb, hsv);

		vc_hsv_segmentation(hsv, seg_laranja, 13, 40, 41, 100, 18, 95);

		vc_binary_close(seg_laranja, seg_temp, 3, 3);
		vc_binary_open(seg_temp, seg_laranja, 3, 3);

		int nblobs = 0;
		OVC* blobs = vc_binary_blob_labelling(seg_laranja, labels, &nblobs);

		int nlaranjas_frame = 0;

		if (blobs != NULL)
		{
			vc_binary_blob_info(labels, blobs, nblobs);

			for (int i = 0; i < nblobs; i++)
			{
				float preenchimento = (float)blobs[i].area / (float)(blobs[i].width * blobs[i].height);

				if (preenchimento < 0.70f) continue;

				if (!vc_laranja_completa_na_imagem(blobs[i], video.width, video.height, 3)) continue;

				float diametro_pixels = (blobs[i].width + blobs[i].height) / 2.0f;
				float diametro_mm = diametro_pixels * escala_mm;

				int calibre = obter_calibre(diametro_mm);

				if (calibre == -1) continue;

				float perimetro_mm = blobs[i].perimeter * escala_mm;
				float area_mm = blobs[i].area * (escala_mm * escala_mm);

				// =========================================================================
				// DETEÇÃO DE DEFEITOS NA LARANJA
				// =========================================================================

				float raio = (blobs[i].width + blobs[i].height) / 4.0f;
				float raio_util = raio * 0.95f;

				int kernel = 35;
				int offset = kernel / 2;
				int c = 10;

				for (int y = blobs[i].y; y < blobs[i].y + blobs[i].height; y++)
				{
					for (int x = blobs[i].x; x < blobs[i].x + blobs[i].width; x++)
					{
						if (x < 0 || y < 0 || x >= video.width || y >= video.height)
							continue;

						float dx = (float)(x - blobs[i].xc);
						float dy = (float)(y - blobs[i].yc);

						if ((dx * dx + dy * dy) > (raio_util * raio_util))
							continue;

						long int pos_hsv = y * hsv->bytesperline + x * hsv->channels;
						long int pos_img = y * image->bytesperline + x * image->channels;

						unsigned char h = hsv->data[pos_hsv];
						unsigned char s = hsv->data[pos_hsv + 1];
						unsigned char v = hsv->data[pos_hsv + 2];

						long int soma = 0;
						int count = 0;

						for (int yy = y - offset; yy <= y + offset; yy++)
						{
							for (int xx = x - offset; xx <= x + offset; xx++)
							{
								if (xx < 0 || yy < 0 || xx >= video.width || yy >= video.height)
									continue;

								long int posk = yy * hsv->bytesperline + xx * hsv->channels;

								soma += hsv->data[posk + 2];
								count++;
							}
						}

						if (count == 0)
							continue;

						int media = (int)(soma / count);

						int escuro = (v < media - c);
						int verde = (h >= 35 && h <= 92 && s > 40);

						if (escuro || verde)
						{
							image->data[pos_img] = 0;
							image->data[pos_img + 1] = 0;
							image->data[pos_img + 2] = 255;
						}
					}
				}

				// =========================================================================

				int indice_laranja = encontrar_laranja(laranjas, total_laranjas, blobs[i].xc, blobs[i].yc);

				if (indice_laranja == -1)
				{
					if (total_laranjas < MAX_LARANJAS)
					{
						indice_laranja = total_laranjas;

						laranjas[indice_laranja].ativo = 1;
						laranjas[indice_laranja].xc = blobs[i].xc;
						laranjas[indice_laranja].yc = blobs[i].yc;
						laranjas[indice_laranja].melhor_area = area_mm;
						laranjas[indice_laranja].melhor_perimetro = perimetro_mm;
						laranjas[indice_laranja].melhor_diametro = diametro_mm;
						laranjas[indice_laranja].melhor_calibre = calibre;

						total_laranjas++;

						vc_homogeneidade_atualizar(
							diametro_mm,
							calibre,
							minDiamGlobal,
							maxDiamGlobal,
							countGrupoGlobal
						);
					}
				}
				else
				{
					laranjas[indice_laranja].xc = blobs[i].xc;
					laranjas[indice_laranja].yc = blobs[i].yc;

					if (area_mm > laranjas[indice_laranja].melhor_area)
					{
						laranjas[indice_laranja].melhor_area = area_mm;
						laranjas[indice_laranja].melhor_perimetro = perimetro_mm;
						laranjas[indice_laranja].melhor_diametro = diametro_mm;
						laranjas[indice_laranja].melhor_calibre = calibre;
					}
				}

				nlaranjas_frame++;

				OVC blob_valido[1];
				blob_valido[0] = blobs[i];

				vc_draw_bounding_box(image, blob_valido, 1);
				vc_draw_center_mass(image, blob_valido, 1, 5);

				
				memcpy(frame.data, image->data, video.width * video.height * 3);
				

				str = "AREA:" + std::to_string((int)laranjas[indice_laranja].melhor_area) +
					" PERIMETRO:" + std::to_string((int)laranjas[indice_laranja].melhor_perimetro) +
					" CALIBRE:" + std::to_string(laranjas[indice_laranja].melhor_calibre) +
					" DIAMETRO:" + std::to_string((int)laranjas[indice_laranja].melhor_diametro);

				cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 10),
					cv::FONT_HERSHEY_SIMPLEX, 0.5,
					cv::Scalar(0, 0, 0), 2);
				cv::putText(frame, str, cv::Point(blobs[i].x, blobs[i].y - 10),
					cv::FONT_HERSHEY_SIMPLEX, 0.5,
					cv::Scalar(0, 255, 255), 1);

				memcpy(image->data, frame.data, video.width * video.height * 3);
			}

			free(blobs);
		}

		float maiorDiferenca = 0.0f;
		int homogeneidade = vc_homogeneidade_verificar(minDiamGlobal,maxDiamGlobal,countGrupoGlobal,&maiorDiferenca);

		str = std::string("RESOLUCAO: ").append(std::to_string(video.width)).append("x").append(std::to_string(video.height));
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 25), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		str = std::string("TOTAL DE FRAMES: ").append(std::to_string(video.ntotalframes));
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		str = std::string("FRAME RATE: ").append(std::to_string(video.fps));
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 75), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		str = std::string("N. DA FRAME: ").append(std::to_string(video.nframe));
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 100), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		str = std::string("LARANJAS FRAME: ").append(std::to_string(nlaranjas_frame));
		cv::putText(frame, str, cv::Point(20, 130), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 130), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		str = std::string("TOTAL LARANJAS: ").append(std::to_string(total_laranjas));
		cv::putText(frame, str, cv::Point(20, 160), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);
		cv::putText(frame, str, cv::Point(20, 160), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 1);

		if (homogeneidade)
		{
			str = "HOMOGENEIDADE DE CALIBRE EXISTENTE, DIFERENCA MAXIMA: " +
				std::to_string((int)maiorDiferenca) + " MM";
		}
		else
		{
			str = "HOMOGENEIDADE DE CALIBRE INEXISTENTE, DIFERENCA MAXIMA: " +
				std::to_string((int)maiorDiferenca) + " MM";
		}

		cv::rectangle(frame, cv::Point(15, 670), cv::Point(550, 700), cv::Scalar(0, 0, 0), cv::FILLED);
		cv::putText(frame, str, cv::Point(20, 690), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(255, 255, 255), 1);

		cv::imshow("VC-VIDEO", frame);
		key = cv::waitKey(1);

	}

	vc_timer();

	vc_image_free(image);
	vc_image_free(rgb);
	vc_image_free(hsv);
	vc_image_free(seg_laranja);
	vc_image_free(seg_temp);
	vc_image_free(labels);

	cv::destroyWindow("VC-VIDEO");
	capture.release();

	return 0;
}