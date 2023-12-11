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
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
#include <vector>
#include <iostream>

#include "../mgl/mgl.hpp"


///////////////////////////////////////////////////////////////////////// SCENE NODE CLASS

class SceneNode {
private:
    glm::mat4 M[3]; // three model matrices: 0 - box position, 1 - current position, and 2 - tangram shape position
    int positionId = 0;

    float animationStage = 0.0f;
    float prevAnimationStage = 0.0f;
    float animationStep = 0.005f;

    mgl::ShaderProgram* shaders = nullptr;

public:
    glm::vec3 color;
    std::vector<SceneNode*> children;
    mgl::Mesh* mesh;

    void setShader(mgl::ShaderProgram* s) {
        shaders = s;
    }

    void setMesh(mgl::Mesh* m) {
        mesh = m;
    }

    void addChild(SceneNode* child) {
        children.push_back(child);
    }

    void draw(GLint modelMatrixId, GLint colorId, const glm::mat4& parentTransform = glm::mat4(1.0f), mgl::ShaderProgram* parentShader = nullptr) {
        glm::mat4 totalTransform = parentTransform * M[1];

        if (mesh) {
            if (!shaders) { shaders = parentShader; }
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

    void update(bool* pressedKeys) {
        if (pressedKeys[GLFW_KEY_LEFT]) {
            animationStage -= animationStep;
        }
        if (pressedKeys[GLFW_KEY_RIGHT]) {
            animationStage += animationStep;
        }
        animationStage = glm::clamp(animationStage, 0.0f, 1.0f);

        if (animationStage == prevAnimationStage) {
            prevAnimationStage = animationStage;
            return;
        }
        prevAnimationStage = animationStage;

        //Linear interpolation for translation
        glm::vec3 initialTranslation = glm::vec3(M[0][3]);
        glm::vec3 finalTranslation = glm::vec3(M[2][3]);
        glm::vec3 currentTranslation = glm::mix(initialTranslation, finalTranslation, animationStage);


        //Linear interpolation for rotation
        glm::mat3 initialRotationMatrix = glm::mat3(M[0]);
        glm::mat3 finalRotationMatrix = glm::mat3(M[2]);

        // Normalize the columns to remove scaling effects
        for (int i = 0; i < 3; ++i) {
            initialRotationMatrix[i] = glm::normalize(initialRotationMatrix[i]);
            finalRotationMatrix[i] = glm::normalize(finalRotationMatrix[i]);
        }
        glm::quat initialRotation = glm::quat_cast(initialRotationMatrix);
        glm::quat finalRotation = glm::quat_cast(finalRotationMatrix);
        glm::quat currentRotation = glm::lerp(initialRotation, finalRotation, animationStage);


        // Linear interpolation for scaling. Scaling doesnt change so we can just use initial scale
        glm::vec3 currentScale = glm::vec3(glm::length(M[0][0]), glm::length(M[0][1]), glm::length(M[0][2]));

        M[1] = glm::translate(currentTranslation) * glm::toMat4(currentRotation) * glm::scale(currentScale);

        for (SceneNode* child : children) {
            child->update(pressedKeys);
        }
    }

    void addPosition(int pos, glm::mat4 m) {
        M[pos] = m;
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

    //  root node for the scene
    SceneNode root = SceneNode();
    SceneNode triangle1 = SceneNode();
    SceneNode triangle2 = SceneNode();
    SceneNode triangle3 = SceneNode();
    SceneNode triangle4 = SceneNode();
    SceneNode triangle5 = SceneNode();
    SceneNode square = SceneNode();
    SceneNode parallelogram = SceneNode();

    const GLuint UBO_BP[2] = { 0, 1 };
    mgl::OrbitCamera* Cameras[2] = { nullptr, nullptr };
    int cameraId = 1;

    GLint ModelMatrixId;
    GLint ColorId;
    std::vector<mgl::Mesh*> meshes;  // Vector to store multiple meshes

    bool pressedKeys[GLFW_KEY_LAST];

    void createMeshes();
    void createShaderPrograms();
    void createCamera();
    void createScene();
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

void MyApp::createScene() {

    mgl::Mesh* triangleMesh = meshes[0];
    mgl::Mesh* squareMesh = meshes[1];
    mgl::Mesh* parallelogramMesh = meshes[2];
    float side = 0.5f;
    float hypotenuse = sqrt(2 * pow(side, 2));
    float triangleHeight = hypotenuse / 2;

    root.setShader(Shaders);
    M = I;
    root.addPosition(0, M);
    root.addPosition(1, M);
    root.addPosition(2, M);

    // Draw triangles
    triangle1.setShader(Shaders);
    triangle1.setMesh(triangleMesh);
    triangle1.addPosition(0, M); // set box matrix model
    triangle1.addPosition(1, M); // set animation matrix equal to initial position matrix model
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, side, -side));
    M = T * R;
    triangle1.addPosition(2, M); // set tangram shape matrix model
    triangle1.color = glm::vec3(0.0f, 0.62f, 0.65f);
    root.addChild(&triangle1);

    triangle2.setShader(Shaders);
    triangle2.setMesh(triangleMesh);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight - hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    triangle2.addPosition(0, M);
    triangle2.addPosition(1, M);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(-45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 0.0f, side));
    M = T * R;
    triangle2.addPosition(2, M);
    triangle2.color = glm::vec3(0.92f, 0.28f, 0.15f);
    root.addChild(&triangle2);

    triangle3.setShader(Shaders);
    triangle3.setMesh(triangleMesh);
    S = glm::scale(glm::vec3(sqrt(2), sqrt(2), 1));
    R = glm::rotate(glm::radians(135.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-2.0f * hypotenuse, side * sqrt(2), 0.0f));
    M = T * R * S;
    glm::quat ads = glm::quat_cast(R);
    triangle3.addPosition(0, M);
    triangle3.addPosition(1, M);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    M = T * R * S;
    triangle3.addPosition(2, M);
    triangle3.color = glm::vec3(0.43f, 0.23f, 0.75f);
    root.addChild(&triangle3);

    triangle4.setShader(Shaders);
    triangle4.setMesh(triangleMesh);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    M = T * R * S;
    triangle4.addPosition(0, M);
    triangle4.addPosition(1, M);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, side, 2.0f * hypotenuse));
    M = T * R * S;
    triangle4.addPosition(2, M);
    triangle4.color = glm::vec3(0.80f, 0.05f, 0.4f);
    root.addChild(&triangle4);

    triangle5.setShader(Shaders);
    triangle5.setMesh(triangleMesh);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    M = T * R * S;
    triangle5.addPosition(0, M);
    triangle5.addPosition(1, M);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f * hypotenuse));
    M = T * R * S;
    triangle5.addPosition(2, M);
    triangle5.color = glm::vec3(0.06f, 0.51f, 0.95f);
    root.addChild(&triangle5);


    // draw square
    square.setShader(Shaders);
    square.setMesh(squareMesh);
    R = glm::rotate(glm::radians(45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight, triangleHeight, 0.0f));
    M = T * R;
    square.addPosition(0, M);
    square.addPosition(1, M);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, 0.0f, 2.0f * hypotenuse));
    M = T * R;
    square.addPosition(2, M);
    square.color = glm::vec3(0.13f, 0.67f, 0.14f);
    root.addChild(&square);


    // draw parallelogram
    parallelogram.setShader(Shaders);
    parallelogram.setMesh(parallelogramMesh);
    R = glm::rotate(glm::radians(-45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-1.5f * hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    parallelogram.addPosition(0, M);
    parallelogram.addPosition(1, M);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, 0.0f, side));
    M = T * R;
    parallelogram.addPosition(2, M);
    parallelogram.color = glm::vec3(0.99f, 0.55f, 0.0f);
    root.addChild(&parallelogram);

}

void MyApp::drawScene() {
    // draw entire scene from root node
    root.draw(ModelMatrixId, ColorId);
}

////////////////////////////////////////////////////////////////////// CALLBACKS

void MyApp::initCallback(GLFWwindow* win) {
    createMeshes();
    createShaderPrograms();  // after mesh;
    createCamera();
    createScene();
}
void MyApp::windowSizeCallback(GLFWwindow* win, int winx, int winy) {
    glViewport(0, 0, 800, 600);
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
}

void MyApp::displayCallback(GLFWwindow* win, double elapsed) {
    Cameras[cameraId]->update();
    root.update(pressedKeys);
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
