#include <cstdio>
#include <cstdlib>
#include <limits.h>
#include <sndfile.h>
#include <algorithm>
#include <lv2/core/lv2.h>
#include <lv2/atom/atom.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <cmath>

#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

static constexpr float db2lin(float x) {
    return std::pow(10.0f, x / 20.0f);
}

/* LV2 settings */
const char *plugin_path = "/usr/lib/lv2/calf.lv2/calf.so";
const char *plugin_bundle = "/usr/lib/lv2/calf.lv2/";
const char *plugin_uri = "http://calf.sourceforge.net/plugins/MultibandLimiter";

void plugin_init_ports(const LV2_Descriptor *dsc, LV2_Handle lv2_handle, float *in_l, float *in_r, float *out_l, float *out_r, float input_gain) {
    static float dummy_out;

    static float bypass = 0.0f;
    static float level_in = std::clamp(db2lin(input_gain), 1.0f / 64.0f, 64.0f);
    static float level_out = 1.0f;
    static float freq0 = 100.0f;
    static float freq1 = 750.0f;
    static float freq2 = 750.0f;
    static float mode = 1.0f;
    static float limit = db2lin(-6.0f);
    static float attack = 4.0f;
    static float release = 30.0f;
    static float minrel = 0.0f;
    static float weight0 = 0.0f;
    static float weight1 = 0.0f;
    static float weight2 = 0.0f;
    static float weight3 = 0.0f;
    static float release0 = 0.5f;
    static float release1 = 0.2f;
    static float release2 = -0.2f;
    static float release3 = -0.5f;
    static float solo0 = 0.0f;
    static float solo1 = 0.0f;
    static float solo2 = 0.0f;
    static float solo3 = 0.0f;
    static float asc = 1.0f;
    static float asc_coeff = 0.5f;
    static float oversampling = 1.0f;
    static float auto_level = 0.0f; // non-default

    dsc->connect_port(lv2_handle, 0, in_l);
    dsc->connect_port(lv2_handle, 1, in_r);
    dsc->connect_port(lv2_handle, 2, out_l);
    dsc->connect_port(lv2_handle, 3, out_r);

    dsc->connect_port(lv2_handle, 4, &bypass);
    dsc->connect_port(lv2_handle, 5, &level_in);
    dsc->connect_port(lv2_handle, 6, &level_out);
    dsc->connect_port(lv2_handle, 7, &dummy_out);
    dsc->connect_port(lv2_handle, 8, &dummy_out);
    dsc->connect_port(lv2_handle, 9, &dummy_out);
    dsc->connect_port(lv2_handle, 10, &dummy_out);
    dsc->connect_port(lv2_handle, 11, &dummy_out);
    dsc->connect_port(lv2_handle, 12, &dummy_out);
    dsc->connect_port(lv2_handle, 13, &dummy_out);
    dsc->connect_port(lv2_handle, 14, &dummy_out);
    dsc->connect_port(lv2_handle, 15, &freq0);
    dsc->connect_port(lv2_handle, 16, &freq1);
    dsc->connect_port(lv2_handle, 17, &freq2);
    dsc->connect_port(lv2_handle, 18, &mode);
    dsc->connect_port(lv2_handle, 19, &limit);
    dsc->connect_port(lv2_handle, 20, &attack);
    dsc->connect_port(lv2_handle, 21, &release);
    dsc->connect_port(lv2_handle, 22, &minrel);
    dsc->connect_port(lv2_handle, 23, &dummy_out);
    dsc->connect_port(lv2_handle, 24, &dummy_out);
    dsc->connect_port(lv2_handle, 25, &dummy_out);
    dsc->connect_port(lv2_handle, 26, &dummy_out);
    dsc->connect_port(lv2_handle, 27, &weight0);
    dsc->connect_port(lv2_handle, 28, &weight1);
    dsc->connect_port(lv2_handle, 29, &weight2);
    dsc->connect_port(lv2_handle, 30, &weight3);
    dsc->connect_port(lv2_handle, 31, &release0);
    dsc->connect_port(lv2_handle, 32, &release1);
    dsc->connect_port(lv2_handle, 33, &release2);
    dsc->connect_port(lv2_handle, 34, &release3);
    dsc->connect_port(lv2_handle, 35, &solo0);
    dsc->connect_port(lv2_handle, 36, &solo1);
    dsc->connect_port(lv2_handle, 37, &solo2);
    dsc->connect_port(lv2_handle, 38, &solo3);
    dsc->connect_port(lv2_handle, 39, &dummy_out);
    dsc->connect_port(lv2_handle, 40, &dummy_out);
    dsc->connect_port(lv2_handle, 41, &dummy_out);
    dsc->connect_port(lv2_handle, 42, &dummy_out);
    dsc->connect_port(lv2_handle, 43, &asc);
    dsc->connect_port(lv2_handle, 44, &dummy_out);
    dsc->connect_port(lv2_handle, 45, &asc_coeff);
    dsc->connect_port(lv2_handle, 46, &oversampling);
    dsc->connect_port(lv2_handle, 47, &auto_level);
    dsc->connect_port(lv2_handle, 48, NULL);
    dsc->connect_port(lv2_handle, 49, NULL);
}

