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
#include <vector>
#include <iostream>


#include "../mgl/mgl.hpp"


///////////////////////////////////////////////////////////////////////// SCENE NODE CLASS

class SceneNode {
private:
    glm::mat4 M[3]; // three model matrices: 0 - box position, 1 - mid-animation position, and 2 - tangram shape position

    mgl::ShaderProgram* shaders = nullptr;

public:
    static int positionId;
    glm::vec3 color;
    std::vector<SceneNode*> children;
    mgl::Mesh* mesh;

    SceneNode(mgl::Mesh* mesh, mgl::ShaderProgram* shaders) : shaders(shaders), mesh(mesh) { }

    void addChild(SceneNode* child) {
        children.push_back(child);
    }

    void draw(GLint modelMatrixId, GLint colorId, const glm::mat4& parentTransform = glm::mat4(1.0f), mgl::ShaderProgram* parentShader = nullptr, bool animate = false) {
        glm::mat4 totalTransform = parentTransform * (animate ? M[1] : M[SceneNode::positionId]);

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

    bool animate(GLint modelMatrixId, GLint colorId, bool paused, float p, float d) {
        bool ended = true;
        if (mesh) {
            if (!paused) {
                glm::mat4 initialMatrixModel = M[SceneNode::positionId == 0 ? 2 : 0];
                glm::mat4 finalMatrixModel = M[SceneNode::positionId];
                float interpolation = glm::clamp(p / d, 0.0f, 1.0f);
                ended = interpolation == 1.0f;
                float easedInterpolation = glm::smoothstep(0.0f, 1.0f, interpolation);

                // Linear interpolation for translation
                glm::vec3 initialTranslation = glm::vec3(M[1][3]);
                glm::vec3 finalTranslation = glm::vec3(finalMatrixModel[3]);
                glm::vec3 interpolatedTranslation = glm::mix(initialTranslation, finalTranslation, easedInterpolation);

                //Linear interpolation for rotation
                glm::quat initialRotation = glm::quat_cast(M[1]);
                glm::quat finalRotation = glm::quat_cast(finalMatrixModel);
                glm::quat interpolatedRotation = glm::lerp(initialRotation, finalRotation, easedInterpolation);

                // Update the model matrix
                M[1] = glm::translate(glm::mat4(1.0f), interpolatedTranslation) * glm::mat4_cast(interpolatedRotation);

                draw(modelMatrixId, colorId, glm::mat4(1.0f), shaders, true);
            }
            else {
                ended = false;
                draw(modelMatrixId, colorId, glm::mat4(1.0f), shaders, true);
            }
        }

        // animate/draw whole scene
        for (SceneNode* child : children) {
            ended = child->animate(modelMatrixId, colorId, paused, p, d);
        }

        return ended;
    }

    void addPosition(int pos, glm::mat4 m) {
        M[pos] = m;
    }
};

int SceneNode::positionId = 0;

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
    SceneNode root = SceneNode(nullptr, nullptr);

    const GLuint UBO_BP[2] = { 0, 1 };
    mgl::OrbitCamera* Cameras[2] = { nullptr, nullptr };
    int cameraId = 1;

    GLint ModelMatrixId;
    GLint ColorId;
    mgl::Mesh* Mesh = nullptr;
    std::vector<mgl::Mesh*> meshes;  // Vector to store multiple meshes

    float animationDuration = 3.0f; 
    float currentTime = 0.0f;
    bool isAnimating = false;
    bool isPaused = false;

    bool pressedKeys[GLFW_KEY_LAST];

    void createMeshes();
    void createShaderPrograms();
    void createCamera();
    void drawScene();

    // Animation functions
    void startAnimation();
    void pauseAnimation();
    void stopAnimation();
    void updateAnimation(float deltaTime);
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

///////////////////////////////////////////////////////////////////////// ANIMATION

void MyApp::startAnimation() {
    isAnimating = true;
    isPaused = false;
}

void MyApp::pauseAnimation() {
    isPaused = true;
}

void MyApp::stopAnimation() {
    currentTime = 0.0f;
    isAnimating = false;
    isPaused = false;
}

void MyApp::updateAnimation(float deltaTime) {
    if (!isPaused) {
        currentTime += deltaTime;
    }
    bool ended = root.animate(ModelMatrixId, ColorId, isPaused, currentTime, animationDuration);
    if (ended) {
        stopAnimation();
    }
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

    root = SceneNode(nullptr, Shaders);
    M = I;
    root.addPosition(0, M);
    root.addPosition(2, M);

    // Draw triangles
    SceneNode triangle1(triangleMesh, Shaders);
    triangle1.addPosition(0, M); // set box matrix model
    if (SceneNode::positionId == 0) {
        triangle1.addPosition(1, M); // set animation matrix equal to initial position matrix model
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, side, -side));
    M = T * R;
    triangle1.addPosition(2, M); // set tangram shape matrix model
    if (SceneNode::positionId == 2) {
        triangle1.addPosition(1, M); // set animation matrix equal to tangram shape matrix model
    }
    triangle1.color = glm::vec3(0.0f, 0.62f, 0.65f);
    root.addChild(&triangle1);

