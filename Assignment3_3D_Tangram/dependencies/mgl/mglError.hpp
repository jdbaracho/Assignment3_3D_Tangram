////////////////////////////////////////////////////////////////////////////////
//
// OpenGL Error Handling
//
// Copyright (c)2022-23 by Carlos Martinho
//
////////////////////////////////////////////////////////////////////////////////

#ifndef MGL_ERROR_HPP
#define MGL_ERROR_HPP

#include <iostream>
#include <string>

//////////////////////////////////////////////////////////// Errors (OpenGL 2.0)

void checkOpenGLError(const std::string &function, const std::string &file,
                      int line);

#ifdef DEBUG
#define MGL_CHECK checkOpenGLError(__FUNCTION__, __FILE__, __LINE__);
#else
#define MGL_CHECK
#endif

////////////////////////////////////////////////////// Debug Output (OpenGL 4.3)

void setupDebugOutput();

#ifdef DEBUG
#define MGL_DEBUG setupDebugOutput();
#else
#define MGL_DEBUG
#endif

////////////////////////////////////////////////////////////////////////////////
#endif /* MGL_ERROR_HPP */
