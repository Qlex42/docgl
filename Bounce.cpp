/* -------------------------------- . ---------------------------------------- .
| Filename : Bounce.cpp             | DOCGL3.3 sample file                     |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */
#include "Tools.h"
#include "Docwgl.h"

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

struct Globals
{
  ClientWithTooMuchRowByteAligmentChangeContext context;
  docgl::GLBufferObject squareVertexBuffer;
  docgl::GLBufferObject squareTexCoordBuffer;
  docgl::GLVertexArrayObject squareVertexArray;
  docgl::GLTextureObject squareTexture;
  docgl::GLProgramObject squareProgram1;
  docgl::GLProgramObject squareProgram2;
  docgl::GLProgramPipelineObject squarePipeline;
  docwgl::OpenGLWindow openGLWindow;
  bool  sceneSetupComplete;

  GLfloat blockSize;
  GLfloat vVerts[12];
  GLfloat texCoord[8];

  Globals()
    : squareVertexBuffer(context)
    , squareTexCoordBuffer(context)
    , squareVertexArray(context)
    , squareTexture(context)
    , squareProgram1(context)
    , squareProgram2(context)
    , squarePipeline(context)
    , openGLWindow(context)
    , sceneSetupComplete(false)
    , blockSize(0.1f)
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

};

