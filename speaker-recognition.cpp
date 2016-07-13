#include <dirent.h>
#include <GRT/GRT.h>
#include <iostream>
#include <limits>

#include "mfcc.h"
#include "wav-reader.h"

static const char* kFemaleDatasetDir = "./data/fsew0_v1.1/";
static const char* kMaleDatasetDir = "./data/msak0_v1.1/";

bool load_audio_file_from_directory(
    GRT::TimeSeriesClassificationData& data,
    const char* const directory,
    uint32_t label,
    uint32_t count = numeric_limits<uint32_t>::max(),
    uint32_t skip = 0) {
    DIR *dir;
    struct dirent *ent;
    if ((dir = opendir(directory)) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            char* suffix = strrchr(ent->d_name, '.');
            if (suffix != NULL && 0 == strcmp(suffix, ".wav")) {
                if (skip > 0) { skip -= 1; continue; }

                printf("loading from %s\n", ent->d_name);

                std::string path;
                path.append(directory);
                path.append(ent->d_name);
                WavReader reader(path);

                GRT::MatrixDouble training_sample;
                training_sample.push_back(reader.getData());
                training_sample.transpose();
                data.addSample(label, training_sample);

                if (--count == 0) { return true; }
            }
        }
        closedir(dir);
        return false;
    } else {
        perror("Failed to open .");
        exit(-1);
    }
}

int main(int argc, char* argv[]) {
    GRT::GestureRecognitionPipeline pipeline;

    // Sample rate is 16k, FFT window size 512 => 32 ms, hopsize 128 => 8 ms
    GRT::FFT fft(512, 128, 1,  GRT::FFT::HAMMING_WINDOW, true, false);
    GRT::MFCC mfcc(16000, 512 / 2, 300, 8000, 26, 5, 22);
    GRT::GMM gmm(5, true, true, 0.5);

    pipeline.addFeatureExtractionModule(fft);
    pipeline.addFeatureExtractionModule(mfcc);
    pipeline.setClassifier(gmm);

    GRT::TimeSeriesClassificationData data(1);
    load_audio_file_from_directory(data, kFemaleDatasetDir, 1, 1);
    load_audio_file_from_directory(data, kMaleDatasetDir, 2, 1);
    std::cout << data.getStatsAsString() << std::endl;

    pipeline.train(data);

    // GRT::TimeSeriesClassificationData test_data(1);
    // load_audio_file_from_directory(test_data, kFemaleDatasetDir, 1, 1, 30);

    // GRT::MatrixDouble test_samples = test_data.getDataAsMatrixDouble();
    // vector<uint32_t> classified_result(2, 0);
    // for (uint32_t i = 0; i < test_samples.getNumRows(); i++) {
    //     classified_result[pipeline.predict(test_samples.getRowVector(i))]++;
    // }

    // for (const auto& i : classified_result) {
    //     std::cout << i << std::endl;
    // }
}
