// GLXBasics.c
// Use GLX to setup OpenGL windows
// Draw eyeballs
// OpenGL SuperBible, 5th Edition
// Nick Haemel
// Modified by Alexandre Buge (Add errors checking and fix memory leak... wip).

#include <GL/glew.h>
#include <GL/glxew.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265

// Store all system info in one place
typedef struct
{
  GLXContext ctx;
  Display *dpy;
  Window win;
  int nWinWidth;
  int nWinHeight;
  int nMousePosX;
  int nMousePosY;
} RenderContext;

Bool EarlyInitGLXfnPointers()
{
  static Bool fnPointersInitialized = False;
  if (!fnPointersInitialized)
  {
    glXCreateContextAttribsARB = 
      (GLXContext(*)(Display* dpy, GLXFBConfig config, GLXContext share_context, Bool direct, const int *attrib_list))
      glXGetProcAddressARB((GLubyte*)"glXCreateContextAttribsARB");
    if (!glXCreateContextAttribsARB)
    {
      fprintf(stderr, "ERROR: unsupported GLX function glXCreateContextAttribsARB\n");
      return False;
    }
    glXChooseFBConfig = (GLXFBConfig*(*)(Display *dpy, int screen, const int *attrib_list, int *nelements))
      glXGetProcAddressARB((GLubyte*)"glXChooseFBConfig");
    if (!glXChooseFBConfig)
    {
      fprintf(stderr, "ERROR: unsupported GLX function glXChooseFBConfig\n");
      return False;
    }
    glXGetVisualFromFBConfig = (XVisualInfo*(*)(Display *dpy, GLXFBConfig config))glXGetProcAddressARB((GLubyte*)"glXGetVisualFromFBConfig");
    if (!glXGetVisualFromFBConfig)
    {
      fprintf(stderr, "ERROR: unsupported GLX function glXGetVisualFromFBConfig\n");
      return False;
    }
    fnPointersInitialized = True;
  }
  return True;
}

