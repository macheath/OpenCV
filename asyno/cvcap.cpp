//
//  cvcap.cpp
//  Frame Capture
//
//  Project asyno
//
//  Miki Fossati and Lorenzo Benoni 09/12/2016.
//
//
#include "zmq.hpp"
#include <string>
#include <iostream>
#include <opencv2/opencv.hpp>

#include "codecs.hpp"
#include "ffcodecs.hpp"

int main( void )
{
    zmq::context_t context (1);
    zmq::socket_t transmitter (context, ZMQ_PUSH);
    zmq::socket_t receiver (context, ZMQ_PULL);

    const int dst_width = 1280;
    const int dst_height = 720;

    cv::VideoCapture cap;
    cap.set(cv::CAP_PROP_FRAME_WIDTH, dst_width);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, dst_height);

    std::cout << "Connecting to server..." << std::endl;
    transmitter.connect ("tcp://127.0.0.1:5555");
    receiver.bind ("tcp://*:5553");

    cap.open( 0 );
    if(!cap.isOpened()) { std::cout << "--(!)Error opening video capture\n" << std::endl; return -1; }

// Sending 0 and waiting for answer
    zmq::message_t zero(1);
    memcpy(zero.data(), "0", 1);
    std::cout << "Sending zero..." << std::endl;
    transmitter.send(zero);
    zmq::message_t answer;
    receiver.recv (&answer);
    std::cout << "Answer received! Ready to go..." << std::endl;
// Ok, answer received, we're good to go

    unsigned nb_frames = 0;
    asyno_codec_init();
    std::vector<uint8_t> imgbuf(dst_height * dst_width * 3 + 16);
    cv::Mat frame(dst_height, dst_width, CV_8UC3, imgbuf.data(), dst_width * 3);
    AsynoCodecContext* encoder = asyno_create_encoder_context(AVCodecID::AV_CODEC_ID_H264, dst_width, dst_height);

    while (  cap.read(frame) )
    {
        if( frame.empty() )
        {
            std::cout << " --(!) No captured frame -- Break!" << std::endl;
            break;
        }

        int len = 0;
        uint8_t* bytes = asyno_encode_frame(&frame, encoder, &len);
        if (len > 0) {
//            std::cout << "OK PANIC! (" << len << ")" << std::endl;
            zmq::message_t request (len);
            memcpy(request.data (), bytes, len);
            transmitter.send (request);
            std::cout << nb_frames << '\r' << std::flush;
            ++nb_frames;
        }
    }
    return 0;
}