/* Main Program below */

void die(const char msg[]) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void error(const char msg[]) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void usage() {
    printf("Usage:\n$ audiomaster <input_gain_in_db> <input.wav> <output.flac>\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 4)
        usage();

    float input_gain = std::stof(argv[1]);
    
    // open input file to retrieve information for writing output file
    SF_INFO input_info = {
        .format = 0,
    };

    SNDFILE *input_file = sf_open(argv[2], SFM_READ, &input_info);
    if (input_file == NULL)
        die("sf_open");

    int samplerate = input_info.samplerate;
    int channels = input_info.channels;

    if (channels != 2)
        die("input file is not stereo!");

    SF_INFO output_info = {
        .samplerate = samplerate,
        .channels = channels,
        .format = SF_FORMAT_FLAC | SF_FORMAT_PCM_16,
    };

    SNDFILE *output_file = sf_open(argv[3], SFM_WRITE, &output_info);
    if (output_file == NULL)
        die("sf_open");

    const sf_count_t frames_per_chunk = 4096;

    // setup buffers
    sf_count_t samples_per_chunk = frames_per_chunk * channels;
    float sndfile_buf[samples_per_chunk];

    std::vector<float> lv2_buf_in_left(frames_per_chunk, 0.0f);
    std::vector<float> lv2_buf_in_right(frames_per_chunk, 0.0f);
    std::vector<float> lv2_buf_out_left(frames_per_chunk, 0.0f);
    std::vector<float> lv2_buf_out_right(frames_per_chunk, 0.0f);

    // LV2 init
    void *dl_handle = dlopen(plugin_path, RTLD_LAZY);
    if (!dl_handle)
        die("Could not load plugin");

    LV2_Descriptor_Function lv2_descriptor = reinterpret_cast<LV2_Descriptor_Function>(dlsym(dl_handle, "lv2_descriptor"));
    if (!lv2_descriptor)
        die("Could not find plugin descriptor function");

    const LV2_Descriptor *dsc;

    for (uint32_t i = 0;; i++) {
        dsc = lv2_descriptor(i);
        if (!dsc)
            break;
        if (std::string(dsc->URI) == plugin_uri)
            break;
    }

    if (!dsc)
        error("Could not find specified URI in shared object");

    const LV2_Feature *features[1] = { nullptr };
    LV2_Handle lv2_handle = dsc->instantiate(
            dsc,
            static_cast<double>(samplerate),
            plugin_bundle,
            features
            );
    if (!lv2_handle)
        error("Failed to instantiate plugin");

    dsc->activate(lv2_handle);

    plugin_init_ports(dsc, lv2_handle, lv2_buf_in_left.data(), lv2_buf_in_right.data(), lv2_buf_out_left.data(), lv2_buf_out_right.data(), input_gain);
    
    // "warmup" plugin (workaround pop sounds in calf multiband limiter)

    for (int i = 0; i < 32; i++) {
        dsc->run(lv2_handle, static_cast<uint32_t>(frames_per_chunk));
    }

    sf_count_t frames_read;
    do {
        frames_read = sf_readf_float(input_file, sndfile_buf, frames_per_chunk);

        // split to input buffers

        for (sf_count_t i = 0; i < frames_read; i++) {
            lv2_buf_in_left[i] = sndfile_buf[i*2 + 0];
            lv2_buf_in_right[i] = sndfile_buf[i*2 + 1];
        }

        // LV2 process
        dsc->run(lv2_handle, static_cast<uint32_t>(frames_read));

        // merge output buffers
        for (sf_count_t i = 0; i < frames_read; i++) {
            sndfile_buf[i*2 + 0] = std::clamp(lv2_buf_out_left[i], -1.0f, 1.0f);
            sndfile_buf[i*2 + 1] = std::clamp(lv2_buf_out_right[i], -1.0f, 1.0f);
        }

        if (sf_writef_float(output_file, sndfile_buf, frames_read) != frames_read)
            error("Unknown error while writing output file");
    } while (frames_read == frames_per_chunk);

    // LV2 deinit
    dsc->deactivate(lv2_handle);
    dsc->cleanup(lv2_handle);
    dlclose(dl_handle);

    // close files
    if (sf_close(output_file))
        die("sf_close");

    if (sf_close(input_file))
        die("sf_close");
}
