////////////////////////////////////////////////////////////////////////////////
//
//  Loading meshes from external files
//
// Copyright (c) 2023 by Carlos Martinho
// 
// modified by João Baracho & Henrique Martins
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
    mgl::OrbitCamera* Camera = nullptr;
    GLint ModelMatrixId;
    mgl::Mesh* Mesh = nullptr;
    std::vector<mgl::Mesh*> meshes;  // Vector to store multiple meshes

    void createMeshes();
    void createShaderPrograms();
    void createCamera();
    void drawScene();
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

void MyApp::createCamera() {
    Camera = new mgl::OrbitCamera(UBO_BP);
    Camera->setViewMatrix(glm::vec3(-8.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    Camera->setPerspectiveMatrix(30.0f, 800.0f / 600.0f, 1.0f, 10.0f);
    //Camera->setOrthoMatrix(-2.0f, 2.0f, -2.0f, 2.0f, 1.0f, 10.0f);
}

/////////////////////////////////////////////////////////////////////////// DRAW

glm::mat4 I(1.0f);
glm::mat4 S;
glm::mat4 R;
glm::mat4 T;
glm::mat4 M;

void MyApp::drawScene() {

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
    glViewport(0, 0, 800, 600);
    //glViewport(0, 0, 640, 480);
}

void MyApp::displayCallback(GLFWwindow* win, double elapsed) {
    drawScene();
    Camera->update();
}

void MyApp::cursorCallback(GLFWwindow* win, double xpos, double ypos) {
    //std::cout << "mouse: " << xpos << " " << ypos << std::endl;
    Camera->cursor(xpos, ypos);
}

void MyApp::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    Camera->mouseButton(button, action);
}

void MyApp::scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    //std::cout << "scroll: " << xoffset << " " << yoffset << std::endl;
    Camera->scroll(xoffset, yoffset);
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
