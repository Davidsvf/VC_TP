#include <iostream>
#include <string>
#include <chrono>
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
	int linha_contagem = video.height / 2;
	bool contou_frame_anterior = false;

	while (key != 'q') {
		capture.read(frame);

		if (frame.empty()) break;

		video.nframe = (int)capture.get(cv::CAP_PROP_POS_FRAMES);

		memcpy(image->data, frame.data, video.width * video.height * 3);

		vc_bgr_to_rgb(image, rgb);
		vc_rgb_to_hsv(rgb, hsv);

		// Segmentação da laranja
		vc_hsv_segmentation(hsv, seg_laranja, 13, 38, 41, 100, 21, 95);

		// Limpeza: close -> open
		vc_binary_close(seg_laranja, seg_temp, 3,3);
		vc_binary_open(seg_temp, seg_laranja, 3, 3);

		int nblobs = 0;
		OVC* blobs = vc_binary_blob_labelling(seg_laranja, labels, &nblobs);
		
		int nlaranjas_frame = 0;
		bool contou_nesta_frame = false;

		if (blobs != NULL)
		{
			vc_binary_blob_info(labels, blobs, nblobs);

			for (int i = 0; i < nblobs; i++)
			{
				float ratio = (float)blobs[i].width / (float)blobs[i].height;
				float preenchimento = (float)blobs[i].area / (float)(blobs[i].width * blobs[i].height);

				if (ratio < 0.80f || ratio > 1.20f) continue;
				if (preenchimento < 0.65f) continue;
				if (blobs[i].area < 700) continue;

				OVC blob_valido[1];
				blob_valido[0] = blobs[i];

				vc_draw_bounding_box(image, blob_valido, 1);
				vc_draw_center_mass(image, blob_valido, 1, 5);

				nlaranjas_frame++;

				if (blobs[i].yc >= linha_contagem - 3 && blobs[i].yc <= linha_contagem + 3)
				{
					contou_nesta_frame = true;
				}

				memcpy(frame.data, image->data, video.width * video.height * 3);

				str = "AREA: " + std::to_string(blobs[i].area) +
					" PERIMETRO: " + std::to_string(blobs[i].perimeter);

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

		if (contou_nesta_frame && !contou_frame_anterior)
		{
			total_laranjas++;
		}

		contou_frame_anterior = contou_nesta_frame;

		memcpy(frame.data, image->data, video.width * video.height * 3);

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