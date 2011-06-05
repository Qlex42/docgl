/* -------------------------------- . ---------------------------------------- .
| Filename : SuperFormula.cpp       | DOCGL3.3 sample file                     |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */

# define  _USE_MATH_DEFINES
# include "Tools.h"
# include <Docgl/Docgl.h>
# include <Docgl/DocglWindow.h>

struct SuperFormula3D
  {float a, b, m, n1, n2, n3;};
static const float step = 0.05f;
enum {superFormulaNumVertices = 7938}; // depends from step

#ifdef linux
#define max(X, Y) ((X) > (Y) ? (X) : (Y))
#define min(X, Y) ((X) < (Y) ? (X) : (Y))
#endif // linux

class Globals: public OpenGLWindowCallback
{
public:
  docgl::GLDirectContext context;
  docgl::GLBufferObject superFormulaVertexBuffer;
  docgl::GLVertexArrayObject superFormulaVertexArray;
  docgl::GLProgramObject flatColorTransformShader;
  GLint vColorLocation;
  GLint mvpMatrixLocation;
#ifdef DOCGL4_1
  GLint getColorFunctionLocation;
  GLuint getUniformColorIndex;
  GLuint getUniformColorInvIndex;
  bool invertColor;
#endif // !DOCGL4_1
  glm::mat4         projectionMatrix;
  Frame			  	    cameraFrame;
  Frame             objectFrame;
  int nStep;
  SuperFormula3D superFormula3d;
  bool invertCoordinate;
  bool cullFace;
  bool depthTest;
  bool wantExit;

  Globals()
    : superFormulaVertexBuffer(context)
    , superFormulaVertexArray(context)
    , flatColorTransformShader(context)
    , vColorLocation(-1)
    , mvpMatrixLocation(-1)
#ifdef DOCGL4_1
    , getColorFunctionLocation(-1)
    , getUniformColorIndex(GL_INVALID_INDEX)
    , getUniformColorInvIndex(GL_INVALID_INDEX)
    , invertColor(false)
#endif // !DOCGL4_1
    , nStep(0)
    , invertCoordinate(false)
    , cullFace(false)
    , depthTest(true)
    , wantExit(false)
    {
      static const SuperFormula3D formula = {1.f, 1.f, 4.f, 12.f, 15.f, 15.f};
      superFormula3d = formula;
    }

  virtual void draw()
  {
    context.clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    context.getCullFace().setValue(cullFace);
    context.getDepthTest().setValue(depthTest);

    bool succeed;
    glm::mat4 modelViewProjection = projectionMatrix * cameraFrame.getCameraMatrix() * objectFrame.getMatrix();
    succeed = flatColorTransformShader.setUniformMatrix<4, 4>(mvpMatrixLocation, 1, GL_FALSE, &modelViewProjection[0][0]).hasSucceed();
    glUniformMatrix4fv(mvpMatrixLocation, 1, GL_FALSE, &modelViewProjection[0][0]);

    GLenum primitive;
    switch (nStep)
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
    if (nStep < 4)
    {
      succeed = flatColorTransformShader.setUniformValue(vColorLocation, 4, 1, vBlack).hasSucceed();
      jassert(succeed);

  #ifdef DOCGL4_1
      // last thing to do with flatColorTransformShader
      succeed = context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, invertColor ? &getUniformColorInvIndex : &getUniformColorIndex).hasSucceed();
      jassert(succeed);
  #endif // !DOCGL4_1

      succeed = superFormulaVertexArray.draw(primitive, 0, superFormulaNumVertices).hasSucceed();
      jassert(succeed);
    }
    else
    {
      // Draw the batch solid green
      succeed = flatColorTransformShader.setUniformValue(vColorLocation, 4, 1, vGreen).hasSucceed();
      jassert(succeed);

  #ifdef DOCGL4_1
      // last thing to do with flatColorTransformShader
      succeed = context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, invertColor ? &getUniformColorInvIndex : &getUniformColorIndex).hasSucceed();
      jassert(succeed);
  #endif // !DOCGL4_1

      succeed = superFormulaVertexArray.draw(primitive, 0, superFormulaNumVertices).hasSucceed();
      jassert(succeed);

      // Draw black outline
      context.getPolygonOffset().setValue(docgl::GLPolygonOffset(-1.0f, -1.0f));      // Shift depth values
      context.getLinePolygonOffset().setValue(true);

      // Draw black wireframe version of geometry
      succeed = flatColorTransformShader.setUniformValue(vColorLocation, 4, 1, vBlack).hasSucceed();
      jassert(succeed);

  #ifdef DOCGL4_1
      // last thing to do with flatColorTransformShader
      succeed = context.loadUniformSubroutines(GL_FRAGMENT_SHADER, 1, invertColor ? &getUniformColorInvIndex : &getUniformColorIndex).hasSucceed();
      jassert(succeed);
  #endif // !DOCGL4_1

      succeed = superFormulaVertexArray.draw(GL_LINE_STRIP, 0, superFormulaNumVertices).hasSucceed();
      jassert(succeed);

