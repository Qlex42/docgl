# define  _USE_MATH_DEFINES
# include "Tools.h"
# include "Docgl.h"
# include "Docwgl.h"

struct SuperFormula3D
  {float a, b, m, n1, n2, n3;};
static const float step = 0.05f;
enum {superFormulaNumVertices = 7938}; // depends from step

struct Globals
{
  docgl::GLDirectContext context;
  docgl::GLBufferObject superFormulaVertexBuffer;
  docgl::GLVertexArrayObject superFormulaVertexArray;
  docgl::GLProgramObject flatColorTransformShader;
  docwgl::OpenGLWindow openGLWindow;
  GLint vColorLocation;
  GLint mvpMatrixLocation;
  GLint getColorFunctionLocation;
  GLuint getUniformColorIndex;
  GLuint getUniformColorInvIndex;
  glm::mat4         projectionMatrix;
  Frame			  	    cameraFrame;
  Frame             objectFrame;
  int nStep;
  SuperFormula3D superFormula3d;
  bool invertCoordinate;
  bool cullFace;
  bool depthTest;
  bool invertColor;

  Globals()
    : superFormulaVertexBuffer(context)
    , superFormulaVertexArray(context)
    , flatColorTransformShader(context)
    , openGLWindow(context)
    , vColorLocation(-1)
    , mvpMatrixLocation(-1)
    , getColorFunctionLocation(-1)
    , getUniformColorIndex(GL_INVALID_INDEX)
    , getUniformColorInvIndex(GL_INVALID_INDEX)
    , nStep(0)
    , invertCoordinate(false)
    , cullFace(false)
    , depthTest(true)
    , invertColor(false)
    {
      static const SuperFormula3D formula = {1.f, 1.f, 4.f, 12.f, 15.f, 15.f};
      superFormula3d = formula;
    }
};

void constructSuperMesh(Globals& g, const SuperFormula3D& s, bool invertCoordinate)
{
  GLfloat computeBuffer[superFormulaNumVertices][3];

  int veccount = 0;
  if (invertCoordinate)
  {
    for (float j = float(-M_PI / 2); j < M_PI / 2; j += step)
      for (float i = float(-M_PI); i < M_PI; i += step)
      {
        const float raux1 = pow(abs(1.f / s.a * cos(s.m * i / 4.f)), s.n2) + pow(abs(1.f / s.b * sin(s.m * i / 4.f)), s.n3);
        const float r1 = pow(abs(raux1), (-1.f / s.n1));
        const float raux2 = pow(abs(1.f / s.a * cos(s.m * j / 4.f)), s.n2) + pow(abs(1.f / s.b * sin(s.m * j / 4.f)), s.n3);
        const float r2 = pow(abs(raux2), (-1.f / s.n1));
        computeBuffer[veccount][0] = r1 * cos(i) * r2 * cos(j);
        computeBuffer[veccount][1] = r1 * sin(i) * r2 * cos(j);
        computeBuffer[veccount][2] = r2 * sin(j);
        ++veccount;
      }
  }
  else
  {
    for (float i = float(-M_PI); i < M_PI; i += step)
      for (float j = float(-M_PI / 2); j < M_PI / 2; j += step)
      {
        const float raux1 = pow(abs(1.f / s.a * cos(s.m * i / 4.f)), s.n2) + pow(abs(1.f / s.b * sin(s.m * i / 4.f)), s.n3);
        const float r1 = pow(abs(raux1), (-1.f / s.n1));
        const float raux2 = pow(abs(1.f / s.a * cos(s.m * j / 4.f)), s.n2) + pow(abs(1.f / s.b * sin(s.m * j / 4.f)), s.n3);
        const float r2 = pow(abs(raux2), (-1.f / s.n1));
        computeBuffer[veccount][0] = r1 * cos(i) * r2 * cos(j);
        computeBuffer[veccount][1] = r1 * sin(i) * r2 * cos(j);
        computeBuffer[veccount][2] = r2 * sin(j);
        ++veccount;
      }
  }

  if (g.superFormulaVertexBuffer.getId())
    g.superFormulaVertexBuffer.destroy();
  bool succeed = g.superFormulaVertexBuffer.create(sizeof(GLfloat) * 3 * superFormulaNumVertices, computeBuffer, GL_DYNAMIC_DRAW).hasSucceed();
  jassert(succeed);
  succeed = g.superFormulaVertexArray.linkAttributeToBuffer(0, 3, GL_FLOAT, GL_FALSE, g.superFormulaVertexBuffer.getId()).hasSucceed();
  jassert(succeed);
}

