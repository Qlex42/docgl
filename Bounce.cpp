/* -------------------------------- . ---------------------------------------- .
| Filename : Bounce.cpp             | DOCGL3.3 sample file                     |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */
#include "Tools.h"
#include <Docgl/DocglWindow.h>

////////////////////////  OpenGL Context caching test //////////////////////////

class ClientWithTooMuchRowByteAligmentChangeContext : public docgl::GLDirectContext
{
public:
  ClientWithTooMuchRowByteAligmentChangeContext()
    : cachedPixelStorePackRowByteAligment(pixelStorePackRowByteAligment)
    , cachedPixelStoreUnPackRowByteAligment(pixelStoreUnPackRowByteAligment) {}

  virtual docgl::GLErrorFlags initialize()
  {
    const docgl::GLErrorFlags errorFlags = docgl::GLDirectContext::initialize();
    if (errorFlags.hasErrors())
      return errorFlags;
    if(!cachedPixelStorePackRowByteAligment.isConsistent())
      {jassertfalse; return docgl::GLErrorFlags::invalidValueFlag;}
    if(!cachedPixelStoreUnPackRowByteAligment.isConsistent())
      {jassertfalse; return docgl::GLErrorFlags::invalidValueFlag;}
    return errorFlags;
  }

  virtual docgl::GLRegister<GLint>& getPixelStoreRowByteAligment(bool packing)
    { if (packing) return cachedPixelStorePackRowByteAligment; else return cachedPixelStoreUnPackRowByteAligment;}

  docgl::GLCachedRegister<GLint> cachedPixelStorePackRowByteAligment;
  docgl::GLCachedRegister<GLint> cachedPixelStoreUnPackRowByteAligment;
};

////////////////////////  Some VBO with shader tests //////////////////////////

struct Globals : public OpenGLWindowCallback
{
  ClientWithTooMuchRowByteAligmentChangeContext context;
  docgl::GLBufferObject squareVertexBuffer;
  docgl::GLBufferObject squareTexCoordBuffer;
  docgl::GLVertexArrayObject squareVertexArray;
  docgl::GLTextureObject squareTexture;
  docgl::GLProgramObject squareProgram1;
  docgl::GLProgramObject squareProgram2;
  docgl::GLProgramPipelineObject squarePipeline;
  bool  sceneSetupComplete;

  GLfloat blockSize;
  GLfloat vVerts[12];
  GLfloat texCoord[8];
  bool wantExit;

  Globals()
    : squareVertexBuffer(context)
    , squareTexCoordBuffer(context)
    , squareVertexArray(context)
    , squareTexture(context)
    , squareProgram1(context)
    , squareProgram2(context)
    , squarePipeline(context)
    , sceneSetupComplete(false)
    , blockSize(0.1f)
    , wantExit(false)

