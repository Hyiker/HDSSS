/**
 * Application.hpp
 * Contributors:
 *      * Arthur Sonzogni (author)
 * Licence:
 *      * MIT
 */

#include "loo/Application.hpp"

#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <stdexcept>

#include "glog/logging.h"
#include "loo/glError.hpp"

namespace loo {

using namespace std;

Application::Application(int width, int height, const std::string& title)
    : state(stateReady), width(width), height(height), title(title) {
    initGLFW();
    // glad load
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Fail to initialize GLAD");
    }
    initImGUI();

    // opengl configuration
    // glEnable(GL_DEPTH_TEST);  // enable depth-testing
    // glDepthFunc(
    //     GL_LESS);  // depth-testing interprets a smaller value as "closer"

    // uncomment to disable vsync
    // glfwSwapInterval(false);
}

GLFWwindow* Application::getWindow() const { return window; }

void Application::exit() { state = stateExit; }

float Application::getFrameDeltaTime() const { return deltaTime; }

float Application::getTime() const { return time; }

void Application::run() {
    state = stateRun;

    // Make the window's context current
    glfwMakeContextCurrent(window);

    time = glfwGetTime();

    while (state == stateRun && !glfwWindowShouldClose(window)) {
        // set glfwPointer to current window just to make sure callback get the
        // right context
        glfwSetWindowUserPointer(getWindow(), this);
        // compute new time and delta time
        float t = glfwGetTime();
        deltaTime = t - time;
        time = t;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // detech window related changes
        // detectWindowDimensionChange();

        // execute the frame code
        this->loop();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Swap Front and Back buffers (double buffering)
        glfwSwapBuffers(window);

        // Pool and process events
        glfwPollEvents();
    }
    beforeCleanup();
    LOG(INFO) << "Cleaning up" << endl;
    // Cleanup
    cleanup();
    afterCleanup();
}

void Application::detectWindowDimensionChange() {
    int w, h;
    glfwGetWindowSize(getWindow(), &w, &h);
    dimensionChanged = (w != width || h != height);
    if (dimensionChanged) {
        width = w;
        height = h;
        glViewport(0, 0, width, height);
    }
}

void Application::loop() {}

void Application::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    logPossibleGLError();

    glfwDestroyWindow(window);
    glfwTerminate();
}

#ifdef __APPLE__
int Application::getFramebufferWidth() {
    int width, _;
    glfwGetFramebufferSize(getWindow(), &width, &_);
    return width;
}

int Application::getFramebufferHeight() {
    int _, height;
    glfwGetFramebufferSize(getWindow(), &_, &height);
    return height;
}
int Application::getWindowWidth() { return width; }

int Application::getWindowHeight() { return height; }
#else
int Application::getWidth() { return width; }

int Application::getHeight() { return height; }
#endif

float Application::getWindowRatio() { return float(width) / float(height); }

bool Application::windowDimensionChanged() { return dimensionChanged; }
void Application::initGLFW() {
    // initialize the GLFW library
    LOG(INFO) << "Initializing GLFW..." << endl;
    if (!glfwInit()) {
        LOG(FATAL) << "Couldn't init GLFW" << endl;
    }

    // setting the opengl version
    int major = 4;
    int minor = 6;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    // create the window
    window = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!window) {
        glfwTerminate();
        LOG(FATAL) << "Couldn't create GLFW window" << endl;
    }

    glfwMakeContextCurrent(window);
}
void Application::initImGUI() {
    // ImGui setup
    LOG(INFO) << "Initializing Dear ImGUI..." << endl;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    logPossibleGLError();

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    (void)io;
}

bool Application::keyForward() {
    return glfwGetKey(getWindow(), GLFW_KEY_W) == GLFW_PRESS ||
           glfwGetKey(getWindow(), GLFW_KEY_UP) == GLFW_PRESS;
}
bool Application::keyBackward() {
    return glfwGetKey(getWindow(), GLFW_KEY_S) == GLFW_PRESS ||
           glfwGetKey(getWindow(), GLFW_KEY_DOWN) == GLFW_PRESS;
}
bool Application::keyLeft() {
    return glfwGetKey(getWindow(), GLFW_KEY_A) == GLFW_PRESS ||
           glfwGetKey(getWindow(), GLFW_KEY_LEFT) == GLFW_PRESS;
}
bool Application::keyRight() {
    return glfwGetKey(getWindow(), GLFW_KEY_D) == GLFW_PRESS ||
           glfwGetKey(getWindow(), GLFW_KEY_RIGHT) == GLFW_PRESS;
}

}  // namespace loo
