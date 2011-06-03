/* -------------------------------- . ---------------------------------------- .
| Filename : DocglWindow.h          | D-LABS cross plateform DocGL Windows     |
| Author   : Alexandre Buge         |                                          |
| Started  : 03/06/2011 17:22       |                                          |
` --------------------------------- . ----------------------------------------*/
#ifndef DOCGL_WINDOW_H_
# define DOCGL_WINDOW_H_

# ifdef WIN32

#  include <Docgl/Docwgl.h>

struct OpenGLWindow
{
public:
  OpenGLWindow(docgl::GLContext& context)
    : window(context) {}
  bool create(OpenGLWindowCallback& windowCallback, int x, int y, int width, int height, const char* className)
  {
    this->className = className;
    if(!docwgl::registerOpenWGLWindowClass(className, GetModuleHandle(NULL), LoadCursor(NULL, IDC_ARROW)))
      return false;

    static const int pixelFormatAttributes[] = {
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
    static const int contextAttributes[] = {
  #ifdef DOCGL4_1
      WGL_CONTEXT_MAJOR_VERSION_ARB,  4,
      WGL_CONTEXT_MINOR_VERSION_ARB,  1,
  #else // !DOCGL4_1
      WGL_CONTEXT_MAJOR_VERSION_ARB,  3,
      WGL_CONTEXT_MINOR_VERSION_ARB,  3,
  #endif // !DOCGL4_1
      WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
      //WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
      0};

    DWORD windowStyle = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE;
    DWORD extendedWindowStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

    // ajust windows size and position depending to wanted client size and position.
    RECT clientRect;
    clientRect.left = x;
    clientRect.top = y;
    clientRect.right = x + width;
    clientRect.bottom = y + height;
    RECT windowRect = clientRect;
    AdjustWindowRectEx(&windowRect, windowStyle, FALSE, extendedWindowStyle);
    OffsetRect(&windowRect, clientRect.left - windowRect.left, clientRect.top - windowRect.top);

    if (!window.create(windowCallback, className, windowRect, windowStyle,
      extendedWindowStyle, pixelFormatAttributes, contextAttributes))
    {
      jassertfalse;
      return false;
    }
    return true;
  }

  bool destroy()
  {
    bool res = true;
    if (!window.destroy())
      res = false;
    // Delete the window class
    if (!UnregisterClass(className, GetModuleHandle(NULL)))
      res = false;
    return res;
  }

  bool swapBuffers()
    {return window.swapBuffers() != FALSE ? true : false;}

  bool isVisible() const
    {return window.isVisible() != FALSE ? true : false;}

private:
  docwgl::OpenWGLWindow window;
  const char* className;
};

class EventDispatcher
{
public:
  EventDispatcher(OpenGLWindow** windowsList, size_t windowsCount) {}

  /**
  ** Dispatch next messages.
  ** @return true if a message was dispatched, false if the message queue is empty.
  */
  bool dispatchNextEvent()
  {
    BOOL threadCanLoop; // ignore
    return docwgl::dispatchNextThreadMessage(threadCanLoop) ? true : false;
  }
};

# else // !WIN32

#  ifdef linux

#   include "Docglx.h"

struct OpenGLWindow
{
public:
  OpenGLWindow(docgl::GLContext& context)
    : window() {}
  bool create(OpenGLWindowCallback& windowCallback, int x, int y, int width, int height, const char* className)
  {
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
    if (!window.create(windowCallback, NULL, width, height, config_attrib_list, context_attrib_list, NULL))
    {
      fprintf(stderr, "Error: could not create window\n");
      return 1;
    }
    return true;
  }

  bool destroy()
    {return window.destroy();}

  bool swapBuffers()
  {
    window.swapBuffer();
    return true;
  }

  bool isVisible() const
    {return window.mapped;}

  docglx::OpenGLXWindow window;
private:
  const char* className;
};


class EventDispatcher
{
public:
  EventDispatcher(OpenGLWindow** windowsList, size_t windowsCount)
    : eventDispatcher(getInternalWindowList(windowsList, windowsCount), windowsCount) {}

  ~EventDispatcher()
  {
    if (internalWindowList)
      delete [] internalWindowList;
  }


  /**
  ** Dispatch next messages.
  ** @return true if a message was dispatched, false if the message queue is empty.
  */
  bool dispatchNextEvent()
  {
    eventDispatcher.dispatchNextEvent();
    return false; // return false to enforce unix client drawing.
  }

private:
  docglx::OpenGLXWindow** getInternalWindowList(OpenGLWindow** windowsList, size_t windowsCount)
  {
    internalWindowList = NULL;
    if (windowsCount)
    {
      internalWindowList = new docglx::OpenGLXWindow*[windowsCount];
      for (size_t i = 0; i < windowsCount; ++i)
        internalWindowList[i] = &windowsList[i]->window;
    }
    return internalWindowList;
  }

private:
  docglx::XEventDispatcher eventDispatcher;
  docglx::OpenGLXWindow** internalWindowList;
};


#  else  // !linux

#   error "plateform not supported"

#  endif // !linux
# endif // !WIN32


#endif // DOCGL_WINDOW_H_