/**
** Create an OpenGL Window.
** @param rcx is the in/out OpenGL windows structure.
** @display_name Specifies the hardware display name, which determines the display and communications domain to be used. 
   On a POSIX-conformant system, if the display_name is NULL, it defaults to the value of the DISPLAY environment variable. 
** @param config_attrib_list specifies a list of attribute/value pairs. The last attribute must be None. 
   (see http://www.opengl.org/sdk/docs/man/xhtml/glXChooseFBConfig.xml)
** @param  contex_attrib_list specifies a list of attributes for the context. The list consists of a sequence of <name,value> pairs terminated by the
   value None (0). If an attribute is not specified, then the default value specified below is used instead.
   (see http://www.opengl.org/registry/specs/ARB/glx_create_context.txt)
** @return True on succes else false on error, see stderr for details. 
*/
Bool CreateWindow(RenderContext *rcx, const char* display_name, const int* config_attrib_list, const int* context_attrib_list)
{
  XSetWindowAttributes winAttribs;
  GLint winmask;
  GLint nMajorVer = 0;
  GLint nMinorVer = 0;
  XVisualInfo *visualInfo;
  GLXFBConfig *fbConfigs;
  int numConfigs = 0;

  if (!EarlyInitGLXfnPointers())
  {
    fprintf(stderr, "ERROR: Failed to initialize GLX\n");
    XCloseDisplay(rcx->dpy);
    return False;
  }  
  // Tell X we are going to use the display
  rcx->dpy = XOpenDisplay(display_name);
  if (!rcx->dpy)
  {
    fprintf(stderr, "ERROR: XOpenDisplay(%s) failure\n", display_name);
    return False;
  }
  // Get Version info
  if (!glXQueryVersion(rcx->dpy, &nMajorVer, &nMinorVer))
  {
    fprintf(stderr, "ERROR: glXQueryVersion failure\n");
    XCloseDisplay(rcx->dpy);
    return False;
  }
  if(nMajorVer == 1 && nMinorVer < 2)
  {
    fprintf(stderr, "ERROR: GLX 1.2 or greater is necessary\n");
    XCloseDisplay(rcx->dpy);
    return False;
  }
  // Get a new fb config that meets our attrib requirements
  // TODO let caller decide if whe need to use DefaultScreen or not.
  fbConfigs = glXChooseFBConfig(rcx->dpy, DefaultScreen(rcx->dpy), config_attrib_list, &numConfigs);
  if (!fbConfigs)
  {
    fprintf(stderr, "ERROR: glXChooseFBConfig failure\n");
    XCloseDisplay(rcx->dpy);
    return False;
  }
  // Get visual info from best config.
  visualInfo = glXGetVisualFromFBConfig(rcx->dpy, fbConfigs[0]);
  if (!visualInfo)
  {
    XFree(fbConfigs);
    fprintf(stderr, "ERROR: glXGetVisualFromFBConfig failure\n");
    XCloseDisplay(rcx->dpy);
    return False;
  }
  // Now create an X window
  // TODO use XSetErrorHandler to manage XCreateColormap errors.
  winAttribs.colormap = XCreateColormap(rcx->dpy, 
                                        RootWindow(rcx->dpy, visualInfo->screen), 
                                        visualInfo->visual, AllocNone);
  // TODO expose all configurable attribute to function parameters
  winAttribs.event_mask = ExposureMask | VisibilityChangeMask | 
                          KeyPressMask | PointerMotionMask    |
                          StructureNotifyMask;

  winAttribs.border_pixel = 0;
  winAttribs.bit_gravity = StaticGravity;
  winmask = CWBorderPixel | CWBitGravity | CWEventMask| CWColormap;

  // TODO use XSetErrorHandler to manage XCreateColormap errors.
  rcx->win = XCreateWindow(rcx->dpy, DefaultRootWindow(rcx->dpy), 20, 20,
               rcx->nWinWidth, rcx->nWinHeight, 0, 
                           visualInfo->depth, InputOutput,
               visualInfo->visual, winmask, &winAttribs);
  XFree(visualInfo);
  // TODO use XSetErrorHandler to manage XMapWindow errors.
  XMapWindow(rcx->dpy, rcx->win);

  // Create a  GL context for rendering
  rcx->ctx = glXCreateContextAttribsARB(rcx->dpy, fbConfigs[0], 0, True, context_attrib_list);
  XFree(fbConfigs);
  if (!rcx->ctx)
  {
    fprintf(stderr, "ERROR: glXCreateContextAttribsARB failure\n");
    XDestroyWindow(rcx->dpy, rcx->win);
    XCloseDisplay(rcx->dpy);
    return False;
  }
  if (!glXMakeCurrent(rcx->dpy, rcx->win, rcx->ctx))
  {
    fprintf(stderr, "ERROR: glXMakeCurrent failure\n");
    XDestroyWindow(rcx->dpy, rcx->win);
    XCloseDisplay(rcx->dpy);
    return False;
  }

  // Glew init
  GLenum err = glewInit();
  if (GLEW_OK != err)
  {
      /* Problem: glewInit failed, something is seriously wrong. */
    fprintf(stderr, "Error: glewInit failure %u: %s\n", err, glewGetErrorString(err)); 
    glXMakeCurrent(rcx->dpy, None, NULL);
    XDestroyWindow(rcx->dpy, rcx->win);
    XCloseDisplay(rcx->dpy);
    return False;
  } 
  return True;
}

