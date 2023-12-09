////////////////////////////////////////////////////////////////////////////////
//
//  Loading meshes from external files
//
// Copyright (c) 2023 by Carlos Martinho
// 
// modified by João Baracho & Henrique Martins
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
private:
    int positionId = 0;
    glm::mat4 S[2];
    glm::mat4 R[2];
    glm::mat4 T[2];

    mgl::ShaderProgram* shaders = nullptr;

public:
    glm::mat4 transform;
    glm::vec3 color;
    std::vector<SceneNode*> children;
    mgl::Mesh* mesh;

    SceneNode(mgl::Mesh* mesh, mgl::ShaderProgram* shaders) : shaders(shaders), mesh(mesh) {}

    void addChild(SceneNode* child) {
        children.push_back(child);
    }

    void draw(GLint modelMatrixId, GLint colorId, const glm::mat4& parentTransform = glm::mat4(1.0f), mgl::ShaderProgram* parentShader = nullptr) {
        glm::mat4 totalTransform = parentTransform * T[positionId] * R[positionId] * S[positionId];

        if (mesh) {
            if (!shaders) { shaders = parentShader;  }
            shaders->bind();
            glUniformMatrix4fv(modelMatrixId, 1, GL_FALSE, glm::value_ptr(totalTransform));
            glUniform3f(colorId, color[0], color[1], color[2]);
            mesh->draw();
            shaders->unbind();
        }

        for (SceneNode* child : children) {
            child->draw(modelMatrixId, colorId, totalTransform, shaders);
        }
    }

    void addPosition(int pos, glm::mat4 s, glm::mat4 r, glm::mat4 t) {
        S[pos] = s;
        R[pos] = r;
        T[pos] = t;
    }
};

////////////////////////////////////////////////////////////////////////// MYAPP

class MyApp : public mgl::App {
public:
    void initCallback(GLFWwindow* win) override;
    void displayCallback(GLFWwindow* win, double elapsed) override;
    void windowSizeCallback(GLFWwindow* win, int width, int height) override;
    void keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) override;
    void cursorCallback(GLFWwindow* win, double xpos, double ypos) override;
    void mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) override;
    void scrollCallback(GLFWwindow* win, double xoffset, double yoffset) override;

private:
    mgl::ShaderProgram* Shaders = nullptr;

    const GLuint UBO_BP[2] = { 0, 1 };
    mgl::OrbitCamera* Cameras[2] = { nullptr, nullptr };
    int cameraId = 1;

    GLint ModelMatrixId;
    GLint ColorId;
    mgl::Mesh* Mesh = nullptr;
    std::vector<mgl::Mesh*> meshes;  // Vector to store multiple meshes

    bool pressedKeys[GLFW_KEY_LAST];

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
    Shaders->addUniform(mgl::COLOR);
    Shaders->addUniformBlock(mgl::CAMERA_BLOCK, UBO_BP[0]);
    
    Shaders->create();

    ModelMatrixId = Shaders->Uniforms[mgl::MODEL_MATRIX].index;
    ColorId = Shaders->Uniforms[mgl::COLOR].index;
}

///////////////////////////////////////////////////////////////////////// CAMERA

