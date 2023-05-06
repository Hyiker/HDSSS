
#include <glog/logging.h>

#include <argparse/argparse.hpp>
#include <filesystem>
#include <iostream>
#include <loo/loo.hpp>
#include <string>
#include "HDSSSApplication.hpp"

namespace fs = std::filesystem;
using namespace std;
void loadScene(HDSSSApplication& app, const char* filename, float scaling) {
    using namespace std;
    fs::path p(filename);
    auto suffix = p.extension();
    if (suffix == ".obj" || suffix == ".fbx") {
        LOG(INFO) << "Loading model from " << suffix << " file" << endl;
        app.loadModel(filename, scaling);
    } else if (suffix == ".gltf" || suffix == ".glb") {
        LOG(INFO) << "Loading scene from gltf file" << endl;
        app.loadGLTF(filename, scaling);
    } else {
        LOG(FATAL) << "Unrecognizable file extension " << suffix << endl;
    }
    app.convertMaterial();
}

int main(int argc, char* argv[]) {
    loo::initialize(argv[0]);

    argparse::ArgumentParser program("HDSSS");
    program.add_argument("model").help("Model file path");
    program.add_argument("-s", "--scaling")
        .default_value(1.0)
        .scan<'g', float>()
        .help("Scaling factor of the model");
    program.add_argument("-b", "--skybox")
        .help(
            "Skybox directory, name the six faces as "
            "[front|back|left|right|top|bottom].jpg");
    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << endl;
        std::cout << program;
        exit(1);
    }
    float scaling = program.get<float>("--scaling");

    const char* skyboxPath;
    string skyboxString;
    if (auto skybox = program.present<string>("-b")) {
        skyboxString = *skybox;
        skyboxPath = skyboxString.c_str();
    } else {
        skyboxPath = nullptr;
    }
    HDSSSApplication app(1920, 1080, skyboxPath);
    auto modelPath = program.get<string>("model").c_str();
    loadScene(app, modelPath, scaling);
    app.run();
}