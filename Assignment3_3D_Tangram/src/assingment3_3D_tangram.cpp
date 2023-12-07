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


///////////////////////////////////////////////////////////////////////// SCENE NODE CLASS

class SceneNode {
public:
    glm::mat4 transform;
    std::vector<SceneNode*> children;
    mgl::Mesh* mesh;

    SceneNode(mgl::Mesh* mesh) : mesh(mesh) {}

    void addChild(SceneNode* child) {
        children.push_back(child);
    }

    void draw(GLint modelMatrixId, const glm::mat4& parentTransform = glm::mat4(1.0f)) {
        glm::mat4 totalTransform = parentTransform * transform;

        if (mesh) {

            glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, glm::value_ptr(totalTransform));
            mesh->draw();
        }

        for (SceneNode* child : children) {
            child->draw(modelMatrixId, totalTransform);
        }
    }
};

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
    std::vector<mgl::Mesh*> meshes;  // Vector to store multiple meshes

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

    std::string mesh_file = "triangular-prism.obj";
    mgl::Mesh* prismMesh = new mgl::Mesh();
    prismMesh->joinIdenticalVertices();
    prismMesh->create(mesh_dir + mesh_file);
    meshes.push_back(prismMesh);


    mesh_file = "cube.obj";
    mgl::Mesh* squareMesh = new mgl::Mesh();
    squareMesh->joinIdenticalVertices();
    squareMesh->create(mesh_dir + mesh_file);
    meshes.push_back(squareMesh);


    mesh_file = "parallelepiped.obj";
    mgl::Mesh* parallelepipedMesh = new mgl::Mesh();
    parallelepipedMesh->joinIdenticalVertices();
    parallelepipedMesh->create(mesh_dir + mesh_file);
    meshes.push_back(parallelepipedMesh);
}

///////////////////////////////////////////////////////////////////////// SHADER

void MyApp::createShaderPrograms() {
    Shaders = new mgl::ShaderProgram();
    Shaders->addShader(GL_VERTEX_SHADER, "./src/shaders/vertex_shader.glsl");
    Shaders->addShader(GL_FRAGMENT_SHADER, "./src/shaders/frag_shader.glsl");

    Shaders->addAttribute(mgl::POSITION_ATTRIBUTE, mgl::Mesh::POSITION);

    if (!meshes.empty()) {
        mgl::Mesh* mesh = meshes[0];

        Shaders->addAttribute(mgl::POSITION_ATTRIBUTE, mgl::Mesh::POSITION);
        if (mesh->hasNormals()) {
            Shaders->addAttribute(mgl::NORMAL_ATTRIBUTE, mgl::Mesh::NORMAL);
        }
        if (mesh->hasTexcoords()) {
            Shaders->addAttribute(mgl::TEXCOORD_ATTRIBUTE, mgl::Mesh::TEXCOORD);
        }
        if (mesh->hasTangentsAndBitangents()) {
            Shaders->addAttribute(mgl::TANGENT_ATTRIBUTE, mgl::Mesh::TANGENT);
        }
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
    //const float sensitivity = 0.01f; // Adjust sensitivity as needed

    //// Update yaw (horizontal rotation)
    //Camera->yawCamera(-deltaX * sensitivity);

    //// Update pitch (vertical rotation)
    //Camera->pitchCamera(-deltaY * sensitivity);

    //deltaX = 0.0f;
    //deltaY = 0.0f;

    //// Get the updated rotation angles
    //float newYaw, newPitch;
    //Camera->getEulerAngles(newYaw, newPitch);

    //// Create quaternions for yaw and pitch
    //glm::quat qYaw = glm::angleAxis(newYaw, glm::vec3(0.0f, 1.0f, 0.0f));
    //glm::quat qPitch = glm::angleAxis(newPitch, glm::vec3(1.0f, 0.0f, 0.0f));

    //// Combine quaternions for total rotation
    //glm::quat qTotal = qYaw * qPitch;

    //// Calculate the new view matrix
    //glm::mat4 T = glm::translate(glm::vec3(0.0f, 0.0f, -glm::length(glm::vec3(5.0f, 5.0f, 5.0f))));
    //glm::mat4 R = glm::toMat4(qTotal);

    //Camera->setViewMatrix(T * R);

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

glm::mat4 I(1.0f);
glm::mat4 S;
glm::mat4 R;
glm::mat4 T;
glm::mat4 M;

void MyApp::drawScene() {
    updateCamera();
    mgl::Mesh* triangleMesh = meshes[0];
    mgl::Mesh* squareMesh = meshes[1];
    mgl::Mesh* parallelogramMesh = meshes[2];
    float side = 0.5f;
    float hypotenuse = sqrt(2*pow(side, 2));
    float triangleHeight = hypotenuse/2;

    // Create a root node for the scene
    SceneNode root(nullptr);
    root.transform = I;

    // Draw triangles
    SceneNode triangle1(triangleMesh);
    triangle1.transform = I;
    root.addChild(&triangle1);

    SceneNode triangle2(triangleMesh);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight - hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    triangle2.transform = M;
    root.addChild(&triangle2);

    SceneNode triangle3(triangleMesh);
    S = glm::scale(glm::vec3(sqrt(2), sqrt(2), 1));
    R = glm::rotate(glm::radians(135.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-2.0f * hypotenuse, side * sqrt(2), 0.0f));
    M = T * R * S;
    triangle3.transform = M;
    root.addChild(&triangle3);

    SceneNode triangle4(triangleMesh);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    M = T * R * S;
    triangle4.transform = M;
    root.addChild(&triangle4);

    SceneNode triangle5(triangleMesh);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    M = T * R * S;
    triangle5.transform = M;
    root.addChild(&triangle5);


    // draw square
    SceneNode square(squareMesh);
    R = glm::rotate(glm::radians(45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight, triangleHeight, 0.0f));
    M = T * R;
    square.transform = M;
    root.addChild(&square);


    // draw parallelogram
    SceneNode parallelogram(parallelogramMesh);
    R = glm::rotate(glm::radians(-45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-1.5f * hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    parallelogram.transform = M;
    root.addChild(&parallelogram);


    Shaders->bind();

    // draw entire scene
    root.draw(ModelMatrixId);

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
