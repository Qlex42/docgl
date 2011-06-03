/* -------------------------------- . ---------------------------------------- .
| Filename : Tools.h                |  helper tools for docgl sample           |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */
# include "Docgl.h"

# define GLM_SWIZZLE_XYZW // for .xyz
# include <glm/glm.hpp> //vec3, vec4, ivec4, mat4
# include <glm/gtc/matrix_transform.hpp> //translate, rotate, scale, perspective
# include <cstdio> // for printf

//////////////////////// Shader Compiler ///////////////////////////////////////

docgl::GLErrorFlags buildAndLog(docgl::GLBuildableInterface& buildable, GLboolean& wasBuilt)
{
  wasBuilt = GL_FALSE;
  const docgl::GLErrorFlags errorFlags = buildable.build();
  GLsizei size = 0;
  if (buildable.getNumCharacteresOfLastLog(size).hasErrors())
    {jassertfalse;}
  if (buildable.hasBuildSucceed(wasBuilt).hasErrors())
    {jassertfalse;}
  if (size)
  {
    GLchar* log = new GLchar[size];
    if (buildable.getLastLog(size, NULL, log).hasErrors())
      {jassertfalse;}
    if (!wasBuilt)
      printf("--BUILD ERROR--\n%s\n", log);
    else if (size > 1)
      printf("--BUILD WARNING--\n%s\n", log);
    delete [] log;
  }
  return errorFlags;
}

docgl::GLErrorFlags validateAndLog(docgl::GLValidableInterface& validable, GLboolean& wasValidated)
{
  wasValidated = GL_FALSE;
  const docgl::GLErrorFlags errorFlags = validable.validate();
  GLsizei size = 0;
  if (validable.getNumCharacteresOfLastLog(size).hasErrors())
    {jassertfalse;}
  if (validable.hasValidationSucceed(wasValidated).hasErrors())
    {jassertfalse;}
  if (size)
  {
    GLchar* log = new GLchar[size];
    if (validable.getLastLog(size, NULL, log).hasErrors())
      {jassertfalse;}
    if (!wasValidated)
      printf("--VALIDATION ERROR--\n%s\n", log);
    else if (size > 1 && log[0]) // on NVidia PPO, size can be > 1 with log[0] == 0
      printf("--VALIDATION WARNING--\n%s\n", log);
    delete [] log;
  }
  return errorFlags;
}

docgl::GLErrorFlags buildShader(docgl::GLShaderObject& shader, GLenum shaderType, const GLchar* source, GLboolean& wasBuilt)
{
  docgl::GLErrorFlags errorFlags;
  errorFlags = shader.create(shaderType, 1, &source);
  if (errorFlags.hasErrors())
    return errorFlags;
  errorFlags = buildAndLog(shader, wasBuilt);
  if (errorFlags.hasErrors())
  {
    shader.destroy();
    return errorFlags;
  }
  if (!wasBuilt)
    shader.destroy();
  return docgl::GLErrorFlags::succeed;
}

docgl::GLErrorFlags buildShaderProgram(docgl::GLProgramObject& program,
                          const GLchar* vertexSource, const GLchar* fragmentSource,
                          const GLchar** vertexAttributes, bool separable = false /** can be true only when GLEW_ARB_separate_shader_objects is true*/)
{
  docgl::GLErrorFlags errorFlags;
  GLboolean wasBuilt;

  // vertex shader compilation
  docgl::GLShaderObject vertexShader(program.getContext());
  if (vertexSource)
  {
    errorFlags = buildShader(vertexShader, GL_VERTEX_SHADER, vertexSource, wasBuilt);
    if (errorFlags.hasErrors())
      return errorFlags;
    if (!wasBuilt)
      return docgl::GLErrorFlags::invalidValueFlag;
  }

  // fragment shader compilation
  docgl::GLShaderObject fragmentShader(program.getContext());
  if (fragmentSource)
  {
    errorFlags = buildShader(fragmentShader, GL_FRAGMENT_SHADER, fragmentSource, wasBuilt);
    if (errorFlags.hasErrors())
    {
      if (vertexSource)
        vertexShader.destroy();
      return errorFlags;
    }
    if (!wasBuilt)
    {
      if (vertexSource)
        vertexShader.destroy();
      return docgl::GLErrorFlags::invalidValueFlag;
    }
  }

  // program link
  errorFlags = program.create();
  if (separable)
    errorFlags.merge(program.setSeparable(GL_TRUE));
  if (errorFlags.hasErrors())
  {
    if (vertexSource)
      vertexShader.destroy();
    if (fragmentSource)
      fragmentShader.destroy();
    return errorFlags;
  }
  if (vertexSource)
    errorFlags.merge(program.linkToShader(vertexShader.getId()));
  if (fragmentSource)
    errorFlags.merge(program.linkToShader(fragmentShader.getId()));
  GLuint attributeIndex = 0;
  while (vertexAttributes && vertexAttributes[attributeIndex])
  {
    errorFlags.merge(program.linkVariableNameToAttributeLocation(attributeIndex, vertexAttributes[attributeIndex]));
    ++attributeIndex;
  }
  if (vertexSource)
    vertexShader.destroy();
  if (fragmentSource)
    fragmentShader.destroy();
  if (errorFlags.hasErrors())
    {program.destroy(); return errorFlags;}
  // program build
  errorFlags = buildAndLog(program, wasBuilt);
  if (errorFlags.hasErrors())
    {program.destroy(); return errorFlags;}
  return wasBuilt ? docgl::GLErrorFlags::succeed : docgl::GLErrorFlags::invalidValueFlag;
}

