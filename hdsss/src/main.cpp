
#include <glog/logging.h>

#include <argparse/argparse.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <loo/loo.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include "HDSSSApplication.hpp"
#include "glm/trigonometric.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/string_cast.hpp"

using json = nlohmann::json;

namespace fs = std::filesystem;
using namespace std;

static glm::vec3 parseVec3(const json& j, const string& key,
                           glm::vec3 defaultVal) {
    if (j.contains(key)) {
        auto& v = j[key];
        return glm::vec3(v[0], v[1], v[2]);
    } else {
        return defaultVal;
    }
}

HDSSSConfig parseJSONConfig(const char* filename, string& modelPath,
                            string& skyboxPath) {
    fs::path p(filename);
    LOG(INFO) << "Loading config from " << fs::absolute(filename).string()
              << endl;
    HDSSSConfig config;
    ifstream ifs(filename);
    if (ifs.fail()) {
        LOG(WARNING) << "Failed to open config file " << filename << endl;
        return config;
    }
    json conf = json::parse(ifs);
    if (conf.contains("camera")) {
        auto& camera = conf["camera"];
        config.camera.position =
            parseVec3(camera, "position", config.camera.position);
        config.camera.lookat =
            parseVec3(camera, "lookat", config.camera.lookat);
        config.camera.fov =
            glm::radians(camera.value("fov", config.camera.fov));
        config.camera.zNear = camera.value("znear", config.camera.zNear);
        config.camera.zFar = camera.value("zfar", config.camera.zFar);
    }
    if (conf.contains("light")) {
        auto& light = conf["light"];
        config.light.direction =
            parseVec3(light, "direction", config.light.direction);
        config.light.color = parseVec3(light, "color", config.light.color);
        config.light.intensity =
            light.value("intensity", config.light.intensity);
    }
    if (conf.contains("model")) {
        auto& model = conf["model"];
        float scale = model.value("scale", 1.0f);
        float rotationY = glm::radians(model.value("rotationY", 0.0f));
        config.model.transform = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        config.model.transform =
            glm::rotate(config.model.transform, rotationY, glm::vec3(0, 1, 0));
        modelPath = model.value("path", modelPath);
    }
    if (conf.contains("skybox")) {
        auto& skybox = conf["skybox"];
        skyboxPath = skybox.value("path", skyboxPath);
    }
    if (conf.contains("bssrdf")) {
        auto& bssrdf = conf["bssrdf"];
        config.bssrdf.sigma_t =
            parseVec3(bssrdf, "sigma_t", config.bssrdf.sigma_t);
        config.bssrdf.albedo =
            parseVec3(bssrdf, "albedo", config.bssrdf.albedo);
    }
    if (conf.contains("animation")) {
        auto& animation = conf["animation"];
        config.animation.cameraRotationY =
            glm::radians(animation.value("cameraRotationY", 0.0f));
        config.animation.modelRotationY =
            glm::radians(animation.value("modelRotationY", 0.0f));
    }
    return config;
}

void loadScene(HDSSSApplication& app, const char* filename,
               glm::mat4 transform) {
    using namespace std;
    fs::path p(filename);
    auto suffix = p.extension();
    if (suffix == ".obj" || suffix == ".fbx") {
        LOG(INFO) << "Loading model from " << suffix << " file" << endl;
        app.loadModel(filename, transform);
    } else if (suffix == ".gltf" || suffix == ".glb") {
        LOG(INFO) << "Loading scene from gltf file" << endl;
        app.loadGLTF(filename, transform);
    } else {
        LOG(FATAL) << "Unrecognizable file extension " << suffix << endl;
    }
    app.convertMaterial();
}

int main(int argc, char* argv[]) {
    loo::initialize(argv[0]);

    argparse::ArgumentParser program("HDSSS");
    program.add_argument("-m", "--model").help("Model file path");
    program.add_argument("-s", "--scaling")
        .default_value(1.0f)
        .help("Scaling factor of the model")
        .scan<'g', float>();
    program.add_argument("-b", "--skybox")
        .help(
            "Skybox directory, name the six faces as "
            "[front|back|left|right|top|bottom].jpg");

    program.add_argument("-c", "--config").help("JSON config file path");
    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << endl;
        std::cout << program;
        exit(1);
    }
    float scaling = program.get<float>("--scaling");

    HDSSSConfig config;
    string modelPath, skyboxDir;

    if (auto configPath = program.present<string>("-c")) {
        config = parseJSONConfig(configPath->c_str(), modelPath, skyboxDir);
    }

    // override config with command line arguments
    if (auto path = program.present<string>("-m")) {
        modelPath = *path;
    }
    if (auto path = program.present<string>("-b")) {
        skyboxDir = *path;
    }
    config.model.transform =
        glm::scale(config.model.transform, glm::vec3(scaling));

    HDSSSApplication app(960, 720, config,
                         skyboxDir.length() == 0 ? nullptr : skyboxDir.c_str());
    loadScene(app, modelPath.c_str(), config.model.transform);
    app.run();
}