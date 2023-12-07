////////////////////////////////////////////////////////////////////////////////
//
// Mesh Loader Class
//
// Copyright (c)2022-23 by Carlos Martinho
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MGL_MESH_HPP
#define MGL_MESH_HPP

#include <GL/glew.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "./mglScenegraph.hpp"

namespace mgl {

class Mesh;

#define CREATE_BITANGENT

/////////////////////////////////////////////////////////////////////////// Mesh

class Mesh : public IDrawable {
 public:
  static const GLuint INDEX = 0;
  static const GLuint POSITION = 1;
  static const GLuint NORMAL = 2;
  static const GLuint TEXCOORD = 3;
  static const GLuint TANGENT = 4;
#ifdef CREATE_BITANGENT
  static const GLuint BITANGENT = 5;
#endif

  Mesh();
  ~Mesh();

  void setAssimpFlags(unsigned int flags);
  void joinIdenticalVertices();
  void generateNormals();
  void generateSmoothNormals();
  void generateTexcoords();
  void calculateTangentSpace();
  void flipUVs();

  void create(const std::string &filename);
  void draw() override;

  bool hasNormals();
  bool hasTexcoords();
  bool hasTangentsAndBitangents();

 private:
  GLuint VaoId;
  unsigned int AssimpFlags;
  bool NormalsLoaded, TexcoordsLoaded, TangentsAndBitangentsLoaded;

  struct MeshData {
    unsigned int nIndices = 0;
    unsigned int baseIndex = 0;
    unsigned int baseVertex = 0;
  };
  std::vector<MeshData> Meshes;

  std::vector<glm::vec3> Positions;
  std::vector<glm::vec3> Normals;
  std::vector<glm::vec2> Texcoords;
  std::vector<glm::vec3> Tangents;
#ifdef CREATE_BITANGENT
  std::vector<glm::vec3> Bitangents;
#endif
  std::vector<unsigned int> Indices;

  void processScene(const aiScene *scene);
  void processMesh(const aiMesh *mesh);
  void createBufferObjects();
  void destroyBufferObjects();
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace mgl

#endif /* MGL_MESH_HPP  */
