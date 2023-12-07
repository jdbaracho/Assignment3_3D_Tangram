////////////////////////////////////////////////////////////////////////////////
//
//  Loading meshes from external files
//
// Copyright (c) 2023 by Carlos Martinho
//
// INTRODUCES:
// MODEL DATA, ASSIMP, mglMesh.hpp
//
////////////////////////////////////////////////////////////////////////////////

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "../mgl/mgl.hpp"

////////////////////////////////////////////////////////////////////////// MYAPP

class MyApp : public mgl::App {
public:
    void initCallback(GLFWwindow* win) override;
    void displayCallback(GLFWwindow* win, double elapsed) override;
    void windowSizeCallback(GLFWwindow* win, int width, int height) override;
    void cursorCallback(GLFWwindow* win, double xpos, double ypos) override;
    void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) override;
    void scrollCallback(GLFWwindow* win, double xoffset, double yoffset) override;

private:
    const GLuint UBO_BP = 0;
    mgl::ShaderProgram* Shaders = nullptr;
    mgl::Camera* Camera = nullptr;
    GLint ModelMatrixId;
    mgl::Mesh* Mesh = nullptr;

    bool leftClick = false;
    double prevXpos, prevYpos;
    float deltaX = 0.0f, deltaY = 0.0f;

    void createMeshes();
    void createShaderPrograms();
    void createCamera();
    void drawScene();

    void updateCamera();
};

///////////////////////////////////////////////////////////////////////// MESHES

void MyApp::createMeshes() {
    std::string mesh_dir = "./assets/models/";
    //std::string mesh_file = "cube.obj";
    //std::string mesh_file = "triangular-prism.obj";
    std::string mesh_file = "parallelepiped.obj";
    //std::string mesh_file = "cube-v.obj";
    // std::string mesh_file = "cube-vn.obj";
    // std::string mesh_file = "cube-vtn.obj";
    // std::string mesh_file = "cube-vtn-2.obj";
    // std::string mesh_file = "torus-vtn-flat.obj";
    // std::string mesh_file = "torus-vtn-smooth.obj";
    // std::string mesh_file = "suzanne-vtn-flat.obj";
    // std::string mesh_file = "suzanne-vtn-smooth.obj";
    // std::string mesh_file = "teapot-vn-flat.obj";
    // std::string mesh_file = "teapot-vn-smooth.obj";
    // std::string mesh_file = "bunny-vn-flat.obj";
    // std::string mesh_file = "bunny-vn-smooth.obj";
    //std::string mesh_file = "monkey-torus-vtn-flat.obj";
    std::string mesh_fullname = mesh_dir + mesh_file;

    Mesh = new mgl::Mesh();
    Mesh->joinIdenticalVertices();
    Mesh->create(mesh_fullname);
}

///////////////////////////////////////////////////////////////////////// SHADER

void MyApp::createShaderPrograms() {
    Shaders = new mgl::ShaderProgram();
    Shaders->addShader(GL_VERTEX_SHADER, "./src/shaders/vertex_shader.glsl");
    Shaders->addShader(GL_FRAGMENT_SHADER, "./src/shaders/frag_shader.glsl");

    Shaders->addAttribute(mgl::POSITION_ATTRIBUTE, mgl::Mesh::POSITION);
    if (Mesh->hasNormals()) {
        Shaders->addAttribute(mgl::NORMAL_ATTRIBUTE, mgl::Mesh::NORMAL);
    }
    if (Mesh->hasTexcoords()) {
        Shaders->addAttribute(mgl::TEXCOORD_ATTRIBUTE, mgl::Mesh::TEXCOORD);
    }
    if (Mesh->hasTangentsAndBitangents()) {
        Shaders->addAttribute(mgl::TANGENT_ATTRIBUTE, mgl::Mesh::TANGENT);
    }

    Shaders->addUniform(mgl::MODEL_MATRIX);
    Shaders->addUniformBlock(mgl::CAMERA_BLOCK, UBO_BP);
    Shaders->create();

    ModelMatrixId = Shaders->Uniforms[mgl::MODEL_MATRIX].index;
}