void SetupGLState(RenderContext *rcx)
{
  float aspectRatio = rcx->nWinHeight ? (float)rcx->nWinWidth/(float)rcx->nWinHeight : 1.0f;
  float fYTop     = 0.05f;
  float fYBottom  = - fYTop;
  float fXLeft    = fYTop     * aspectRatio;
  float fXRight   = fYBottom  * aspectRatio;

  glViewport(0,0,rcx->nWinWidth,rcx->nWinHeight);
  glScissor(0,0,rcx->nWinWidth,rcx->nWinHeight);

  glClearColor(0.0f, 1.0f, 1.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Clear matrix stack
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
      
  // Set the frustrum
  glFrustum(fXLeft, fXRight, fYBottom, fYTop, 0.1f, 100.f);
}

void DrawCircle()
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


void Draw(RenderContext *rcx)
{
  float fRightX = 0.0;
  float fRightY = 0.0;
  float fLeftX = 0.0;
  float fLeftY = 0.0;

  int nLtEyePosX = (rcx->nWinWidth)/2 - (0.3 * ((rcx->nWinWidth)/2));
  int nLtEyePosY = (rcx->nWinHeight)/2;
  int nRtEyePosX = (rcx->nWinWidth)/2 + (0.3 * ((rcx->nWinWidth)/2));
  int nRtEyePosY = (rcx->nWinHeight)/2;

  double fLtVecMag = 100;
  double fRtVecMag = 100;

  // Use the eyeball width as the minimum
  double fMinLength =  (0.1 * ((rcx->nWinWidth)/2));

  // Calculate the length of the vector from the eyeball
  // to the mouse pointer
  fLtVecMag = sqrt( pow((double)(rcx->nMousePosX - nLtEyePosX), 2.0) + 
                    pow((double)(rcx->nMousePosY - nLtEyePosY), 2.0));

  fRtVecMag =  sqrt( pow((double)(rcx->nMousePosX - nRtEyePosX), 2.0) + 
                    pow((double)(rcx->nMousePosY - nRtEyePosY), 2.0));

  // Clamp the minimum magnatude
  if (fRtVecMag < fMinLength)
    fRtVecMag = fMinLength;
  if (fLtVecMag < fMinLength)
    fLtVecMag = fMinLength;

  // Normalize the vector components
  fLeftX = (float)(rcx->nMousePosX - nLtEyePosX) / fLtVecMag;
  fLeftY = -1.0 * (float)(rcx->nMousePosY - nLtEyePosY) / fLtVecMag;
  fRightX = (float)(rcx->nMousePosX - nRtEyePosX) / fRtVecMag;
  fRightY = -1.0 * ((float)(rcx->nMousePosY - nRtEyePosY) / fRtVecMag);

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
  DrawCircle();

  // Draw black
  glColor3f(0.0, 0.0, 0.0);
  glScalef(0.40, 0.40, 1.0);
  glTranslatef(fLeftX, fLeftY, 0.0);
  DrawCircle();

  // Draw right eyeball
  glColor3f(1.0, 1.0, 1.0);
  glLoadIdentity();
  glScalef(0.20, 0.20, 1.0);
  glTranslatef(1.5, 0.0, 0.0);
  DrawCircle();

  // Draw black
  glColor3f(0.0, 0.0, 0.0);
  glScalef(0.40, 0.40, 1.0);
  glTranslatef(fRightX, fRightY, 0.0);
  DrawCircle();

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

  // Display rendering
  glXSwapBuffers(rcx->dpy, rcx->win);    
}

void Cleanup(RenderContext *rcx)
{
  glXMakeCurrent(rcx->dpy, None, NULL);
  glXDestroyContext(rcx->dpy, rcx->ctx);
  XDestroyWindow(rcx->dpy, rcx->win);
  XCloseDisplay(rcx->dpy);
}

int main()
{
  Bool bWinMapped = False;
  Bool running = True;
  RenderContext rcx;
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

  // Set initial window properties
  rcx.nWinWidth = 400;
  rcx.nWinHeight = 200;
  rcx.nMousePosX = 0;
  rcx.nMousePosY = 0;

  // Setup X window and GLX context
  if (!CreateWindow(&rcx, NULL, config_attrib_list, context_attrib_list))
  {
    fprintf(stderr, "Error: could not create window\n");
    return 1;
  }
  printf("GL Version = %s\n", glGetString(GL_VERSION));
  SetupGLState(&rcx);

  // Execute loop the whole time the app runs
  while (running)
  {
      XEvent newEvent;
      XWindowAttributes winData;

      // Watch for new X events
      XNextEvent(rcx.dpy, &newEvent);
      switch (newEvent.type)
      {
      case UnmapNotify:
        // WHY it's no called ? Need an explicit call to XUnmapWindow ?
        bWinMapped = False;
        break;
      case MapNotify :
        bWinMapped = True;
        break;
      case ConfigureNotify:
        XGetWindowAttributes(rcx.dpy, rcx.win, &winData);
        rcx.nWinHeight = winData.height;
        rcx.nWinWidth = winData.width;
        SetupGLState(&rcx);
        break;
      case MotionNotify:
        rcx.nMousePosX = newEvent.xmotion.x;
        rcx.nMousePosY = newEvent.xmotion.y;
        break;
      case KeyPress:
      case DestroyNotify:
        running = False;
        break;
      }

      if (bWinMapped && running)
        Draw(&rcx);
  }
  Cleanup(&rcx);
  // TODO: undestand why quitting with window close button send: UnmapNotify problem ?
  // XIO:  fatal IO error 11 (Resource temporarily unavailable) on X server ":0"
  //   after 48 requests (48 known processed) with 0 events remaining.
  return 0;
}