  {

    const GLfloat initvVerts[12] = {-blockSize - 0.5f, -blockSize, 0.0f,
                         blockSize - 0.5f, -blockSize, 0.0f,
                         blockSize - 0.5f,  blockSize, 0.0f,
                        -blockSize - 0.5f,  blockSize, 0.0f};

    const GLfloat inittexCoord[8] = {0.0f, 0.0f,
                                     1.0f, 0.0f,
                                     1.0f, 1.0f,
                                     0.0f, 1.0f};

    memcpy(vVerts, initvVerts, sizeof(initvVerts));
    memcpy(texCoord, inittexCoord, sizeof(inittexCoord));
  }

///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context.
// This is the first opportunity to do any OpenGL related tasks.
void SetupRC()
{
  // Black background
  bool succeed;
  jassert(context.getClearColor().isDefaultValue());
  succeed = context.getClearColor().setValue(docgl::GLColor(0.0f, 0.0f, 1.0f, 1.0f)).hasSucceed();
  jassert(succeed);

  // Load up triangles and mapping coordinate with VBO
  succeed = squareVertexBuffer.create(sizeof(GLfloat) * 3 * 4, vVerts, GL_DYNAMIC_DRAW).hasSucceed();
  jassert(succeed);
  succeed = squareTexCoordBuffer.create(sizeof(GLfloat) * 2 * 4, texCoord, GL_DYNAMIC_DRAW).hasSucceed();
  jassert(succeed);
  succeed = squareVertexArray.create().hasSucceed();
  jassert(succeed);
  succeed = squareVertexArray.linkAttributeToBuffer(0, 3, GL_FLOAT, GL_FALSE, squareVertexBuffer.getId()).hasSucceed();
  jassert(succeed);
  succeed = squareVertexArray.linkAttributeToBuffer(1, 2, GL_FLOAT, GL_FALSE, squareTexCoordBuffer.getId()).hasSucceed();
  jassert(succeed);

  // compile shader with attribute
  static const GLchar* identityVertexShader =
    "attribute vec4 vVertex;"
    "attribute vec2 vTexCoord0;"
    "varying vec2 vTex;"
	  ""
    "void main(void) "
    "  {vTex = vTexCoord0; gl_Position = vVertex;}";

  static const GLchar* coloredTextureFragmentShader =
    "varying vec2 vTex;"
    "uniform sampler2D textureUnit0;"
    ""
    "void main(void) "
    "  {gl_FragColor = texture2D(textureUnit0, vTex);}";

  static const GLchar* coloredFragmentShader =
    "uniform vec4 vColor;"
    ""
    "void main(void) "
    "  {gl_FragColor = vColor;}";

  static const GLchar* vertexAttributes[] = {"vVertex", "vTexCoord0", NULL};

  succeed = squarePipeline.create().hasSucceed();
  jassert(succeed);

  succeed = buildShaderProgram(squareProgram1, identityVertexShader, coloredTextureFragmentShader, vertexAttributes, true).hasSucceed();
  jassert(succeed);
  succeed = buildShaderProgram(squareProgram2, NULL, coloredFragmentShader, NULL, true).hasSucceed();
  jassert(succeed);

  succeed = squarePipeline.linkToProgram(GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, squareProgram1.getId()).hasSucceed();
  jassert(succeed)
  succeed = squarePipeline.linkToProgram(GL_FRAGMENT_SHADER_BIT, squareProgram2.getId()).hasSucceed();
  jassert(succeed);
  // set uniforms
  GLint vColorLocation = -1;
  succeed = squareProgram2.getUniformVariableLocation("vColor", vColorLocation).hasSucceed();
  jassert(succeed);
  GLfloat vWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};
  //  succeed = squareProgram1.setUniformValue(vColorLocation, 4, 1, vWhite).hasSucceed();
  succeed = squarePipeline.setProgramUniformValue(squareProgram2.getId(), vColorLocation, 4, 1, vWhite).hasSucceed();
  jassert(succeed);

  GLint textureUnit0Location = -1;
  succeed = squareProgram1.getUniformVariableLocation("textureUnit0", textureUnit0Location).hasSucceed();
  jassert(succeed);
  GLint textureUnit = 0;
  //succeed = squareProgram1.setUniformValue(textureUnit0Location, 1, 1, &textureUnit).hasSucceed();
  succeed = squarePipeline.setProgramUniformValue(squareProgram1.getId(), textureUnit0Location, 1, 1, &textureUnit).hasSucceed();
  jassert(succeed);