static void SetupRC(Globals& g)
{
  // Black background
  g.context.getClearColor().setValue(docgl::GLColor(0.7f, 0.7f, 0.7f, 1.0f));
	g.cameraFrame.moveForward(-6.0f);
  bool succeed = g.superFormulaVertexArray.create().hasSucceed();
  jassert(succeed);
  constructSuperMesh(g, g.superFormula3d, g.invertCoordinate);

  // compile shader with attribute
  static const GLchar* transformVertexShader = 
    "uniform mat4 mvpMatrix;"
    "attribute vec4 vVertex;"
	  ""
    "void main(void) "
    "  {gl_Position = mvpMatrix * vVertex;}";
	
  static const GLchar* coloredFragmentShader = 
    "#version 410\n"
    ""
    "subroutine vec4 getColorSubRoutine();"
    ""
    "uniform vec4 vColor;"
    "subroutine uniform getColorSubRoutine getColorFunction;"    
    ""
    "subroutine(getColorSubRoutine)"
    "vec4 getUniformColor()"
    "  {return vColor;}"
    ""
    "subroutine(getColorSubRoutine)"
    "vec4 getUniformColorInv()"
    "  {return vec4(vec3(1 - vColor.rgb), vColor.a);}"
    ""
    "void main(void) "
    "  {gl_FragColor = getColorFunction();}";

  static const GLchar* vertexAttributes[] = {"vVertex", NULL};

  succeed = buildShaderProgram(g.flatColorTransformShader, transformVertexShader, coloredFragmentShader, vertexAttributes).hasSucceed();
  jassert(succeed);
  succeed = g.flatColorTransformShader.getUniformVariableLocation("vColor", g.vColorLocation).hasSucceed();
  jassert(succeed);
  succeed = g.flatColorTransformShader.getUniformVariableLocation("mvpMatrix", g.mvpMatrixLocation).hasSucceed();
  jassert(succeed);
  succeed = g.flatColorTransformShader.getSubroutineUniformLocation(GL_FRAGMENT_SHADER, "getColorFunction", g.getColorFunctionLocation).hasSucceed();
  jassert(succeed);
  succeed = g.flatColorTransformShader.getSubroutineIndex(GL_FRAGMENT_SHADER, "getUniformColor", g.getUniformColorIndex).hasSucceed();
  jassert(succeed);
  succeed = g.flatColorTransformShader.getSubroutineIndex(GL_FRAGMENT_SHADER, "getUniformColorInv", g.getUniformColorInvIndex).hasSucceed();
  jassert(succeed);
  succeed = g.context.getActiveProgramBind().setValue(g.flatColorTransformShader.getId()).hasSucceed();
  jassert(succeed);

  //GLint toto;
  //succeed = g.flatColorTransformShader.getActiveSubRoutines(GL_FRAGMENT_SHADER, toto).hasSucceed();
  //succeed = g.flatColorTransformShader.getActiveSubRoutineUniformLocations(GL_FRAGMENT_SHADER, toto).hasSucceed();
  //succeed = g.flatColorTransformShader.getActiveSubRoutineUniforms(GL_FRAGMENT_SHADER, toto).hasSucceed();
}

static void ShutdownRC(Globals& g)
{
  g.flatColorTransformShader.destroy();
  g.superFormulaVertexArray.destroy();
  g.superFormulaVertexBuffer.destroy();
  g.context.getClearColor().setValue(docgl::GLColor(0.0f));
}