///////////////////////////////////////////////////////////////////////////////
// This function does any needed initialization on the rendering context. 
// This is the first opportunity to do any OpenGL related tasks.
static void SetupRC(Globals& g)
{
  // Black background
  bool succeed;
  jassert(g.context.getClearColor().isDefaultValue());
  succeed = g.context.getClearColor().setValue(docgl::GLColor(0.0f, 0.0f, 1.0f, 1.0f)).hasSucceed();
  jassert(succeed);

  // Load up triangles and mapping coordinate with VBO
  succeed = g.squareVertexBuffer.create(sizeof(GLfloat) * 3 * 4, g.vVerts, GL_DYNAMIC_DRAW).hasSucceed();
  jassert(succeed);
  succeed = g.squareTexCoordBuffer.create(sizeof(GLfloat) * 2 * 4, g.texCoord, GL_DYNAMIC_DRAW).hasSucceed();
  jassert(succeed);
  succeed = g.squareVertexArray.create().hasSucceed();
  jassert(succeed);
  succeed = g.squareVertexArray.linkAttributeToBuffer(0, 3, GL_FLOAT, GL_FALSE, g.squareVertexBuffer.getId()).hasSucceed();
  jassert(succeed);
  succeed = g.squareVertexArray.linkAttributeToBuffer(1, 2, GL_FLOAT, GL_FALSE, g.squareTexCoordBuffer.getId()).hasSucceed();
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

  succeed = g.squarePipeline.create().hasSucceed();
  jassert(succeed);

  succeed = buildShaderProgram(g.squareProgram1, identityVertexShader, coloredTextureFragmentShader, vertexAttributes, true).hasSucceed();
  jassert(succeed);
  succeed = buildShaderProgram(g.squareProgram2, NULL, coloredFragmentShader, NULL, true).hasSucceed();
  jassert(succeed);

  succeed = g.squarePipeline.linkToProgram(GL_VERTEX_SHADER_BIT | GL_FRAGMENT_SHADER_BIT, g.squareProgram1.getId()).hasSucceed();
  jassert(succeed)
  succeed = g.squarePipeline.linkToProgram(GL_FRAGMENT_SHADER_BIT, g.squareProgram2.getId()).hasSucceed();
  jassert(succeed);
  // set uniforms
  GLint vColorLocation = -1;
  succeed = g.squareProgram2.getUniformVariableLocation("vColor", vColorLocation).hasSucceed();
  jassert(succeed);
  GLfloat vWhite[] = {1.0f, 1.0f, 1.0f, 1.0f};
  //  succeed = g.squareProgram1.setUniformValue(vColorLocation, 4, 1, vWhite).hasSucceed();
  succeed = g.squarePipeline.setProgramUniformValue(g.squareProgram2.getId(), vColorLocation, 4, 1, vWhite).hasSucceed();
  jassert(succeed);

  GLint textureUnit0Location = -1;
  succeed = g.squareProgram1.getUniformVariableLocation("textureUnit0", textureUnit0Location).hasSucceed();
  jassert(succeed);
  GLint textureUnit = 0;
  //succeed = g.squareProgram1.setUniformValue(textureUnit0Location, 1, 1, &textureUnit).hasSucceed();
  succeed = g.squarePipeline.setProgramUniformValue(g.squareProgram1.getId(), textureUnit0Location, 1, 1, &textureUnit).hasSucceed();
  jassert(succeed);
  
  // texture [red / green / cyan / magenta]
  docgl::GLPackedImage imageData;
  GLubyte buffer[2][2][4] = {{{255, 0, 0, 255}, {0, 255, 0, 255}}, {{0, 255, 255, 255}, {255, 0, 255, 255}}};
  imageData.set(GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  succeed = g.squareTexture.create2D(GL_RGBA8, 2, 2, &imageData).hasSucceed();
  jassert(succeed);
  g.squareTexture.setMagnificationFilter(GL_NEAREST);
  g.squareTexture.setMinificationFilter(GL_LINEAR);
  g.squareTexture.setHorizontalWrapMode(GL_CLAMP_TO_EDGE);
  g.squareTexture.setVerticalWrapMode(GL_CLAMP_TO_EDGE);

  g.sceneSetupComplete = true;
}

// Respond to arrow keys by moving the camera frame of reference
void BounceFunction(Globals& g)
{
  static GLfloat xDir = 1.0f;
  static GLfloat yDir = 1.0f;

  GLfloat stepSize = 0.005f;

  GLfloat blockX = g.vVerts[0];   // Upper left X
  GLfloat blockY = g.vVerts[7];  // Upper left Y

  blockY += stepSize * yDir;
  blockX += stepSize * xDir;

  // Collision detection
  if(blockX < -1.0f) { blockX = -1.0f; xDir *= -1.0f; }
  if(blockX > (1.0f - g.blockSize * 2)) { blockX = 1.0f - g.blockSize * 2; xDir *= -1.0f; }
  if(blockY < -1.0f + g.blockSize * 2)  { blockY = -1.0f + g.blockSize * 2; yDir *= -1.0f; }
  if(blockY > 1.0f) { blockY = 1.0f; yDir *= -1.0f; }

  // Recalculate vertex positions
  g.vVerts[0] = blockX;
  g.vVerts[1] = blockY - g.blockSize*2;

  g.vVerts[3] = blockX + g.blockSize*2;
  g.vVerts[4] = blockY - g.blockSize*2;

  g.vVerts[6] = blockX + g.blockSize*2;
  g.vVerts[7] = blockY;

  g.vVerts[9] = blockX;
  g.vVerts[10] = blockY;

  const bool succeed = g.squareVertexBuffer.set(sizeof(GLfloat) * 3 * 4, g.vVerts).hasSucceed();
  jassert(succeed);
}

///////////////////////////////////////////////////////////////////////////////
// Called to draw scene

static void RenderScene(Globals& g)
{
  if(!g.sceneSetupComplete)
    return;

  // Clear the window with current clearing color
  g.context.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  bool succeed;
  GLboolean wasValidated;

  succeed = g.context.getActiveTextureBind(GL_TEXTURE_2D).setValue(g.squareTexture.getId()).hasSucceed();
  jassert(succeed);

  succeed = g.context.getActiveProgramBind().setValue(g.squareProgram1.getId()).hasSucceed();
  jassert(succeed);
  succeed = validateAndLog(g.squareProgram1, wasValidated).hasSucceed();
  jassert(succeed);
  jassert(wasValidated);

  //succeed = g.squarePipeline.setActiveProgram(g.squareProgram1.getId()).hasSucceed();
  //jassert(succeed);
  //succeed = g.squarePipeline.setActiveProgram(g.squareProgram2.getId()).hasSucceed();
  //jassert(succeed);

  //succeed = g.context.getActiveProgramPipelineBind().setValue(g.squarePipeline.getId()).hasSucceed();
  //jassert(succeed);
  //succeed = validateAndLog(g.squarePipeline, wasValidated).hasSucceed();
  //jassert(succeed);
  //jassert(wasValidated);
  //succeed = g.squareProgram1.hasValidationSucceed(wasValidated).hasSucceed();
  //jassert(succeed);
  //jassert(wasValidated);
  //succeed = g.squareProgram2.hasValidationSucceed(wasValidated).hasSucceed();
  //jassert(succeed);
  //jassert(wasValidated);

  // can also be GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_LINE_STRIP_ADJACENCY, GL_LINES_ADJACENCY,
  // GL_TRIANGLE_STRIP, GL_TRIANGLES, GL_TRIANGLE_STRIP_ADJACENCY
  succeed = g.squareVertexArray.draw(GL_TRIANGLE_FAN, 0, 4).hasSucceed();
  jassert(succeed);

  // Flush drawing commands
  //glFlush(); // instead of swapBuffers if there is no double buffering.
  g.openGLWindow.swapBuffers();

  BounceFunction(g);
}

///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.
static void ChangeSize(Globals& g, int w, int h)
{
  if (!g.context.isInitialized()) 
    return; // ChangeSize can be called before g.context initializing (durring window creation)

  g.context.getActiveViewport().setValue(docgl::GLRegion(w, h));
  RenderScene(g);  // redraw on window resize;
}

///////////////////////////////////////////////////////////////////////////////
// Do your cleanup here. Free textures, display lists, buffer objects, etc.
static void ShutdownRC(Globals& g)
{
  if (g.sceneSetupComplete == true)
  {
    g.sceneSetupComplete = false;

    g.squareVertexArray.destroy();
    g.squareVertexBuffer.destroy();
    g.squareTexCoordBuffer.destroy();
    g.squareTexture.destroy();
    g.squareProgram1.destroy();
    g.squareProgram2.destroy();
    g.squarePipeline.destroy();
    g.context.getClearColor().setValue(docgl::GLColor(0.0f));
  }
}

///////////////////////////////////////////////////////////////////////////////
// Callback functions to handle all window functions this app cares about.
// Once complete, pass message on to next app in the hook chain.
static LRESULT CALLBACK WndProc(HWND	hWnd,		    // Handle For This Window
                         UINT	uMsg,		    // Message For This Window
                         WPARAM	wParam,		// Additional Message Information
                         LPARAM	lParam)		// Additional Message Information
{
  unsigned int key = 0;

  // Handle relevant messages individually
  switch(uMsg)
  {
  case WM_ERASEBKGND:
    return 0; // http://www.opengl.org/pipeline/article/vol003_7/ avoid GDI clearing the OpenGL windows background
  case WM_SIZE:
    {
      Globals* g = (Globals*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
      if (g)
        ChangeSize(*g, LOWORD(lParam), HIWORD(lParam));
    }
    break;
  case WM_CLOSE :
    PostQuitMessage(0);
    return 0;
  case WM_KEYDOWN:
    key = (unsigned int)wParam;
    if (key == 27)
      PostQuitMessage(0);
    return 0;
  default:
    break;
  }
  // Pass All Unhandled Messages To DefWindowProc
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int bounceMain(LPCTSTR szClassName)
{
  Globals g;

  HINSTANCE module = GetModuleHandle(NULL);
  if(!docwgl::registerOpenGLWindowClass(szClassName, module, (WNDPROC)WndProc, LoadCursor(NULL, IDC_ARROW)))
  {
    printf("Unable to register window class.\n");
    return 1;
  }

  // Specify the important attributes we care about
  const int pixelFormatAttributes[] = {
    WGL_SUPPORT_OPENGL_ARB, 1, // Must support OGL rendering
    WGL_DRAW_TO_WINDOW_ARB, 1, // pf that can run a window
    WGL_ACCELERATION_ARB,   WGL_FULL_ACCELERATION_ARB, // must be HW accelerated
    WGL_RED_BITS_ARB,       8, // 8 bits of red precision in window
    WGL_GREEN_BITS_ARB,     8, // 8 bits of green precision in window
    WGL_BLUE_BITS_ARB,      8, // 8 bits of blue precision in window
    //WGL_ALPHA_BITS_ARB, 8, 
    //WGL_COLOR_BITS_ARB,     24, // 8 bits of each R, G and B
    WGL_DEPTH_BITS_ARB,     16, // 16 bits of depth precision for window
    WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB, // pf should be RGBA type
    WGL_DOUBLE_BUFFER_ARB,	GL_TRUE, // Double buffered context
    WGL_SWAP_METHOD_ARB,    WGL_SWAP_EXCHANGE_ARB, // double buffer swap (more speed ?)
    WGL_SAMPLE_BUFFERS_ARB, GL_TRUE, // MSAA on
    WGL_SAMPLES_ARB,        8, // 8x MSAA 
    0
  }; // NULL termination
  const int contextAttributes[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB,  3,
    WGL_CONTEXT_MINOR_VERSION_ARB,  3,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    0};

  CONST RECT clientRect = {0, 0, 800, 600};
  DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;
  DWORD extendedWindowStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

  // ajust windows size and position depending to wanted client size and position.
  RECT windowRect = clientRect;
  AdjustWindowRectEx(&windowRect, windowStyle, FALSE, extendedWindowStyle);
  OffsetRect(&windowRect, clientRect.left - windowRect.left, clientRect.top - windowRect.top);

  if (!g.openGLWindow.create(szClassName, windowRect, windowStyle, 
    extendedWindowStyle, pixelFormatAttributes, contextAttributes))
  {
    printf("Error: 0x%08X Unable to create OpenGL Window.\n", (int)GetLastError());
    g.openGLWindow.destroy();
    return 2;
  }

  SetLastError(0);
  SetWindowLongPtr(g.openGLWindow.hWnd, GWLP_USERDATA, (LONG_PTR)&g);
  jassert(GetLastError() == 0);

  if (g.context.initialize().hasErrors())
  {
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
  printf("Max texture size: %u\n", g.context.getMaxTextureSize());
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

  const size_t textureSize = g.context.getMaxTextureSize() / 2;
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
  
  SetupRC(g);
  BOOL threadCanLoop;
  do 
  { 
    if (!docwgl::dispatchNextThreadMessage(threadCanLoop) && threadCanLoop)
      RenderScene(g); // no message to dispatch: render the scene.
    Sleep(0); // yeld to other thread -> check if it's realy needed.
  }
  while (threadCanLoop);
  ShutdownRC(g);

  g.openGLWindow.destroy();
  // Delete the window class
  UnregisterClass(szClassName, module);
  return 0;
}