  // texture [red / green / cyan / magenta]
  docgl::GLPackedImage imageData;
  GLubyte buffer[2][2][4] = {{{255, 0, 0, 255}, {0, 255, 0, 255}}, {{0, 255, 255, 255}, {255, 0, 255, 255}}};
  imageData.set(GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  succeed = squareTexture.create2D(GL_RGBA8, 2, 2, &imageData).hasSucceed();
  jassert(succeed);
  squareTexture.setMagnificationFilter(GL_NEAREST);
  squareTexture.setMinificationFilter(GL_LINEAR);
  squareTexture.setHorizontalWrapMode(GL_CLAMP_TO_EDGE);
  squareTexture.setVerticalWrapMode(GL_CLAMP_TO_EDGE);

  sceneSetupComplete = true;
}

// Respond to arrow keys by moving the camera frame of reference
void BounceFunction()
{
  static GLfloat xDir = 1.0f;
  static GLfloat yDir = 1.0f;

  GLfloat stepSize = 0.005f;

  GLfloat blockX = vVerts[0];   // Upper left X
  GLfloat blockY = vVerts[7];  // Upper left Y

  blockY += stepSize * yDir;
  blockX += stepSize * xDir;

  // Collision detection
  if(blockX < -1.0f) { blockX = -1.0f; xDir *= -1.0f; }
  if(blockX > (1.0f - blockSize * 2)) { blockX = 1.0f - blockSize * 2; xDir *= -1.0f; }
  if(blockY < -1.0f + blockSize * 2)  { blockY = -1.0f + blockSize * 2; yDir *= -1.0f; }
  if(blockY > 1.0f) { blockY = 1.0f; yDir *= -1.0f; }

  // Recalculate vertex positions
  vVerts[0] = blockX;
  vVerts[1] = blockY - blockSize*2;

  vVerts[3] = blockX + blockSize*2;
  vVerts[4] = blockY - blockSize*2;

  vVerts[6] = blockX + blockSize*2;
  vVerts[7] = blockY;

  vVerts[9] = blockX;
  vVerts[10] = blockY;

  const bool succeed = squareVertexBuffer.set(sizeof(GLfloat) * 3 * 4, vVerts).hasSucceed();
  jassert(succeed);
}

virtual void draw()
{
  if(!sceneSetupComplete)
    return;

  // Clear the window with current clearing color
  context.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  bool succeed;
  GLboolean wasValidated;

  succeed = context.getActiveTextureBind(GL_TEXTURE_2D).setValue(squareTexture.getId()).hasSucceed();
  jassert(succeed);

  succeed = context.getActiveProgramBind().setValue(squareProgram1.getId()).hasSucceed();
  jassert(succeed);
  succeed = validateAndLog(squareProgram1, wasValidated).hasSucceed();
  jassert(succeed);
  jassert(wasValidated);

  //succeed = squarePipeline.setActiveProgram(squareProgram1.getId()).hasSucceed();
  //jassert(succeed);
  //succeed = squarePipeline.setActiveProgram(squareProgram2.getId()).hasSucceed();
  //jassert(succeed);

  //succeed = context.getActiveProgramPipelineBind().setValue(squarePipeline.getId()).hasSucceed();
  //jassert(succeed);
  //succeed = validateAndLog(squarePipeline, wasValidated).hasSucceed();
  //jassert(succeed);
  //jassert(wasValidated);
  //succeed = squareProgram1.hasValidationSucceed(wasValidated).hasSucceed();
  //jassert(succeed);
  //jassert(wasValidated);
  //succeed = squareProgram2.hasValidationSucceed(wasValidated).hasSucceed();
  //jassert(succeed);
  //jassert(wasValidated);

  // can also be GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY,
  // GL_TRIANGLE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY
  succeed = squareVertexArray.draw(GL_TRIANGLE_FAN, 0, 4).hasSucceed();
  jassert(succeed);

}

///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.
virtual void resized(int w, int h)
{
  context.getActiveViewport().setValue(docgl::GLRegion(w, h));
}

///////////////////////////////////////////////////////////////////////////////
// Do your cleanup here. Free textures, display lists, buffer objects, etc.
 void ShutdownRC()
{
  if (sceneSetupComplete == true)
  {
    sceneSetupComplete = false;

    squareVertexArray.destroy();
    squareVertexBuffer.destroy();
    squareTexCoordBuffer.destroy();
    squareTexture.destroy();
    squareProgram1.destroy();
    squareProgram2.destroy();
    squarePipeline.destroy();
    context.getClearColor().setValue(docgl::GLColor(0.0f));
  }
}