///////////////////////////////////////////////////////////////////////// CAMERA

// Axis
const glm::vec3 XX(0.0f, 1.0f, 0.0f);
const glm::vec3 YY(1.0f, 0.0f, 0.0f);

// Eye(5,5,5) Center(0,0,0) Up(0,1,0)
const glm::mat4 ViewMatrix1 =
glm::lookAt(glm::vec3(5.0f, 5.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

// Eye(-5,-5,-5) Center(0,0,0) Up(0,1,0)
const glm::mat4 ViewMatrix2 =
glm::lookAt(glm::vec3(-5.0f, -5.0f, -5.0f), glm::vec3(0.0f, 0.0f, 0.0f),
    glm::vec3(0.0f, 1.0f, 0.0f));

// Orthographic LeftRight(-2,2) BottomTop(-2,2) NearFar(1,10)
const glm::mat4 ProjectionMatrix1 =
glm::ortho(-2.0f, 2.0f, -2.0f, 2.0f, 1.0f, 10.0f);

// Perspective Fovy(30) Aspect(640/480) NearZ(1) FarZ(10)
const glm::mat4 ProjectionMatrix2 =
glm::perspective(glm::radians(30.0f), 640.0f / 480.0f, 1.0f, 10.0f);

glm::quat q = glm::angleAxis(0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
const glm::mat4 ViewMatrix3 = glm::translate(glm::vec3(0, 0, -glm::length(glm::vec3(5.0f, 5.0f, 5.0f)))) * glm::toMat4(q);

void MyApp::createCamera() {
    Camera = new mgl::Camera(UBO_BP);
    Camera->setViewMatrix(ViewMatrix3);
    Camera->setProjectionMatrix(ProjectionMatrix2);
}

void MyApp::updateCamera() {

    glm::quat qX = glm::angleAxis(deltaX / 100, XX);
    glm::quat qY = glm::angleAxis(deltaY / 100, YY);

    deltaX = 0.0f;
    deltaY = 0.0f;

    q = qX * qY * q;

    glm::mat4 T = glm::translate(glm::vec3(0.0f, 0.0f, -glm::length(glm::vec3(5.0f, 5.0f, 5.0f))));
    glm::mat4 R = glm::toMat4(q);

    Camera->setViewMatrix(T * R);
}

/////////////////////////////////////////////////////////////////////////// DRAW

glm::mat4 ModelMatrix(1.0f);

void MyApp::drawScene() {
    updateCamera();
    Shaders->bind();
    glUniformMatrix4fv(ModelMatrixId, 1, GL_FALSE, glm::value_ptr(ModelMatrix));
    Mesh->draw();
    Shaders->unbind();
}

////////////////////////////////////////////////////////////////////// CALLBACKS

void MyApp::initCallback(GLFWwindow* win) {
    createMeshes();
    createShaderPrograms();  // after mesh;
    createCamera();
}
void MyApp::windowSizeCallback(GLFWwindow* win, int winx, int winy) {
    glViewport(0, 0, winx, winy);
    // change projection matrices to maintain aspect ratio
}

void MyApp::displayCallback(GLFWwindow* win, double elapsed) { drawScene(); }

void MyApp::cursorCallback(GLFWwindow* win, double xpos, double ypos) {
    //std::cout << "mouse: " << xpos << " " << ypos << std::endl;

    if (leftClick) {

        deltaX += xpos - prevXpos;
        deltaY += ypos - prevYpos;
    }

    prevXpos = xpos;
    prevYpos = ypos;
}

void MyApp::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    leftClick = button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS;
}

void MyApp::scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    std::cout << "scroll: " << xoffset << " " << yoffset << std::endl;
}

/////////////////////////////////////////////////////////////////////////// MAIN

int main(int argc, char* argv[]) {
    mgl::Engine& engine = mgl::Engine::getInstance();
    engine.setApp(new MyApp());
    engine.setOpenGL(4, 6);
    engine.setWindow(800, 600, "Mesh Loader", 0, 1);
    engine.init();
    engine.run();
    exit(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
