/* -------------------------------- . ---------------------------------------- .
| Filename : SuperFormula.cpp       | DOCGL3.3 sample file                     |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */

# define  _USE_MATH_DEFINES
# include "Tools.h"
# include <Docgl/Docgl.h>
# include <Docgl/DocglWindow.h>

#ifdef linux
#define max(X, Y) ((X) > (Y) ? (X) : (Y))
#define min(X, Y) ((X) < (Y) ? (X) : (Y))
#endif // linux

class SuperFormula: public OpenGLWindowCallback
{
public:
  struct SuperFormulaParameters
    {float a, b, m, n1, n2, n3;};

  docgl::GLContext& context;
  docgl::GLBufferObject vertexBuffer;
  docgl::GLVertexArrayObject vertexArray;
  docgl::GLProgramObject flatColorTransformShader;
  GLint vColorLocation;
  GLint mvpMatrixLocation;
#ifdef DOCGL4_1
  GLint getColorFunctionLocation;
  GLuint getUniformColorIndex;
  GLuint getUniformColorInvIndex;
  bool invertColor;
#endif // !DOCGL4_1
  glm::mat4 projectionMatrix;
  Frame	cameraFrame;
  Frame objectFrame;
  int primitiveId;
  SuperFormulaParameters superFormulaParameters;
  bool invertCoordinate;
  bool cullFace;
  bool depthTest;
  bool running;

  static const float meshPrecisionStep = 0.05f;
  enum {superFormulaNumVertices = 7938}; // hardcoded depends from meshPrecisionStep

  SuperFormula(docgl::GLContext& context)
    : context(context)
    , vertexBuffer(context)
    , vertexArray(context)
    , flatColorTransformShader(context)
    , vColorLocation(-1)
    , mvpMatrixLocation(-1)
#ifdef DOCGL4_1
    , getColorFunctionLocation(-1)
    , getUniformColorIndex(GL_INVALID_INDEX)
    , getUniformColorInvIndex(GL_INVALID_INDEX)
    , invertColor(false)
#endif // !DOCGL4_1
    , primitiveId(0)
    , invertCoordinate(false)
    , cullFace(false)
    , depthTest(true)
    , running(true)
  {
    static const SuperFormulaParameters defaultSuperFormulaParameters = {1.f, 1.f, 4.f, 12.f, 15.f, 15.f};
    superFormulaParameters = defaultSuperFormulaParameters;
  }

  virtual void draw()
  {
    context.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    context.getCullFace().setValue(cullFace);
    context.getDepthTest().setValue(depthTest);

    bool succeed;
    glm::mat4 modelViewProjection = projectionMatrix * cameraFrame.getCameraMatrix() * objectFrame.getMatrix();
    succeed = flatColorTransformShader.setUniformMatrix<4, 4>(mvpMatrixLocation, 1, GL_FALSE, &modelViewProjection[0][0]).hasSucceed();

    GLenum primitive;
    switch (primitiveId)
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

    static const GLfloat greenColor[] = {0.0f, 1.0f, 0.0f, 1.0f};
    static const GLfloat blackColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
    if (primitiveId < 4)
    {
      succeed = flatColorTransformShader.setUniformValue(vColorLocation, 4, 1, blackColor).hasSucceed();
      jassert(succeed);

  #ifdef DOCGL4_1
      // last thing to do with flatColorTransformShader
      succeed = context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, invertColor ? &getUniformColorInvIndex : &getUniformColorIndex).hasSucceed();
      jassert(succeed);
  #endif // !DOCGL4_1

      succeed = vertexArray.draw(primitive, 0, superFormulaNumVertices).hasSucceed();
      jassert(succeed);
    }
    else
    {
      // Draw the batch solid green
      succeed = flatColorTransformShader.setUniformValue(vColorLocation, 4, 1, greenColor).hasSucceed();
      jassert(succeed);

  #ifdef DOCGL4_1
      // last thing to do with flatColorTransformShader
      succeed = context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, invertColor ? &getUniformColorInvIndex : &getUniformColorIndex).hasSucceed();
      jassert(succeed);
  #endif // !DOCGL4_1