  virtual void keyPressed(unsigned char key)
  {
    bool invalidateMesh = false;
    if (key == 0x1B) // Escape
      wantExit = true;
  }

  virtual void closed()
    {wantExit = true;}
};

int bounceMain(const char* szClassName, int x, int y, int width, int height)
{
  Globals g;
  OpenGLWindow window(g.context);

  if (!window.create(g, x, y, width, height, szClassName))
  {
    printf("Error: 0x%08X Unable to create OpenGL Window.\n", (int)GetLastError());
    jassertfalse;
    return 1;
  }
  if (g.context.initialize().hasErrors())
  {
    window.destroy();
    jassertfalse;
    fprintf(stderr, "GLContext initialize error\n");
    return 2;
  }

  // show environment
  printf("GL vendor: %s\n", g.context.getVendorName());
  printf("GL render: %s\n", g.context.getRenderName());
  printf("GL version: %s\n", g.context.getVersion());
  printf("GL shader version: %s\n", g.context.getShadingLanguageVersion());

  // show some limits
  printf("Texture Units count: %u\n", g.context.getNumTextureUnits());
  printf("Vertex attributes count: %u\n", g.context.getNumVertexAttributes());
  printf("Max texture size: %zu\n", g.context.getMaxTextureSize());
  printf("Points [%f, %f] +- %f\n", g.context.getPointSmallestSize(),
                                    g.context.getPointLargestSize(),
                                    g.context.getPointSizeGranularity());

  printf("Max viewport dimensions: %u, %u\n", g.context.getMaxViewportWidth(), g.context.getMaxViewportHeight());

  // show hits
  printf("Hints: %u, %u, %u, %u\n", g.context.getLineSmoothHint().isDefaultValue(),
                                    g.context.getPolygonSmoothHint().isDefaultValue(),
                                    g.context.getTextureCompressionQualityHint().isDefaultValue(),
                                    g.context.getFragmentShaderDerivativeAccuracyHint().isDefaultValue());

  jassert(g.context.getPointSizeProgrammable().isDefaultValue());
  jassert(g.context.getPixelStore(false).isDefaultValue());

  const GLsizei textureSize = (GLsizei)(g.context.getMaxTextureSize() / 2);
  docgl::GLPackedImage imageData;
  createPackedImage(imageData, GL_RGBA, GL_UNSIGNED_BYTE, textureSize * textureSize);
  docgl::GLTextureObject textureObject(g.context);

  for (int i = 0; i < 4; ++i)
  {
    docgl::GLErrorFlags errorflags = textureObject.create2D(GL_RGBA8, textureSize, textureSize, &imageData);
    if (errorflags.hasSucceed())
    {
      textureObject.setMinificationFilter(GL_LINEAR);
      printf("Texture:%i dimension:%ix%i msaa:%i fmt:0x%04X\n", i, textureObject.getWidth(), textureObject.getHeight(), textureObject.getNumSamplesPerTexel(), textureObject.getInternalFormat());
      printf("stencil component size: %i compressed:%i texture depth:%i\n", textureObject.getNumBitsForStencil(), textureObject.isCompressed(), textureObject.getDepth());
      textureObject.destroy();
    }
    else
      jassertfalse;
    GLubyte* data = (GLubyte*)imageData.data;
    data[0]++;
  }
  destroyPackedImage(imageData);

  g.SetupRC();

  OpenGLWindow* windowsLists = &window;
  EventDispatcher dispatcher(&windowsLists, 1);
  do
  {
    if (!dispatcher.dispatchNextEvent())
    {
      g.draw(); // no message to dispatch: render the scene.
	  	window.swapBuffers(); // Flush drawing commands
      g.BounceFunction(); // compute changes
    }
  }
  while (!g.wantExit);
  g.ShutdownRC();

  window.destroy();
  return 0;
}

