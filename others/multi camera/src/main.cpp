///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2017, STEREOLABS.
//
// All rights reserved.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


/******************************************************************************************************************
 ** This sample demonstrates how to use two ZEDs with the ZED SDK, each grab are in a separated thread            **
 ** This sample has been tested with 3 ZEDs in HD720@30fps resolution                                             **
 ** This only works for Linux                                                                                     **
 *******************************************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctime>
#include <chrono>
#include <thread>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <sl/Camera.hpp>

using namespace std;

const int NUM_CAMERAS = 2;
const int FPS = 15;
const sl::RESOLUTION ZED_RES = sl::RESOLUTION_HD720;


sl::Camera* zed[NUM_CAMERAS];
cv::Mat SbSResult[NUM_CAMERAS];
cv::Mat ZED_LRes[NUM_CAMERAS];
int width, height;
long long ZED_Timestamp[NUM_CAMERAS];

bool stop_signal;

void grab_run(int x) {
    sl::Mat aux;
    sl::RuntimeParameters rt_params;
    while (!stop_signal) {
        sl::ERROR_CODE res = zed[x]->grab(rt_params);

        if (res != sl::SUCCESS) {
            ZED_Timestamp[x] = zed[x]->getCameraTimestamp();
            zed[x]->retrieveImage(aux, sl::VIEW_LEFT, sl::MEM_CPU);
            cv::Mat(aux.getHeight(), aux.getWidth(), CV_8UC4, aux.getPtr<sl::uchar1>(sl::MEM_CPU)).copyTo(SbSResult[x](cv::Rect(0, 0, width, height)));
            zed[x]->retrieveImage(aux, sl::VIEW_DEPTH, sl::MEM_CPU);
            cv::Mat(aux.getHeight(), aux.getWidth(), CV_8UC4, aux.getPtr<sl::uchar1>(sl::MEM_CPU)).copyTo(SbSResult[x](cv::Rect(width, 0, width, height)));
        }
        sl::sleep_ms(1);
    }
    delete zed[x];
}

int main(int argc, char **argv) {

#ifdef WIN32
    std::cout << "Multiple ZEDs are not available under Windows" << std::endl;
    return -1;
#endif

    sl::InitParameters params;
    params.depth_mode = sl::DEPTH_MODE_PERFORMANCE;
    params.camera_resolution = ZED_RES;
    params.camera_fps = FPS;

    // Create every ZED and init them
    for (int i = 0; i < NUM_CAMERAS; i++) {
        zed[i] = new sl::Camera();
        params.camera_linux_id = i;

        sl::ERROR_CODE err = zed[i]->open(params);

        cout << "ZED no. " << i << " -> Result : " << sl::errorCode2str(err) << endl;
        if (err != sl::SUCCESS) {
            delete zed[i];
            return 1;
        }

        width = zed[i]->getResolution().width;
        height = zed[i]->getResolution().height;
        SbSResult[i] = cv::Mat(height, width * 2, CV_8UC4, 1);
    }

    char key = ' ';

    // Create each grabbing thread with the camera number as parameters
    std::vector<std::thread*> thread_vec;
    for (int i = 0; i < NUM_CAMERAS; i++)
        thread_vec.push_back(new std::thread(grab_run, i));

    // Create windows for viewing results with OpenCV
    cv::Size DisplaySize(720, 404);

    for (int i = 0; i < NUM_CAMERAS; i++)
        ZED_LRes[i] = cv::Mat(DisplaySize, CV_8UC4);

    // Loop until 'q' is pressed
    while (key != 'q') {
        // Resize and imshow
        for (int i = 0; i < NUM_CAMERAS; i++) {
            char wnd_name[21];
            sprintf(wnd_name, "ZED no. %d", i);
            cv::resize(SbSResult[i], ZED_LRes[i], DisplaySize);
            cv::imshow(wnd_name, ZED_LRes[i]);
        }

        // Compare Timestamp between both camera (uncomment following line)
        //for (int i = 0; i < NUM_CAMERAS; i++) std::cout << " Timestamp " << i << ": " << ZED_Timestamp[i] << std::endl;

        key = cv::waitKey(20);
    }

    // Send the signal to stop every threads to finish
    stop_signal = true;

    // Wait for every thread to be stopped
    for (auto it : thread_vec) it->join();

    return 0;
}