      succeed = vertexArray.draw(primitive, 0, superFormulaNumVertices).hasSucceed();
      jassert(succeed);

      // Draw black outline
      context.getPolygonOffset().setValue(docgl::GLPolygonOffset(-1.0f, -1.0f));      // Shift depth values
      context.getLinePolygonOffset().setValue(true);

      // Draw black wireframe version of geometry
      succeed = flatColorTransformShader.setUniformValue(vColorLocation, 4, 1, blackColor).hasSucceed();
      jassert(succeed);

  #ifdef DOCGL4_1
      // last thing to do with flatColorTransformShader
      succeed = context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, invertColor ? &getUniformColorInvIndex : &getUniformColorIndex).hasSucceed();
      jassert(succeed);
  #endif // !DOCGL4_1

      succeed = vertexArray.draw(GL_LINE_STRIP, 0, superFormulaNumVertices).hasSucceed();
      jassert(succeed);

      // Put everything back the way we found it
      context.getLinePolygonOffset().setValue(false);
      context.getPolygonOffset().setValue(docgl::GLPolygonOffset(0.0f, 0.0f));
    }
  }

  void constructSuperMesh(const SuperFormulaParameters& s, bool invertCoordinate)
  {
    GLfloat computeBuffer[superFormulaNumVertices][3];
    int veccount = 0;
    if (invertCoordinate)
    {
      for (float j = float(-M_PI / 2.f); j < M_PI / 2.f; j += meshPrecisionStep)
        for (float i = float(-M_PI); i < M_PI; i += meshPrecisionStep)
        {
           float raux1 = pow(std::abs(1.f / s.a * cos(s.m * i / 4.f)), s.n2) + pow(std::abs(1.f / s.b * sin(s.m * i / 4.f)), s.n3);
           float r1 = pow(std::abs(raux1), (-1.f / s.n1));
           float raux2 = pow(std::abs(1.f / s.a * cos(s.m * j / 4.f)), s.n2) + pow(std::abs(1.f / s.b * sin(s.m * j / 4.f)), s.n3);
           float r2 = pow(std::abs(raux2), (-1.f / s.n1));
          computeBuffer[veccount][0] = r1 * cos(i) * r2 * cos(j);
          computeBuffer[veccount][1] = r1 * sin(i) * r2 * cos(j);
          computeBuffer[veccount][2] = r2 * sin(j);
          ++veccount;
        }
    }
    else
    {
      for (float i = float(-M_PI); i < M_PI; i += meshPrecisionStep)
        for (float j = float(-M_PI / 2.f); j < M_PI / 2.f; j += meshPrecisionStep)
        {
          const float raux1 = pow(std::abs(1.f / s.a * cos(s.m * i / 4.f)), s.n2) + pow(std::abs(1.f / s.b * sin(s.m * i / 4.f)), s.n3);
          const float r1 = pow(std::abs(raux1), (-1.f / s.n1));
          const float raux2 = pow(std::abs(1.f / s.a * cos(s.m * j / 4.f)), s.n2) + pow(std::abs(1.f / s.b * sin(s.m * j / 4.f)), s.n3);
          const float r2 = pow(std::abs(raux2), (-1.f / s.n1));
          computeBuffer[veccount][0] = r1 * cos(i) * r2 * cos(j);
          computeBuffer[veccount][1] = r1 * sin(i) * r2 * cos(j);
          computeBuffer[veccount][2] = r2 * sin(j);
          ++veccount;
        }
    }

    if (vertexBuffer.getId())
      vertexBuffer.destroy();
    bool succeed = vertexBuffer.create(sizeof(GLfloat) * 3 * superFormulaNumVertices, computeBuffer, GL_DYNAMIC_DRAW).hasSucceed();
    jassert(succeed);
    succeed = vertexArray.linkAttributeToBuffer(0, 3, GL_FLOAT, GL_FALSE, vertexBuffer.getId()).hasSucceed();
    jassert(succeed);
  }

  virtual void keyPressed(unsigned char key)
  {
    bool invalidateMesh = false;
    if (key == 0x1B) // [Escape]
      running = false;
    if (key == 43) // [+]
	  {
		  ++primitiveId;
		  if(primitiveId > 9)
			  primitiveId = 0;
      invalidateMesh = true;
    }
    if (key == 45) // [-]
      {invertCoordinate = !invertCoordinate; invalidateMesh = true;}
    if (key == 47) // [/]
      cullFace = !cullFace;
    if (key == 46) // [.]
      depthTest = !depthTest;
  #ifdef DOCGL4_1
    if (key == 48) // [0]
      invertColor = !invertColor;
  #endif // !DOCG4_1
    if(key == 56) // [8]
	    objectFrame.rotateWorld(-5.0f, 1.0f, 0.0f, 0.0f);
    if(key == 50) // [2]
	    objectFrame.rotateWorld(5.0f, 1.0f, 0.0f, 0.0f);
    if(key == 52) // [4]
	    objectFrame.rotateWorld(-5.0f, 0.0f, 1.0f, 0.0f);
    if(key == 54) // [6]
	    objectFrame.rotateWorld(5.0f, 0.0f, 1.0f, 0.0f);
    if (key == 'Q' || key == 'q')
      {superFormulaParameters.a = max(0.f, superFormulaParameters.a - 0.01f); invalidateMesh = true;}
    else if (key == 'A' || key == 'a')
      {superFormulaParameters.a = min(3.f, superFormulaParameters.a + 0.01f); invalidateMesh = true;}
    else if (key == 'S' || key == 's')
      {superFormulaParameters.b = max(0.f, superFormulaParameters.b - 0.01f); invalidateMesh = true;}
    else if (key == 'Z' || key == 'z')
      {superFormulaParameters.b = min(3.f, superFormulaParameters.b + 0.01f); invalidateMesh = true;}
    else if (key == 'D' || key == 'd')
      {superFormulaParameters.m = max(0.f, superFormulaParameters.m - 0.1f); invalidateMesh = true;}
    else if (key == 'E' || key == 'e')
      {superFormulaParameters.m = min(20.f, superFormulaParameters.m + 0.1f); invalidateMesh = true;}
    else if (key == 'F' || key == 'f')
      {superFormulaParameters.n1 = max(0.f, superFormulaParameters.n1 - 0.1f); invalidateMesh = true;}
    else if (key == 'R' || key == 'r')
      {superFormulaParameters.n1 = min(20.f, superFormulaParameters.n1 + 0.1f); invalidateMesh = true;}
    else if (key == 'G' || key == 'g')
      {superFormulaParameters.n2 = max(0.f, superFormulaParameters.n2 - 0.1f); invalidateMesh = true;}
    else if (key == 'T' || key == 't')
      {superFormulaParameters.n2 = min(20.f, superFormulaParameters.n2 + 0.1f); invalidateMesh = true;}
    else if (key == 'H' || key == 'h')
      {superFormulaParameters.n3 = max(0.f, superFormulaParameters.n3 - 0.1f); invalidateMesh = true;}
    else if (key == 'Y' || key == 'y')
      {superFormulaParameters.n3 = min(20.f, superFormulaParameters.n3 + 0.1f); invalidateMesh = true;}
    if (invalidateMesh)
      constructSuperMesh(superFormulaParameters, invertCoordinate);
  }

  virtual void resized(int w, int h)
  {
    context.getActiveViewport().setValue(docgl::GLRegion(w, h));
    //context.getActiveScissor().setValue(docgl::GLRegion(w, h));
    projectionMatrix = glm::perspective(35.0f, float(w) / float(h), 1.0f, 100.f);
  }

  virtual void closed()
    {running = false;}

  void SetupRC()
  {
    context.getClearColor().setValue(docgl::GLColor(0.7f, 0.7f, 0.7f, 1.0f));
	  cameraFrame.moveForward(-6.0f);
    bool succeed = vertexArray.create().hasSucceed();
    jassert(succeed);
    constructSuperMesh(superFormulaParameters, invertCoordinate);

    // compile shader with attribute
    static const GLchar* transformVertexShader =
      "uniform mat4 mvpMatrix;"
      "attribute vec4 vVertex;"
	    ""
      "void main(void) "
      "  {gl_Position = mvpMatrix * vVertex;}";

  #ifdef DOCGL4_1
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
  #else // !DOCGL4_1
    static const GLchar* coloredFragmentShader =
      "uniform vec4 vColor;"
      ""
      "void main(void) "
      "  {gl_FragColor = vColor;}";
  #endif // !DOCGL4_1

    static const GLchar* vertexAttributes[] = {"vVertex", NULL};

    succeed = buildShaderProgram(flatColorTransformShader, transformVertexShader, coloredFragmentShader, vertexAttributes).hasSucceed();
    jassert(succeed);
    succeed = flatColorTransformShader.getUniformVariableLocation("vColor", vColorLocation).hasSucceed();
    jassert(succeed);
    succeed = flatColorTransformShader.getUniformVariableLocation("mvpMatrix", mvpMatrixLocation).hasSucceed();
    jassert(succeed);
  #ifdef DOCGL4_1
    succeed = flatColorTransformShader.getSubroutineUniformLocation(GL_FRAGMENT_SHADER, "getColorFunction", getColorFunctionLocation).hasSucceed();
    jassert(succeed);
    succeed = flatColorTransformShader.getSubroutineIndex(GL_FRAGMENT_SHADER, "getUniformColor", getUniformColorIndex).hasSucceed();
    jassert(succeed);
    succeed = flatColorTransformShader.getSubroutineIndex(GL_FRAGMENT_SHADER, "getUniformColorInv", getUniformColorInvIndex).hasSucceed();
    jassert(succeed);
  #endif // !DOCGL4_1
    succeed = context.getActiveProgramBind().setValue(flatColorTransformShader.getId()).hasSucceed();
    jassert(succeed);
  }

  void ShutdownRC()
  {
    flatColorTransformShader.destroy();
    vertexArray.destroy();
    vertexBuffer.destroy();
    context.getClearColor().setValue(docgl::GLColor(0.0f));
  }
};

