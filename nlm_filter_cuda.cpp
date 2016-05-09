#include "nlm_filter_cuda.h"
#include "utils.h"
#include <cuda.h>


// Forward declare the function in the .cu file
void nlm_filter_classic_CUDA(const float* h_src, float* h_dst, int w, int h, float fSigma, float fParam, int patch, int window);

void nlm_filter_cuda(QImage *imageNoise, QImage *imageFiltered, QSize imageSize, int halfWindowSize, int halfPatchSize, float fSigma, float fParam)
{
    if (fSigma > 0.0f && fSigma <= 15.0f) {
        halfPatchSize = 1;
        halfWindowSize = 10;
        fParam = 0.4f;
    } else if ( fSigma > 15.0f && fSigma <= 30.0f) {
        halfPatchSize = 2;
        halfWindowSize = 10;
        fParam = 0.4f;
    } else if ( fSigma > 30.0f && fSigma <= 45.0f) {
        halfPatchSize = 3;
        halfWindowSize = 17;
        fParam = 0.35f;
    } else if ( fSigma > 45.0f && fSigma <= 75.0f) {
        halfPatchSize = 4;
        halfWindowSize = 17;
        fParam = 0.35f;
    } else if (fSigma <= 100.0f) {
        halfPatchSize = 5;
        halfWindowSize = 17;
        fParam = 0.30f;
    }


    /* SIZES */
    int patchSize = halfPatchSize * 2 + 1;
    int w = imageSize.width();
    int h = imageSize.height();
    int incWidth = patchSize - 1 + w;
    int incHeight = patchSize - 1 + h;

    /* CREATE ARRAYS */
    float** colorInput = new float*[w];
    float** colorOutput = new float*[w];
    for (int i = 0; i < w; i++) {
        colorInput[i] = new float[h];
        colorOutput[i] = new float[h];
    }
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            colorInput[i][j] = qGray(imageNoise->pixel(i,j));
        }
    }

    /* INCREASE IMAGE */
    float** increasedImage = new float*[incWidth];
    for (int i = 0; i < incWidth; i++) {
        increasedImage[i] = new float[incHeight];
    }
    nlm_increse_image2(colorInput, increasedImage, imageSize, halfPatchSize);

    /* Creating arrays for processing on device */
    float* h_input = new float[incHeight*incWidth];
    float* h_output = new float[incHeight*incWidth];

    for (int i = 0; i < incWidth; i++) {
        for (int j = 0; j < incHeight; j++) {
            h_output[incWidth*j+i] = 0;
            h_input[incWidth*j+i] = increasedImage[i][j];
        }
    }

//    cuInit(0);
    T_START
    nlm_filter_classic_CUDA(h_input, h_output, incWidth, incHeight, fSigma, fParam, halfPatchSize, halfWindowSize);
    T_END

    /* CREATE FILTERED IMAGE */
    for (int i = 0; i < incWidth; i++) {
        for (int j = 0; j < incHeight; j++) {
            increasedImage[i][j] = h_output[incWidth*j+i];
        }
    }
    for (int i = 0; i < w; i++) {
        for (int j = 0; j < h; j++) {
            int gray = increasedImage[i][j];
            imageFiltered->setPixel(i, j, qRgb(gray, gray, gray));
        }
    }

    /* CLEAR MEMORY */
    for (int i = 0; i < w; i++) {
        delete []colorInput[i];
        delete []colorOutput[i];
    }
    delete []colorOutput;
    delete []colorInput;
}