    SceneNode triangle2(triangleMesh, Shaders);
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight - hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    triangle2.addPosition(0, M);
    if (SceneNode::positionId == 0) {
        triangle2.addPosition(1, M);
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(-45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 0.0f, side));
    M = T * R;
    triangle2.addPosition(2, M);
    if (SceneNode::positionId == 2) {
        triangle2.addPosition(1, M);
    }
    triangle2.color = glm::vec3(0.92f, 0.28f, 0.15f);
    root.addChild(&triangle2);

    SceneNode triangle3(triangleMesh, Shaders);
    S = glm::scale(glm::vec3(sqrt(2), sqrt(2), 1));
    R = glm::rotate(glm::radians(135.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-2.0f * hypotenuse, side * sqrt(2), 0.0f));
    M = T * R * S;
    triangle3.addPosition(0, M);
    if (SceneNode::positionId == 0) {
        triangle3.addPosition(1, M);
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f));
    M = T * R * S;
    triangle3.addPosition(2, M);
    if (SceneNode::positionId == 2) {
        triangle3.addPosition(1, M);
    }
    triangle3.color = glm::vec3(0.43f, 0.23f, 0.75f);
    root.addChild(&triangle3);

    SceneNode triangle4(triangleMesh, Shaders);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(90.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    M = T * R * S;
    triangle4.addPosition(0, M);
    if (SceneNode::positionId == 0) {
        triangle4.addPosition(1, M);
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, side, 2.0f*hypotenuse));
    M = T * R * S;
    triangle4.addPosition(2, M);
    if (SceneNode::positionId == 2) {
        triangle4.addPosition(1, M);
    }
    triangle4.color = glm::vec3(0.80f, 0.05f, 0.4f);
    root.addChild(&triangle4);

    SceneNode triangle5(triangleMesh, Shaders);
    S = glm::scale(glm::vec3(2, 2, 1));
    R = glm::rotate(glm::radians(180.0f), glm::vec3(1, 0, 0));
    T = glm::translate(glm::vec3(0.0f, 2.0f * hypotenuse, 0.0f));
    M = T * R * S;
    triangle5.addPosition(0, M);
    if (SceneNode::positionId == 0) {
        triangle5.addPosition(1, M);
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0)) * glm::rotate(glm::radians(180.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(0.0f, 0.0f, 0.0f * hypotenuse));
    M = T * R * S;
    triangle5.addPosition(2, M);
    if (SceneNode::positionId == 2) {
        triangle5.addPosition(1, M);
    }
    triangle5.color = glm::vec3(0.06f, 0.51f, 0.95f);
    root.addChild(&triangle5);


    // draw square
    SceneNode square(squareMesh, Shaders);
    R = glm::rotate(glm::radians(45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-triangleHeight, triangleHeight, 0.0f));
    M = T * R;
    square.addPosition(0, M);
    if (SceneNode::positionId == 0) {
        square.addPosition(1, M);
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, 0.0f, 2.0f * hypotenuse));
    M = T * R;
    square.addPosition(2, M);
    if (SceneNode::positionId == 2) {
        square.addPosition(1, M);
    }
    square.color = glm::vec3(0.13f, 0.67f, 0.14f);
    root.addChild(&square);


    // draw parallelogram
    SceneNode parallelogram(parallelogramMesh, Shaders);
    R = glm::rotate(glm::radians(-45.0f), glm::vec3(0, 0, 1));
    T = glm::translate(glm::vec3(-1.5f * hypotenuse, triangleHeight, 0.0f));
    M = T * R;
    parallelogram.addPosition(0, M);
    if (SceneNode::positionId == 0) {
        parallelogram.addPosition(1, M);
    }
    R = glm::rotate(glm::radians(-90.0f), glm::vec3(0, 1, 0));
    T = glm::translate(glm::vec3(0.0f, 0.0f, side));
    M = T * R;
    parallelogram.addPosition(2, M);
    if (SceneNode::positionId == 2) {
        parallelogram.addPosition(1, M);
    }
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

        case GLFW_KEY_LEFT:
        case GLFW_KEY_RIGHT:
            pauseAnimation();
            break;
        }
    }
    else if (pressedKeys[key]) {
        switch (key) {
        case GLFW_KEY_LEFT:
            if (isAnimating or SceneNode::positionId != 0) {
                if (SceneNode::positionId != 0 && currentTime > 0.0f) {
                    currentTime = animationDuration - currentTime; // reverse animation time
                }
                SceneNode::positionId = 0;
                startAnimation();
            }
            break;
        case GLFW_KEY_RIGHT:
            if (isAnimating or SceneNode::positionId != 2) {
                if (SceneNode::positionId != 2 && currentTime > 0.0f) {
                    currentTime = animationDuration - currentTime; // reverse animation time
                }
                SceneNode::positionId = 2;
                startAnimation();
            }
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
    if (isAnimating) {
        updateAnimation(elapsed);
    }
    else {
        drawScene();
    }
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