void MyApp::createCamera() {
    Cameras[0] = new mgl::OrbitCamera(UBO_BP[0], 'A');
    Cameras[0]->setViewMatrix(glm::vec3(0.0f, 0.0f, 8.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    Cameras[0]->setOrthoMatrix(-2.0f, 2.0f, -2.0f, 2.0f, 1.0f, 10.0f);
    Cameras[0]->setPerspectiveMatrix(30.0f, 800.0f / 600.0f, 1.0f, 10.0f);

    Cameras[1] = new mgl::OrbitCamera(UBO_BP[0], 'B');
    Cameras[1]->setViewMatrix(glm::vec3(-8.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));
    Cameras[1]->setOrthoMatrix(-2.0f, 2.0f, -2.0f, 2.0f, 1.0f, 10.0f);
    Cameras[1]->setPerspectiveMatrix(30.0f, 800.0f / 600.0f, 1.0f, 10.0f);
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
    SceneNode root(nullptr, Shaders);
    root.addPosition(0, I, I, I);
    root.addPosition(1, I, I, I);

    // Draw triangles
    SceneNode triangle1(triangleMesh, Shaders);
    triangle1.addPosition(0, I, I, I);
    triangle1.addPosition(1, I, I, I);
    triangle1.color = glm::vec3(0.0f, 0.62f, 0.65f);
    root.addChild(&triangle1);

    SceneNode triangle2(triangleMesh, Shaders);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight - hypotenuse, triangleHeight, 0.0f));
    triangle2.addPosition(0, I, R, T);
    triangle2.addPosition(1, I, R, T);
    triangle2.color = glm::vec3(0.92f, 0.28f, 0.15f);
    root.addChild(&triangle2);

    SceneNode triangle3(triangleMesh, Shaders);
    S = glm::scale(glm::vec3(sqrt(2), sqrt(2), 1));
    R = glm::rotate(glm::radians(135.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-2.0f * hypotenuse, side * sqrt(2), 0.0f));
    triangle3.addPosition(0, S, R, T);
    triangle3.addPosition(1, S, R, T);
    triangle3.color = glm::vec3(0.43f, 0.23f, 0.75f);
    root.addChild(&triangle3);

    SceneNode triangle4(triangleMesh, Shaders);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    triangle4.addPosition(0, S, R, T);
    triangle4.addPosition(1, S, R, T);
    triangle4.color = glm::vec3(0.80f, 0.05f, 0.4f);
    root.addChild(&triangle4);

    SceneNode triangle5(triangleMesh, Shaders);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    triangle5.addPosition(0, S, R, T);
    triangle5.addPosition(1, S, R, T);
    triangle5.color = glm::vec3(0.06f, 0.51f, 0.95f);
    root.addChild(&triangle5);


    // draw square
    SceneNode square(squareMesh, Shaders);
    R = glm::rotate(glm::radians(45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight, triangleHeight, 0.0f));
    square.addPosition(0, I, R, T);
    square.addPosition(1, I, R, T);
    square.color = glm::vec3(0.13f, 0.67f, 0.14f);
    root.addChild(&square);


    // draw parallelogram
    SceneNode parallelogram(parallelogramMesh, Shaders);
    R = glm::rotate(glm::radians(-45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-1.5f * hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    parallelogram.transform = M;
    parallelogram.addPosition(0, I, R, T);
    parallelogram.addPosition(1, I, R, T);
    parallelogram.color = glm::vec3(0.99f, 0.55f, 0.0f);
    root.addChild(&parallelogram);

    // draw entire scene
    root.draw(ModelMatrixId, ColorId);
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

void MyApp::keyCallback(GLFWwindow* win, int key, int scancode, int action, int mods) {
    //std::cout << "key: " << key << " " << scancode << " " << action << " " << mods << std::endl;
    pressedKeys[key] = action != GLFW_RELEASE;

    if (action == GLFW_RELEASE) {
        switch (key) {
        case GLFW_KEY_C:
            cameraId = (cameraId + 1) % 2;
            Cameras[cameraId]->activate();
            break;

        case GLFW_KEY_P:
            Cameras[cameraId]->changeProjection();
            break;
        }
    }

    //if (key == GLFW_KEY_C && action == GLFW_RELEASE) {
    //    cameraId = (cameraId + 1) % 2;
    //    Cameras[cameraId]->activate();
    //}
}

void MyApp::displayCallback(GLFWwindow* win, double elapsed) {
    Cameras[cameraId]->update();
    drawScene();
}

void MyApp::cursorCallback(GLFWwindow* win, double xpos, double ypos) {
    //std::cout << "mouse: " << xpos << " " << ypos << std::endl;
    Cameras[cameraId]->cursor(xpos, ypos);
}

void MyApp::mouseButtonCallback(GLFWwindow* win, int button, int action, int mods) {
    Cameras[cameraId]->mouseButton(win, button, action);
}

void MyApp::scrollCallback(GLFWwindow* win, double xoffset, double yoffset) {
    //std::cout << "scroll: " << xoffset << " " << yoffset << std::endl;
    Cameras[cameraId]->scroll(xoffset, yoffset);
}

/////////////////////////////////////////////////////////////////////////// MAIN

int main(int argc, char* argv[]) {
    mgl::Engine& engine = mgl::Engine::getInstance();
    engine.setApp(new MyApp());
    engine.setOpenGL(4, 6);
    engine.setWindow(800, 600, "Assignment 3: 3D Tangram", 0, 1);
    engine.init();
    engine.run();
    exit(EXIT_SUCCESS);
}

////////////////////////////////////////////////////////////////////////////////