static void RenderScene(Globals& g)
{
  g.context.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  g.context.getCullFace().setValue(g.cullFace);
  g.context.getDepthTest().setValue(g.depthTest);

  bool succeed;
  glm::mat4 modelViewProjection = g.projectionMatrix * g.cameraFrame.getCameraMatrix() * g.objectFrame.getMatrix();
  succeed = g.flatColorTransformShader.setUniformMatrix<4, 4>(g.mvpMatrixLocation, 1, GL_FALSE, &modelViewProjection[0][0]).hasSucceed();
  jassert(succeed);

  GLenum primitive;
  switch (g.nStep)
  {
  case 0: primitive = GL_POINTS; break;
  case 1: primitive = GL_LINES; break;
  case 2: primitive = GL_LINE_STRIP; break;
  case 3: primitive = GL_LINE_LOOP; break;
  case 4: primitive = GL_TRIANGLES; break;
  case 7: primitive = GL_QUADS; break;
  case 5: primitive = GL_TRIANGLE_FAN; break;
  case 6: primitive = GL_TRIANGLE_STRIP; break;
  case 8: primitive = GL_QUAD_STRIP; break;
  case 9: primitive = GL_POLYGON; break;
  }

  const GLfloat vGreen[] = {0.0f, 1.0f, 0.0f, 1.0f};
  const GLfloat vBlack[] = {0.0f, 0.0f, 0.0f, 1.0f};
  if (g.nStep < 4) 
  {
    succeed = g.flatColorTransformShader.setUniformValue(g.vColorLocation, 4, 1, vBlack).hasSucceed();
    jassert(succeed);

    // last thing to do with flatColorTransformShader
    succeed = g.context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, g.invertColor ? &g.getUniformColorInvIndex : &g.getUniformColorIndex).hasSucceed();
    jassert(succeed);

    succeed = g.superFormulaVertexArray.draw(primitive, 0, superFormulaNumVertices).hasSucceed();
    jassert(succeed);
  }
  else
  {
    // Draw the batch solid green
    succeed = g.flatColorTransformShader.setUniformValue(g.vColorLocation, 4, 1, vGreen).hasSucceed();
    jassert(succeed);

    // last thing to do with flatColorTransformShader
    succeed = g.context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, g.invertColor ? &g.getUniformColorInvIndex : &g.getUniformColorIndex).hasSucceed();
    jassert(succeed);

    succeed = g.superFormulaVertexArray.draw(primitive, 0, superFormulaNumVertices).hasSucceed();
    jassert(succeed);
    
    // Draw black outline
    g.context.getPolygonOffset().setValue(docgl::GLPolygonOffset(-1.0f, -1.0f));      // Shift depth values
    g.context.getLinePolygonOffset().setValue(true);

    // Draw black wireframe version of geometry
    succeed = g.flatColorTransformShader.setUniformValue(g.vColorLocation, 4, 1, vBlack).hasSucceed();
    jassert(succeed);

    // last thing to do with flatColorTransformShader
    succeed = g.context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, g.invertColor ? &g.getUniformColorInvIndex : &g.getUniformColorIndex).hasSucceed();
    jassert(succeed);

    succeed = g.superFormulaVertexArray.draw(GL_LINE_STRIP, 0, superFormulaNumVertices).hasSucceed();
    jassert(succeed);
    
    // Put everything back the way we found it
    g.context.getLinePolygonOffset().setValue(false);
    g.context.getPolygonOffset().setValue(docgl::GLPolygonOffset(0.0f, 0.0f));
  }

	// Flush drawing commands
	g.openGLWindow.swapBuffers();
}

static void KeyPressFunc(Globals& g, unsigned char key)
{
  bool invalidateMesh = false;
  if (key == VK_F1)
	{
		++g.nStep;
		if(g.nStep > 9)
			g.nStep = 0;
    invalidateMesh = true;
  }
  if (key == VK_F2)
    {g.invertCoordinate = !g.invertCoordinate; invalidateMesh = true;}
  if (key == VK_F3)
    g.cullFace = !g.cullFace;
  if (key == VK_F4)
    g.depthTest = !g.depthTest;
  if (key == VK_F5)
    g.invertColor = !g.invertColor;
  if(key == VK_UP)
	  g.objectFrame.rotateWorld(-5.0f, 1.0f, 0.0f, 0.0f);
  if(key == VK_DOWN)
	  g.objectFrame.rotateWorld(5.0f, 1.0f, 0.0f, 0.0f);
  if(key == VK_LEFT)
	  g.objectFrame.rotateWorld(-5.0f, 0.0f, 1.0f, 0.0f);
  if(key == VK_RIGHT)
	  g.objectFrame.rotateWorld(5.0f, 0.0f, 1.0f, 0.0f);
  if (key == 'Q')
    {g.superFormula3d.a = max(0.f, g.superFormula3d.a - 0.01f); invalidateMesh = true;}
  else if (key == 'A')
    {g.superFormula3d.a = min(3.f, g.superFormula3d.a + 0.01f); invalidateMesh = true;}
  else if (key == 'S')
    {g.superFormula3d.b = max(0.f, g.superFormula3d.b - 0.01f); invalidateMesh = true;}
  else if (key == 'Z')
    {g.superFormula3d.b = min(3.f, g.superFormula3d.b + 0.01f); invalidateMesh = true;}
  else if (key == 'D')
    {g.superFormula3d.m = max(0.f, g.superFormula3d.m - 0.1f); invalidateMesh = true;}
  else if (key == 'E')
    {g.superFormula3d.m = min(20.f, g.superFormula3d.m + 0.1f); invalidateMesh = true;}
  else if (key == 'F')
    {g.superFormula3d.n1 = max(0.f, g.superFormula3d.n1 - 0.1f); invalidateMesh = true;}
  else if (key == 'R')
    {g.superFormula3d.n1 = min(20.f, g.superFormula3d.n1 + 0.1f); invalidateMesh = true;}
  else if (key == 'G')
    {g.superFormula3d.n2 = max(0.f, g.superFormula3d.n2 - 0.1f); invalidateMesh = true;}
  else if (key == 'T')
    {g.superFormula3d.n2 = min(20.f, g.superFormula3d.n2 + 0.1f); invalidateMesh = true;}          
  else if (key == 'H')
    {g.superFormula3d.n3 = max(0.f, g.superFormula3d.n3 - 0.1f); invalidateMesh = true;}
  else if (key == 'Y')
    {g.superFormula3d.n3 = min(20.f, g.superFormula3d.n3 + 0.1f); invalidateMesh = true;}

  if (invalidateMesh)
    constructSuperMesh(g, g.superFormula3d, g.invertCoordinate);
}