int superFormula(const char* szClassName, int x, int y, int width, int height)
{
  docgl::GLDirectContext context;
  SuperFormula superFormula(context);
  OpenGLWindow window(context);

  if (!window.create(superFormula, x, y, width, height, szClassName))
  {
    printf("Error: Unable to create OpenGL Window.\n");
    jassertfalse;
    return 1;
  }

  if (context.initialize().hasErrors())
  {
    window.destroy();
    jassertfalse;
    fprintf(stderr, "GLContext initialize error\n");
    return 2;
  }

#ifdef DOCGL4_1
  printf("max subroutines per shader stage: %u\n", superFormula.context.getMaxSubRoutines());
  printf("max subroutines uniform locations per stage: %u\n", superFormula.context.getMaxSubRoutinesUniformLocation());
#endif // !DOCGL4_1
  printf("SuperFormula HELP:\n");
  printf("[A]/[Q] +- a\n");
  printf("[Z]/[S] +- b\n");
  printf("[E]/[D] +- m\n");
  printf("[R]/[F] +- n1\n");
  printf("[T]/[G] +- n2\n");
  printf("[Y]/[H] +- n3\n");
  printf("Moving Help:\n");
  printf("[8]/[2] Rotate around X axis\n");
  printf("[4]/[6] Rotate around Y axis\n");
  printf("Rendering Help:\n");
  printf("[+] cicle primitives\n");
  printf("[-] switch orientations\n");
  printf("[/] cull face on/off\n");
  printf("[.] depth test on/off\n");
#ifdef DOCGL4_1
  printf("[0] invert color on/off\n");
#endif // !DOCGL4_1
  printf("[ESC] exit\n");

  superFormula.SetupRC();
  superFormula.resized(width, height);

  OpenGLWindow* windowsLists = &window;
  EventDispatcher dispatcher(&windowsLists, 1);
  while (superFormula.running)
  {
    if (!dispatcher.dispatchNextEvent())
    {
      if (window.isVisible() && superFormula.running)
      {
        superFormula.draw(); // no message to dispatch: render the scene.
        window.swapBuffers(); // Flush drawing commands
      }
    }
  }

  superFormula.ShutdownRC();
  window.destroy();

  printf("exit\n");
  return 0;
}
