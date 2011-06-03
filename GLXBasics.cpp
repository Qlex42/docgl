// GLXBasics.c
// Use GLX to setup OpenGL windows
// Draw eyeballs
// OpenGL SuperBible, 5th Edition
// Nick Haemel
// Modified by Alexandre Buge (use Docglx, c++ port...).

#include "Docglx.h"
#include <cmath>

#define PI 3.14159265


typedef struct // RenderContext
{
  void setupGLState()
  {
    nMousePosX = 0;
    nMousePosY = 0;
    glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Clear matrix stack
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
  }

  void resized(int width, int height)
  {
    nWinWidth = width;
    nWinHeight = height;

    float aspectRatio = nWinHeight ? (float)nWinWidth/(float)nWinHeight : 1.0f;
    float fYTop     = 0.05f;
    float fYBottom  = - fYTop;
    float fXLeft    = fYTop     * aspectRatio;
    float fXRight   = fYBottom  * aspectRatio;

    glViewport(0,0,nWinWidth,nWinHeight);
    glScissor(0,0,nWinWidth,nWinHeight);
    // Set the frustrum
    glFrustum(fXLeft, fXRight, fYBottom, fYTop, 0.1f, 100.f);
    update();
  }

  void mouseMove(int x, int y)
  {
    nMousePosX = x;
    nMousePosY = y;
    update();
  }

  void update()
  {
    int nLtEyePosX = (nWinWidth)/2 - (0.3 * ((nWinWidth)/2));
    int nLtEyePosY = (nWinHeight)/2;
    int nRtEyePosX = (nWinWidth)/2 + (0.3 * ((nWinWidth)/2));
    int nRtEyePosY = (nWinHeight)/2;

    double fLtVecMag = 100;
    double fRtVecMag = 100;

    // Use the eyeball width as the minimum
    double fMinLength =  (0.1 * ((nWinWidth)/2));

    // Calculate the length of the vector from the eyeball
    // to the mouse pointer
    fLtVecMag = sqrt( pow((double)(nMousePosX - nLtEyePosX), 2.0) +
                      pow((double)(nMousePosY - nLtEyePosY), 2.0));

    fRtVecMag =  sqrt( pow((double)(nMousePosX - nRtEyePosX), 2.0) +
                       pow((double)(nMousePosY - nRtEyePosY), 2.0));

    // Clamp the minimum magnatude
    if (fRtVecMag < fMinLength)
      fRtVecMag = fMinLength;
    if (fLtVecMag < fMinLength)
      fLtVecMag = fMinLength;

    // Normalize the vector components
    fLeftX = (float)(nMousePosX - nLtEyePosX) / fLtVecMag;
    fLeftY = -1.0 * (float)(nMousePosY - nLtEyePosY) / fLtVecMag;
    fRightX = (float)(nMousePosX - nRtEyePosX) / fRtVecMag;
    fRightY = -1.0 * ((float)(nMousePosY - nRtEyePosY) / fRtVecMag);
  }

  void drawCircle()
  {
    float fx = 0.0;
    float fy = 0.0;

    int nDegrees = 0;

    // Use a triangle fan with 36 tris for the circle
    glBegin(GL_TRIANGLE_FAN);
      glVertex2f(0.0, 0.0);
      for(nDegrees = 0; nDegrees < 360; nDegrees+=10)
      {
        fx = sin((float)nDegrees*PI/180);
        fy = cos((float)nDegrees*PI/180);
        glVertex2f(fx, fy);
      }
      glVertex2f(0.0f, 1.0f);
    glEnd();
  }

  void draw()
  {
    glClear(GL_COLOR_BUFFER_BIT);

     // Clear matrix stack
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw left eyeball
    glColor3f(1.0, 1.0, 1.0);
    glScalef(0.20, 0.20, 1.0);
    glTranslatef(-1.5, 0.0, 0.0);
    drawCircle();

    // Draw black
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.40, 0.40, 1.0);
    glTranslatef(fLeftX, fLeftY, 0.0);
    drawCircle();

    // Draw right eyeball
    glColor3f(1.0, 1.0, 1.0);
    glLoadIdentity();
    glScalef(0.20, 0.20, 1.0);
    glTranslatef(1.5, 0.0, 0.0);
    drawCircle();

    // Draw black
    glColor3f(0.0, 0.0, 0.0);
    glScalef(0.40, 0.40, 1.0);
    glTranslatef(fRightX, fRightY, 0.0);
    drawCircle();

    // Clear matrix stack
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Draw Nose
    glColor3f(0.5, 0.0, 0.0);
    glScalef(0.20, 0.20, 1.0);
    glTranslatef(0.0, -1.5, 0.0);

    glBegin(GL_TRIANGLES);
      glVertex2f(0.0, 1.0);
      glVertex2f(-0.5, -1.0);
      glVertex2f(0.5, -1.0);
    glEnd();
  }

  int nWinWidth;
  int nWinHeight;
  int nMousePosX;
  int nMousePosY;
  float fRightX;
  float fRightY;
  float fLeftX;
  float fLeftY;
} RenderContext;


int main()
{
  Bool bWinMapped = False;
  Bool running = True;
  RenderContext rcx;
  OpenGLXWindow window;
  static const int config_attrib_list[] = {
                  GLX_RENDER_TYPE,   GLX_RGBA_BIT,
                  GLX_X_RENDERABLE,  True,
                  GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
                  GLX_DOUBLEBUFFER,  True,
                  GLX_RED_SIZE,      8,
                  GLX_BLUE_SIZE,     8,
                  GLX_GREEN_SIZE,    8,
                  None};
  // ALEX: understand why 3.3 do not work with GTX295/Ubuntu32 11.04 (with or without unity window manager)
  // read http://www.opengl.org/registry/specs/ARB/glx_create_context.txt
  static const int context_attrib_list[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
    GLX_CONTEXT_MINOR_VERSION_ARB, 1,
    0};

  // Setup X window and GLX context
  if (!window.createWindow(NULL, 400, 200, config_attrib_list, context_attrib_list, NULL))
  {
    fprintf(stderr, "Error: could not create window\n");
    return 1;
  }
  printf("GL Version = %s\n", glGetString(GL_VERSION));
  rcx.setupGLState();
  rcx.resized(400, 200);

  // Execute loop the whole time the app runs
  while (running)
  {
    XEvent newEvent;
    XWindowAttributes winData;

    // Watch for new X events
    XNextEvent(window.dpy, &newEvent);
    switch (newEvent.type)
    {
    case UnmapNotify:
      bWinMapped = False;
      break;
    case MapNotify :
      bWinMapped = True;
      break;
    case ConfigureNotify:
      XGetWindowAttributes(window.dpy, window.win, &winData);
      rcx.resized(winData.width, winData.height);
      break;
    case MotionNotify:
      rcx.mouseMove(newEvent.xmotion.x, newEvent.xmotion.y);
      break;
    case KeyPress:
    case DestroyNotify:
      running = False;
      break;
    }

    if (bWinMapped && running)
    {
      rcx.draw();
      window.swapBuffer();
    }
  }
  window.destroyWindow();
  // TODO: To fix close window button, use XChangeProperty(), add the WM_DELETE_WINDOW message to the WM_PROTOCOLS events.
  // This should avoid following error:
  // XIO:  fatal IO error 11 (Resource temporarily unavailable) on X server ":0"
  //   after 48 requests (48 known processed) with 0 events remaining.
  return 0;
}