//////////////////////// GLPackedImage allocator ///////////////////////////////////////

bool createPackedImage(docgl::GLPackedImage& image, GLenum format, GLenum type, size_t pixelCount)
{
  jassert(!image.data); // overwritting existing data: potential memory leak
  if (!pixelCount)
    {jassertfalse; return false;} // count must not be 0

  const size_t pixelSize = image.getPixelSize(format, type);
  if (!pixelSize)
    {jassertfalse; return false;} // unable to define pixel type
  GLvoid* newData = new(std::nothrow) GLubyte[pixelSize * pixelCount];
  if (!newData)
    {jassertfalse; return false;} // not enougth memory
  image.set(format, type, newData);
  return true;
}

// call this only on a GLPackedImage created by the create methode
void destroyPackedImage(docgl::GLPackedImage& image)
{
  jassert(image.data); // destroy an unset GLPackedImage
  if (image.data)
  {
    delete [] (GLubyte*)image.data;
    image.data = NULL;
  }
}

//////////////////////// Math Tools ///////////////////////////////////////

struct Frame
{
  glm::vec3 origin;	  // Where am I?
  glm::vec3 forward;	// Where am I going?
  glm::vec3 up;		    // Which way is up?

  Frame()
    : origin(0.0, 0.0, 0.0)   // At origin
    , up(0.0, 1.0, 0.0)       // Up is up (+Y)
    , forward(0.0, 0.0, -1.0) // Forward is -Z (default OpenGL)
  {}

  glm::vec3 getZAxis() const
    {return forward;}
  glm::vec3 getYAxis() const
    {return up;}
  glm::vec3 getXAxis() const
    {return glm::cross(up, forward);}

  void moveForward(float delta)   // Move Forward (Move along front direction) (Z axis)
    {origin += forward * delta;}
  void moveUp(float delta)        // Move along up direction (Y axis)
    {origin += up * delta;};
  void moveRight(float delta)     // Move along right vector (X axis)
    {origin += getXAxis() * delta;}

  // Just assemble the matrix
  glm::mat4 getMatrix(bool rotationOnly = false)
  {
    return glm::mat4(glm::vec4(getXAxis(), 0),
                     glm::vec4(up, 0),
                     glm::vec4(forward, 0),
                     rotationOnly ? glm::vec4(0, 0, 0, 1) : glm::vec4(origin, 1));
  }

  // Rotate in world coordinates...
  void rotateWorld(float angle, float x, float y, float z)
  {
    // Create the Rotation matrix
    glm::mat4 rotMat = glm::rotate(glm::mat4(1), angle, glm::vec3(x, y, z));
    up = (glm::vec4(up, 0) * rotMat).xyz;
    forward = (glm::vec4(forward, 0) * rotMat).xyz;
  }

  // Assemble the camera matrix
  glm::mat4 getCameraMatrix(bool rotationOnly = false)
  {
    // Make rotation matrix
    // Z vector is reversed
    const glm::vec3 z = -forward;
    const glm::vec3 x = glm::cross(up, z);

    const glm::mat4 res = glm::transpose(glm::mat4(glm::vec4(x, 0),
                                   glm::vec4(up, 0),
                                   glm::vec4(z, 0),
                                   glm::vec4(0, 0, 0, 1)));
    // Apply translation too
    return rotationOnly ? res : glm::translate(res, -origin);
  }
};
