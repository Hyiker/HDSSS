
#include <glog/logging.h>

#include <filesystem>
#include <iostream>
#include <loo/loo.hpp>
#include <string>

#include "HDSSSApplication.hpp"
namespace fs = std::filesystem;

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

    if (argc < 2)
        LOG(FATAL) << "Bad argument count\n";
    float scaling = 1.0;
    if (argc >= 3) {
        scaling = std::stof(argv[2]);
    }
    const char* skyboxPath{};
    if (argc >= 4)
        skyboxPath = argv[3];
    HDSSSApplication app(1920, 1280, skyboxPath);
    loadScene(app, argv[1], scaling);
    app.run();
}