////////////////////////////////////////////////////////////////////////////////
//
// Shader Program Class
//
// Copyright (c)2022-23 by Carlos Martinho
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MGL_SHADER_HPP
#define MGL_SHADER_HPP

#include <GL/glew.h>

#include <iostream>
#include <map>
#include <string>

namespace mgl {

class ShaderProgram;

////////////////////////////////////////////////////////////////// ShaderProgram

class ShaderProgram {
 public:
  GLuint ProgramId;

  struct ShaderInfo {
    GLuint index;
  };
  std::map<GLenum, GLuint> Shaders;

  struct AttributeInfo {
    GLuint index;
  };
  std::map<std::string, AttributeInfo> Attributes;

  struct UniformInfo {
    GLint index;
  };
  std::map<std::string, UniformInfo> Uniforms;

  struct UboInfo {
    GLuint index;
    GLuint binding_point;
  };
  std::map<std::string, UboInfo> Ubos;

  ShaderProgram();
  ~ShaderProgram();
  void addShader(const GLenum shader_type, const std::string &filename);
  void addAttribute(const std::string &name, const GLuint index);
  bool isAttribute(const std::string &name);
  void addUniform(const std::string &name);
  bool isUniform(const std::string &name);
  void addUniformBlock(const std::string &name, const GLuint binding_point);
  bool isUniformBlock(const std::string &name);
  void create();
  void bind();
  void unbind();

 private:
  const std::string read(const std::string &filename);
  const GLuint checkCompilation(const GLuint shader_id,
                                const std::string &filename);
  void checkLinkage();
};

////////////////////////////////////////////////////////////////////////////////
}  // namespace mgl

#endif /* MGL_SHADER_HPP */