///////////////////////////////////////////////////////////////////////////////
// Window has changed size, or has just been created. In either case, we need
// to use the window dimensions to set the viewport and the projection matrix.
static void ChangeSize(Globals& g, int w, int h)
{
  if (!g.context.isInitialized()) 
    return; // ChangeSize can be called before g.context initializing (durring window creation)

  g.context.getActiveViewport().setValue(docgl::GLRegion(w, h));
  g.projectionMatrix = glm::perspective(35.0f, float(w) / float(h), 1.0f, 100.f);
  RenderScene(g);  // redraw on window resize;
}

///////////////////////////////////////////////////////////////////////////////
// Callback functions to handle all window functions this app cares about.
// Once complete, pass message on to next app in the hook chain.
static LRESULT CALLBACK WndProc(HWND	hWnd,		    // Handle For This Window
                         UINT	uMsg,		    // Message For This Window
                         WPARAM	wParam,		// Additional Message Information
                         LPARAM	lParam)		// Additional Message Information
{
  unsigned char key = 0;

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
    {
      key = (char)wParam;
      if (key == 27)
        PostQuitMessage(0);
      else
      {
        Globals* g = (Globals*)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if (g)
          KeyPressFunc(*g, key);
      }
    }
    return 0;
  default:
    break;
  }
  // Pass All Unhandled Messages To DefWindowProc
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int superFormula(LPCTSTR szClassName, CONST RECT& clientRect)
{
  Globals g;

  HINSTANCE module = GetModuleHandle(NULL);
  if(!docwgl::registerOpenGLWindowClass(szClassName, module, (WNDPROC)WndProc, LoadCursor(NULL, IDC_ARROW)))
  {
    printf("Unable to register window class.\n");
    return 1;
  }

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
    0                          // NULL termination
  }; 
  const int contextAttributes[] = {
    WGL_CONTEXT_MAJOR_VERSION_ARB,  4,
    WGL_CONTEXT_MINOR_VERSION_ARB,  1,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
    0};

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
  printf("max subroutines per shader stage: %u\n", g.context.getMaxSubRoutines());
  printf("max subroutines uniform locations per stage: %u\n", g.context.getMaxSubRoutinesUniformLocation());

  printf("HELP:\n");
  printf("[A]/[Q] +-a\n");
  printf("[Z]/[S] +-b\n");
  printf("[E]/[D] +-m\n");
  printf("[R]/[F] +-n1\n");
  printf("[T]/[G] +-n2\n");
  printf("[Y]/[H] +-n3\n");
  printf("[F1] cicle primitives\n");
  printf("[F2] cicle orientations\n");
  printf("[F3] cull face on/off\n");
  printf("[F4] depth test on/off\n");
  printf("[F5] invert color on/off\n");
  printf("[ESC] exit\n");

  SetupRC(g);
  ChangeSize(g, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top);
  
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