      // Put everything back the way we found it
      context.getLinePolygonOffset().setValue(false);
      context.getPolygonOffset().setValue(docgl::GLPolygonOffset(0.0f, 0.0f));
    }
  }

  void constructSuperMesh(const SuperFormula3D& s, bool invertCoordinate)
  {
    GLfloat computeBuffer[superFormulaNumVertices][3];
    int veccount = 0;
    if (invertCoordinate)
    {
      for (float j = float(-M_PI / 2.f); j < M_PI / 2.f; j += step)
        for (float i = float(-M_PI); i < M_PI; i += step)
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
      for (float i = float(-M_PI); i < M_PI; i += step)
        for (float j = float(-M_PI / 2.f); j < M_PI / 2.f; j += step)
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

    if (superFormulaVertexBuffer.getId())
      superFormulaVertexBuffer.destroy();
    bool succeed = superFormulaVertexBuffer.create(sizeof(GLfloat) * 3 * superFormulaNumVertices, computeBuffer, GL_DYNAMIC_DRAW).hasSucceed();
    jassert(succeed);
    succeed = superFormulaVertexArray.linkAttributeToBuffer(0, 3, GL_FLOAT, GL_FALSE, superFormulaVertexBuffer.getId()).hasSucceed();
    jassert(succeed);
  }

  virtual void keyPressed(unsigned char key)
  {
    bool invalidateMesh = false;
    if (key == 0x1B) // [Escape]
      wantExit = true;
    if (key == 43) // [+]
	  {
		  ++nStep;
		  if(nStep > 9)
			  nStep = 0;
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
      {superFormula3d.a = max(0.f, superFormula3d.a - 0.01f); invalidateMesh = true;}
    else if (key == 'A' || key == 'a')
      {superFormula3d.a = min(3.f, superFormula3d.a + 0.01f); invalidateMesh = true;}
    else if (key == 'S' || key == 's')
      {superFormula3d.b = max(0.f, superFormula3d.b - 0.01f); invalidateMesh = true;}
    else if (key == 'Z' || key == 'z')
      {superFormula3d.b = min(3.f, superFormula3d.b + 0.01f); invalidateMesh = true;}
    else if (key == 'D' || key == 'd')
      {superFormula3d.m = max(0.f, superFormula3d.m - 0.1f); invalidateMesh = true;}
    else if (key == 'E' || key == 'e')
      {superFormula3d.m = min(20.f, superFormula3d.m + 0.1f); invalidateMesh = true;}
    else if (key == 'F' || key == 'f')
      {superFormula3d.n1 = max(0.f, superFormula3d.n1 - 0.1f); invalidateMesh = true;}
    else if (key == 'R' || key == 'r')
      {superFormula3d.n1 = min(20.f, superFormula3d.n1 + 0.1f); invalidateMesh = true;}
    else if (key == 'G' || key == 'g')
      {superFormula3d.n2 = max(0.f, superFormula3d.n2 - 0.1f); invalidateMesh = true;}
    else if (key == 'T' || key == 't')
      {superFormula3d.n2 = min(20.f, superFormula3d.n2 + 0.1f); invalidateMesh = true;}
    else if (key == 'H' || key == 'h')
      {superFormula3d.n3 = max(0.f, superFormula3d.n3 - 0.1f); invalidateMesh = true;}
    else if (key == 'Y' || key == 'y')
      {superFormula3d.n3 = min(20.f, superFormula3d.n3 + 0.1f); invalidateMesh = true;}
    if (invalidateMesh)
      constructSuperMesh(superFormula3d, invertCoordinate);
  }

  virtual void resized(int w, int h)
  {
    context.getActiveViewport().setValue(docgl::GLRegion(w, h));
    context.getActiveScissor().setValue(docgl::GLRegion(w, h));
    projectionMatrix = glm::perspective(35.0f, float(w) / float(h), 1.0f, 100.f);
  }

  virtual void closed()
    {wantExit = true;}

  void SetupRC()
  {
    // Black background
    context.getClearColor().setValue(docgl::GLColor(0.7f, 0.7f, 0.7f, 1.0f));
	  cameraFrame.moveForward(-6.0f);
    bool succeed = superFormulaVertexArray.create().hasSucceed();
    jassert(succeed);
    constructSuperMesh(superFormula3d, invertCoordinate);

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
    superFormulaVertexArray.destroy();
    superFormulaVertexBuffer.destroy();
    context.getClearColor().setValue(docgl::GLColor(0.0f));
  }
};

int superFormula(const char* szClassName, int x, int y, int width, int height)
{
  Globals g;
  OpenGLWindow window(g.context);

  if (!window.create(g, x, y, width, height, szClassName))
  {
    printf("Error: Unable to create OpenGL Window.\n");
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

#ifdef DOCGL4_1
  printf("max subroutines per shader stage: %u\n", g.context.getMaxSubRoutines());
  printf("max subroutines uniform locations per stage: %u\n", g.context.getMaxSubRoutinesUniformLocation());
#endif // !DOCGL4_1

  printf("HELP:\n");
  printf("[A]/[Q] +-a\n");
  printf("[Z]/[S] +-b\n");
  printf("[E]/[D] +-m\n");
  printf("[R]/[F] +-n1\n");
  printf("[T]/[G] +-n2\n");
  printf("[Y]/[H] +-n3\n");
  printf("[+] cicle primitives\n");
  printf("[-] cicle orientations\n");
  printf("[/] cull face on/off\n");
  printf("[.] depth test on/off\n");
#ifdef DOCGL4_1
  printf("[0] invert color on/off\n");
#endif // !DOCGL4_1
  printf("[ESC] exit\n");

  g.SetupRC();
  g.resized(width, height);

  OpenGLWindow* windowsLists = &window;
  EventDispatcher dispatcher(&windowsLists, 1);
  do
  {
    if (!dispatcher.dispatchNextEvent())
    {
      if (window.isVisible() && !g.wantExit)
      {
        g.draw(); // no message to dispatch: render the scene.
        window.swapBuffers(); // Flush drawing commands
      }
    }
  }
  while (!g.wantExit);
  printf("exiting...\n");
  g.ShutdownRC();
  window.destroy();
  return 0;
}
