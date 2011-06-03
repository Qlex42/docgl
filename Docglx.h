/* -------------------------------- . ---------------------------------------- .
| Filename : Docglx.h               | DOCGLX: D-LABS XLIB DocGL extensions     |
| Author   : Alexandre Buge         | Managing multicontext OpenGL Windows     |
| Started  : 03/06/2011 13:25       | With Advanced pixel formats              |
` --------------------------------- . ----------------------------------------*/
#ifndef DOCGLX_H_
# define DOCGLX_H_

//# include "Docgl.h"
# include <GL/glxew.h>
# include <cstdio> // TODO REMOVE IO ?
# include <X11/Xatom.h>

namespace docglx
{


/**
** OpenGL Window message windowCallback class.
** All windowCallback function is called in the dispatching thread with current window OpenGL context.
*/
class OpenGLWindowCallback
{
public:
  virtual ~OpenGLWindowCallback() {}
  virtual void draw() {}
  virtual void keyPressed(unsigned char /* key */) {}
  virtual void resized(int /* width */, int /* height */) {}
  virtual void mouseMove(int /* x */ , int /* y */) {}
  virtual void closed() {}
};

typedef struct //OpenGLXWindow
{
  // internal
  Bool earlyInitinializeGLX()
  {
    static Bool fnPointersInitialized = False;
    if (!fnPointersInitialized)
    {
      glXCreateContextAttribsARB =
        (GLXContext(*)(Display* display, GLXFBConfig config, GLXContext shareContext, Bool direct, const int *attrib_list))
        glXGetProcAddressARB((GLubyte*)"glXCreateContextAttribsARB");
      if (!glXCreateContextAttribsARB)
      {
        fprintf(stderr, "ERROR: unsupported GLX function glXCreateContextAttribsARB\n");
        return False;
      }
      glXChooseFBConfig = (GLXFBConfig*(*)(Display *display, int screen, const int *attrib_list, int *nelements))
        glXGetProcAddressARB((GLubyte*)"glXChooseFBConfig");
      if (!glXChooseFBConfig)
      {
        fprintf(stderr, "ERROR: unsupported GLX function glXChooseFBConfig\n");
        return False;
      }
      glXGetVisualFromFBConfig = (XVisualInfo*(*)(Display *display, GLXFBConfig config))glXGetProcAddressARB((GLubyte*)"glXGetVisualFromFBConfig");
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
  ** @windowCallback is the windowCallback message handler who intercept event.
  ** @display_name Specifies the hardware display name, which determines the display and communications domain to be used.
  ** On a POSIX-conformant system, if the display_name is NULL, it defaults to the value of the DISPLAY environment variable.
  ** @param pixelFormatAttributes specifies a list of attribute/value pairs. The last attribute must be None.
  ** (see http://www.opengl.org/sdk/docs/man/xhtml/glXChooseFBConfig.xml)
  ** @param  contex_attrib_list specifies a list of attributes for the context. The list consists of a sequence of <name,value> pairs terminated by the
  ** value None (0). If an attribute is not specified, then the default value specified below is used instead.
  ** (see http://www.opengl.org/registry/specs/ARB/glx_create_context.txt)
  ** @param shareContext is not NULL, then all shareable data (excluding OpenGL texture objects named 0) will be shared.
  ** @return True on succes else false on error, see stderr for details.
  */
  Bool create(OpenGLWindowCallback& windowCallback, const char* display_name, int width, int height, 
                    const int* pixelFormatAttributes, const int* contextAttributes, GLXContext shareContext)
  {
    this->windowCallback = &windowCallback;
    XSetWindowAttributes winAttribs;
    GLint nMajorVer;
    GLint nMinorVer;
    XVisualInfo* visualInfo;
    GLXFBConfig* fbConfigs;
    int numConfigs;

    mapped = False;
    if (!earlyInitinializeGLX())
    {
      fprintf(stderr, "ERROR: Failed to initialize GLX\n");
      XCloseDisplay(display);
      return False;
    }
    // Tell X we are going to use the display
    display = XOpenDisplay(display_name);
    if (!display)
    {
      fprintf(stderr, "ERROR: XOpenDisplay(%s) failure\n", display_name);
      return False;
    }
    // Get GLX Version
    if (!glXQueryVersion(display, &nMajorVer, &nMinorVer))
    {
      fprintf(stderr, "ERROR: glXQueryVersion failure\n");
      XCloseDisplay(display);
      return False;
    }
    if(nMajorVer == 1 && nMinorVer < 2)
    {
      fprintf(stderr, "ERROR: GLX 1.2 or greater is necessary\n");
      XCloseDisplay(display);
      return False;
    }
    // Get a new fb config that meets our attrib requirements
    // TODO let caller decide if whe need to use DefaultScreen or not.
    fbConfigs = glXChooseFBConfig(display, DefaultScreen(display), pixelFormatAttributes, &numConfigs);
    if (!fbConfigs)
    {
      fprintf(stderr, "ERROR: glXChooseFBConfig failure\n");
      XCloseDisplay(display);
      return False;
    }
    // Get visual info from best config.
    visualInfo = glXGetVisualFromFBConfig(display, fbConfigs[0]);
    if (!visualInfo)
    {
      XFree(fbConfigs);
      fprintf(stderr, "ERROR: glXGetVisualFromFBConfig failure\n");
      XCloseDisplay(display);
      return False;
    }
    // Now create an X window
    // TODO use XSetErrorHandler to manage XCreateColormap errors.
    winAttribs.colormap = XCreateColormap(display,
                                          RootWindow(display, visualInfo->screen),
                                          visualInfo->visual, AllocNone);
    // TODO expose all configurable attribute to function parameters
    winAttribs.event_mask = ExposureMask | VisibilityChangeMask |
                            KeyPressMask | PointerMotionMask    |
                            StructureNotifyMask;
    winAttribs.border_pixel = 0; // TODO understand why modification have no impact.
    winAttribs.bit_gravity = StaticGravity;

    // TODO use XSetErrorHandler to manage XCreateColormap errors.
    // TODO understand why x, y and border size have no effect.
    window = XCreateWindow(display, DefaultRootWindow(display), 0, 0,
                 width, height, 0, visualInfo->depth, InputOutput,
                 visualInfo->visual, CWBorderPixel | CWBitGravity | CWEventMask| CWColormap, &winAttribs);
    XFree(visualInfo);

    // allow WM_DELETE_WINDOW event interception. TODO: cache computation of XInternAtoms statically and reuse them on eventHandler
    Atom protocols = XInternAtom(display, "WM_PROTOCOLS", 1);
    Atom deleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", 1);
    XChangeProperty(display, window, protocols, XA_ATOM, 32, PropModeReplace, (const unsigned char*) &deleteWindow, 1);

    // TODO use XSetErrorHandler to manage XMapWindow errors.
    XMapWindow(display, window);

    // Create a  GL context for rendering
    glxContext = glXCreateContextAttribsARB(display, fbConfigs[0], shareContext, True, contextAttributes);
    XFree(fbConfigs);
    if (!glxContext)
    {
      fprintf(stderr, "ERROR: glXCreateContextAttribsARB failure\n");
      XDestroyWindow(display, window);
      XCloseDisplay(display);
      return False;
    }
    if (!glXMakeCurrent(display, window, glxContext))
    {
      fprintf(stderr, "ERROR: glXMakeCurrent failure\n");
      XDestroyWindow(display, window);
      XCloseDisplay(display);
      return False;
    }

    // Glew init
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: glewInit failure %u: %s\n", err, glewGetErrorString(err));
      glXMakeCurrent(display, None, NULL);
      XDestroyWindow(display, window);
      XCloseDisplay(display);
      return False;
    }
    return True;
  }

  /**
  ** Destroy an OpenGL Window created with create() function
  ** @return True on succed else False on error.
  */ 
  Bool destroy()
  {
    Bool res = True;
    if (glXMakeCurrent(display, None, NULL))
      res = False;
    glXDestroyContext(display, glxContext);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    return res;
  }

  /**
  ** Swap OpenGL window buffers (ensure double buffering is active)
  */ 
  void swapBuffer()
    {glXSwapBuffers(display, window);}
  
  /**
  ** Internal event handler, do not call it directly use EventDispatcher instead
  ** @param newEvent is the event to interpret.
  */ 
  void eventHandler(const XEvent& newEvent)
  {
    XWindowAttributes winData;
    switch (newEvent.type)
    {
    case UnmapNotify:
      mapped = False;
      break;
    case MapNotify :
      mapped = True;
      break;
    case ConfigureNotify:
      XGetWindowAttributes(display, window, &winData);
      windowCallback->resized(winData.width, winData.height);
      break;
    case MotionNotify:
      windowCallback->mouseMove(newEvent.xmotion.x, newEvent.xmotion.y);
      break;
    case KeyPress:
      windowCallback->keyPressed(0 /*TODO compute code*/);
      break;
    case ClientMessage:
      { 
        const XClientMessageEvent* const clientMsg = (const XClientMessageEvent*) &newEvent.xclient;
        if (clientMsg->message_type == XInternAtom (display, "WM_PROTOCOLS", 1) && clientMsg->format == 32)
        {
          const Atom atom = (Atom) clientMsg->data.l[0];
          if (atom == XInternAtom(display, "WM_DELETE_WINDOW", 1))
          {
             mapped = False;
             windowCallback->closed();
          }
        }
      }
      break;
    }
  }

  GLXContext glxContext;
  Display* display;
  Window window;
  bool mapped;
  OpenGLWindowCallback* windowCallback;

} OpenGLXWindow;


//////////////////////////////// TOOLS ////////////////////////////////////////

class EventDispatcher
{
public:
  // Event dispatcher initializing
  // @param windowsList is an array of previously created windows with the same Display.
  // @param windowsCount is the window count on the windowsList.
  EventDispatcher(OpenGLXWindow* windowsList, size_t windowsCount)
    : windowsList(windowsList), windowsCount(windowsCount)
  {
    if (windowsCount)
      display = windowsList[0].display;
  }

  /**
  ** Dispacth the next process message of from display of the windows list.
  */ 
  void dispatchNextEvent()
  {
    XEvent newEvent;
    XNextEvent(display, &newEvent);
    for (size_t i = 0; i < windowsCount; ++i)
      if (newEvent.xany.window == windowsList[0].window)
      {
        windowsList[0].eventHandler(newEvent);
        break;
      }
  } 

  OpenGLXWindow* windowsList;
  size_t windowsCount;
  Display* display;
};

}; // namespace docglx

#endif // DOCGLX_H_


