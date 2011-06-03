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

typedef struct //OpenGLXWindow
{
  // internal
  Bool earlyInitinializeGLX()
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
  ** @display_name Specifies the hardware display name, which determines the display and communications domain to be used.
  ** On a POSIX-conformant system, if the display_name is NULL, it defaults to the value of the DISPLAY environment variable.
  ** @param config_attrib_list specifies a list of attribute/value pairs. The last attribute must be None.
  ** (see http://www.opengl.org/sdk/docs/man/xhtml/glXChooseFBConfig.xml)
  ** @param  contex_attrib_list specifies a list of attributes for the context. The list consists of a sequence of <name,value> pairs terminated by the
  ** value None (0). If an attribute is not specified, then the default value specified below is used instead.
  ** (see http://www.opengl.org/registry/specs/ARB/glx_create_context.txt)
  ** @param share_context is not NULL, then all shareable data (excluding OpenGL texture objects named 0) will be shared.
  ** @return True on succes else false on error, see stderr for details.
  */
  Bool createWindow(const char* display_name, int width, int height, 
                    const int* config_attrib_list, const int* context_attrib_list, GLXContext share_context)
  {
    XSetWindowAttributes winAttribs;
    GLint nMajorVer;
    GLint nMinorVer;
    XVisualInfo* visualInfo;
    GLXFBConfig* fbConfigs;
    int numConfigs;

    if (!earlyInitinializeGLX())
    {
      fprintf(stderr, "ERROR: Failed to initialize GLX\n");
      XCloseDisplay(dpy);
      return False;
    }
    // Tell X we are going to use the display
    dpy = XOpenDisplay(display_name);
    if (!dpy)
    {
      fprintf(stderr, "ERROR: XOpenDisplay(%s) failure\n", display_name);
      return False;
    }
    // Get GLX Version
    if (!glXQueryVersion(dpy, &nMajorVer, &nMinorVer))
    {
      fprintf(stderr, "ERROR: glXQueryVersion failure\n");
      XCloseDisplay(dpy);
      return False;
    }
    if(nMajorVer == 1 && nMinorVer < 2)
    {
      fprintf(stderr, "ERROR: GLX 1.2 or greater is necessary\n");
      XCloseDisplay(dpy);
      return False;
    }
    // Get a new fb config that meets our attrib requirements
    // TODO let caller decide if whe need to use DefaultScreen or not.
    fbConfigs = glXChooseFBConfig(dpy, DefaultScreen(dpy), config_attrib_list, &numConfigs);
    if (!fbConfigs)
    {
      fprintf(stderr, "ERROR: glXChooseFBConfig failure\n");
      XCloseDisplay(dpy);
      return False;
    }
    // Get visual info from best config.
    visualInfo = glXGetVisualFromFBConfig(dpy, fbConfigs[0]);
    if (!visualInfo)
    {
      XFree(fbConfigs);
      fprintf(stderr, "ERROR: glXGetVisualFromFBConfig failure\n");
      XCloseDisplay(dpy);
      return False;
    }
    // Now create an X window
    // TODO use XSetErrorHandler to manage XCreateColormap errors.
    winAttribs.colormap = XCreateColormap(dpy,
                                          RootWindow(dpy, visualInfo->screen),
                                          visualInfo->visual, AllocNone);
    // TODO expose all configurable attribute to function parameters
    winAttribs.event_mask = ExposureMask | VisibilityChangeMask |
                            KeyPressMask | PointerMotionMask    |
                            StructureNotifyMask;
    winAttribs.border_pixel = 0; // TODO understand why modification have no impact.
    winAttribs.bit_gravity = StaticGravity;

    // TODO use XSetErrorHandler to manage XCreateColormap errors.
    // TODO understand why x, y and border size have no effect.
    win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0,
                 width, height, 0, visualInfo->depth, InputOutput,
                 visualInfo->visual, CWBorderPixel | CWBitGravity | CWEventMask| CWColormap, &winAttribs);
    XFree(visualInfo);
    // TODO use XSetErrorHandler to manage XMapWindow errors.
    XMapWindow(dpy, win);

    // Create a  GL context for rendering
    ctx = glXCreateContextAttribsARB(dpy, fbConfigs[0], share_context, True, context_attrib_list);
    XFree(fbConfigs);
    if (!ctx)
    {
      fprintf(stderr, "ERROR: glXCreateContextAttribsARB failure\n");
      XDestroyWindow(dpy, win);
      XCloseDisplay(dpy);
      return False;
    }
    if (!glXMakeCurrent(dpy, win, ctx))
    {
      fprintf(stderr, "ERROR: glXMakeCurrent failure\n");
      XDestroyWindow(dpy, win);
      XCloseDisplay(dpy);
      return False;
    }

    // Glew init
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        /* Problem: glewInit failed, something is seriously wrong. */
      fprintf(stderr, "Error: glewInit failure %u: %s\n", err, glewGetErrorString(err));
      glXMakeCurrent(dpy, None, NULL);
      XDestroyWindow(dpy, win);
      XCloseDisplay(dpy);
      return False;
    }
    return True;
  }

  Bool destroyWindow()
  {
    Bool res = True;
    if (glXMakeCurrent(dpy, None, NULL))
      res = False;
    glXDestroyContext(dpy, ctx);
    XDestroyWindow(dpy, win);
    XCloseDisplay(dpy);
    return res;
  }

  void swapBuffer()
    {glXSwapBuffers(dpy, win);}

  GLXContext ctx;
  Display* dpy;
  Window win;

} OpenGLXWindow;

#endif // DOCGLX_H_


