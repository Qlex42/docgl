/* -------------------------------- . ---------------------------------------- .
| Filename : Docwgl.h               | DOCWGL: D-LABS Windows DocGL extensions  |
| Author   : Alexandre Buge         | Managing multicontext OpenGL Windows     |
| Started  : 26/02/2011 23:09       | With Advanced pixel formats              |
` --------------------------------- . ----------------------------------------*/
#ifndef DOCWGL_H_
# define DOCWGL_H_

# include "Docgl.h"
# include <gl\wglew.h>

namespace docwgl
{

struct OpenGLWindow
{
  /**
  ** Create an OpenGL window this specified OpenGL context and pixel format.
  ** @param className specifies the window class name. The class name can be any name registered with registerOpenGLWindowClass.
  ** @param bounds is the dimension of the windows in the desktop (take care of location for choosing good GPU)
  ** @param style is the window style. For a list of possible values, see http://msdn.microsoft.com/en-us/library/ms632600.aspx
  ** @param extentedStyle is the extended style. For a list of possible values, see http://msdn.microsoft.com/en-us/library/ff700543.aspx
  ** @param pixelFormatAttributes is pixel format attributes. For a list of possible values, see http://www.opengl.org/registry/specs/ARB/wgl_pixel_format.txt
  ** @param contextAttributes is OpenGL context attributes. For a list of possible values, see http://www.opengl.org/registry/specs/ARB/wgl_create_context.txt
  ** @param shareContext is not 0, then all shareable data will be shared by shareContext,
            all other contexts shareContext already shares with, and the newly created context.
  ** @param caption is the text in the title bar.
  ** @param module is an handle to the instance of the module to be associated with the window.
  ** @param parentWindow indentifies a handle to the parent or owner window of the window being created.
  ** @param menu identifies the menu to be used with the window. It can be NULL if the class menu is to be used.
  ** @return TRUE on succeed. Call OpenGLWindow::destroy for freeing required memory. 
  **   Else return FALSE. Call GetLastError to get extended error information.
  */
  BOOL create(LPCTSTR className, const RECT& bounds, DWORD style, DWORD extentedStyle, 
              const int* pixelFormatAttributes, const int* contextAttributes,
              HGLRC shareContext = NULL, LPCTSTR caption = NULL, 
              HINSTANCE module = NULL, HWND parentWindow = NULL, HMENU menu = NULL)
  {
    if (extentedStyle & (WS_EX_COMPOSITED | WS_EX_LAYERED))
      {SetLastError(ERROR_INVALID_FLAGS); return FALSE;} // conflict flags with Owner DC
    
    const int nWindowWidth = bounds.right - bounds.left;
    const int nWindowHeight = bounds.bottom - bounds.top;
    // create a window with dummy pixel format so that we can get access to wgl functions
    hWnd = CreateWindowEx(extentedStyle,                // Extended style
                            className,                  // class name
                            caption,                    // window name
                            style & ~WS_VISIBLE,        // window stlye
                            bounds.left,                // window position, x
                            bounds.top,                 // window position, y
                            nWindowWidth,               // height
                            nWindowHeight,              // width
                            parentWindow,               // Parent window
                            menu,                       // menu
                            module,                     // instance
                            NULL);                      // pass this to WM_CREATE
    if (hWnd == NULL)
      return FALSE; // see GetLastError
    hDC = GetDC(hWnd);
    if (hDC == NULL)
    {
      DestroyWindow(hWnd);
      hWnd = NULL;
      SetLastError(ERROR_DC_NOT_FOUND); return FALSE;
    }
    PIXELFORMATDESCRIPTOR pfd;
    memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));
    if (!SetPixelFormat(hDC, 1 /** dummy*/, &pfd))
    {
      const DWORD lastError = GetLastError();
      ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    // create an old school OpenGL context.
    hGLRC = wglCreateContext(hDC);
    if (hGLRC == NULL)
    {
      const DWORD lastError = GetLastError();
      ReleaseDC(hWnd, hDC); DestroyWindow(hWnd); 
      hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    if (!wglMakeCurrent(hDC, hGLRC))
    {
      const DWORD lastError = GetLastError();
      wglDeleteContext(hGLRC); ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hGLRC = NULL; hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    // Setup GLEW which loads OpenGL function pointers
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
      wglMakeCurrent(NULL, NULL); wglDeleteContext(hGLRC); ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hGLRC = NULL; hDC = NULL; hWnd = NULL;
      SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;// glew cannot find OpenGL function pointers
    }

# ifdef GLEW_MX 
    err = wglewContextInit(wglewGetContext());
    if (GLEW_OK != err)
    {
      wglMakeCurrent(NULL, NULL); wglDeleteContext(hGLRC); ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hGLRC = NULL; hDC = NULL; hWnd = NULL;
      SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;	// glew cannot find OpenGL function pointers
    }
#endif // GLEW_MX
    // Now glew is setup, delete dummy window.
    wglMakeCurrent(NULL, NULL); wglDeleteContext(hGLRC); ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
    hGLRC = NULL; hDC = NULL; hWnd = NULL;
    if (!wglewIsSupported("WGL_ARB_create_context WGL_ARB_pixel_format"))
      {SetLastError(ERROR_PROC_NOT_FOUND); return FALSE;}
    // Start over picking a real format.
    hWnd = CreateWindowEx(extentedStyle,                // Extended style
                            className,                  // class name
                            caption,                    // window name
                            style & ~WS_VISIBLE,        // window stlye
                            bounds.left,                // window position, x
                            bounds.top,                 // window position, y
                            nWindowWidth,               // height
                            nWindowHeight,              // width
                            parentWindow,               // Parent window
                            menu,                       // menu
                            module,                     // instance
                            NULL);                      // pass this to WM_CREATE
    if (hWnd == NULL)
      return FALSE; // see GetLastError
    hDC = GetDC(hWnd);
    if (hDC == NULL)
    {
      DestroyWindow(hWnd);
      hWnd = NULL;
      SetLastError(ERROR_DC_NOT_FOUND); return FALSE;
    }

    // Ask OpenGL to find the most relevant format matching our attribs
    int nPixelFormat;
    UINT nPixCount = 0;
    if (!wglChoosePixelFormatARB(hDC, pixelFormatAttributes, NULL, 1, &nPixelFormat, &nPixCount) || !nPixCount)
    {
      const DWORD lastError = GetLastError();
      ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    if (!SetPixelFormat(hDC, nPixelFormat, &pfd))
    {
      const DWORD lastError = GetLastError();
      ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    hGLRC = wglCreateContextAttribsARB(hDC, shareContext, contextAttributes);
    if (hGLRC == NULL)
    {
      const DWORD lastError = GetLastError();
      ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    if (!wglMakeCurrent(hDC, hGLRC))
    {
      const DWORD lastError = GetLastError();
      wglDeleteContext(hGLRC); ReleaseDC(hWnd, hDC); DestroyWindow(hWnd);
      hGLRC = NULL; hDC = NULL; hWnd = NULL;
      SetLastError(lastError); return FALSE;
    }
    // Defere Visible flags avoid some WNDPROC calls before binding OpenGL context.
    // WARNING: some WNDPROC calls are done before binding OpenGL context anyway.
    if (style & WS_VISIBLE)
      ShowWindow(hWnd, SW_SHOW);
    return TRUE;
  }

  /**
  ** Destroy an OpenGL Window created with OpenGLWindow::create
  ** @return TRUE on succeed. Else return FALSE. Call GetLastError to get extended error information.
  */
  BOOL destroy()
  { 
    BOOL retVal = TRUE;
    if (!wglMakeCurrent(NULL, NULL))
      retVal = FALSE;
    if (!wglDeleteContext(hGLRC))
      retVal = FALSE;
    hGLRC = NULL;
    if (!ReleaseDC(hWnd, hDC))
      retVal = FALSE;
    hDC = NULL;
    if (!DestroyWindow(hWnd))
      retVal = FALSE;
    hWnd = NULL;
    return retVal;
  }

  /**
  ** Swap buffer of double buffered window. Prefere glFlush on simple buffered one.
  ** @return TRUE on succeed. Else return FALSE. Call GetLastError to get extended error information.
  */
  BOOL swapBuffers()
    {return SwapBuffers(hDC);}

  OpenGLWindow(docgl::GLContext& context)
    : context(context), hWnd(NULL), hDC(NULL), hGLRC(NULL) {}

# ifdef GLEW_MX
  WGLEWContext wglewContext;
  WGLEWContext* wglewGetContext() const
    {return const_cast<WGLEWContext*>(&wglewContext);}

  GLEWContext* glewGetContext() const
    {return context.glewGetContext();}
# endif  //GLEW_MX

  docgl::GLContext& context;
  HWND         hWnd;
  HDC          hDC;
  HGLRC        hGLRC;
}; // struct OpenGLWindow

//////////////////////////////// TOOLS ////////////////////////////////////////

/**
** Registers a window class for subsequent use in calls to OpenGLWindow::create function.
** @param name specifies the window class name. The maximum length is 256.
** @param callbackModule is handle to the instance that contains the window procedure.
** @param messageCallback is a pointer to the window procedure.
**   Take care that some messageCallback call are for dummy test windows and before OpenGL init.
** @param cursor A handle to the class cursor. This must be a handle to a cursor resource.
**   If this is NULL, an application must explicitly set it whenever the mouse moves into the application's window. 
** @param icon is a handle to the class icon. This must be a handle to an icon resource.
** @param smallIcon is a handle to the class small icon. This must be a handle to an icon resource.
** @param menuName specifies the resource name of the class menu, as the name appears in the resource file. 
**   If you use an integer to identify the menu, use the MAKEINTRESOURCE macro.
** @param haveCloseMenu define if close menu item is shown.
** @return TRUE on succeed. Call UnregisterClass for freeing class required memory. 
**   Else return FALSE. Call GetLastError to get extended error information.
*/
BOOL registerOpenGLWindowClass(LPCTSTR name, HINSTANCE callbackModule, WNDPROC messageCallback, 
                         HCURSOR cursor = NULL, HICON icon = NULL, HICON smallIcon = NULL, 
                         LPCTSTR menuName = NULL, bool haveCloseMenu = true)
{
  WNDCLASSEX windowClassProperties;
  windowClassProperties.cbSize = sizeof(WNDCLASSEX);
  windowClassProperties.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  if (!haveCloseMenu)
    windowClassProperties.style |= CS_NOCLOSE;
  windowClassProperties.lpfnWndProc = messageCallback;
  windowClassProperties.cbClsExtra = 0;
  windowClassProperties.cbWndExtra = 0;
  windowClassProperties.hInstance = callbackModule;
  windowClassProperties.hIcon = icon;
  windowClassProperties.hCursor = cursor;
  windowClassProperties.hbrBackground = NULL;
  windowClassProperties.lpszMenuName = menuName;
  windowClassProperties.lpszClassName = name;
  windowClassProperties.hIconSm = smallIcon;
  return RegisterClassEx(&windowClassProperties) != 0;
}

/**
** Dispatch next thread messages (or windows messages owned by this thread)
** @param threadCanLoop output define if the thread loop must exit.
** @return TRUE if a message was dispatched, FALSE if the message queue is empty.
*/
BOOL dispatchNextThreadMessage(BOOL& threadCanLoop)
{
  MSG		msg;
  threadCanLoop = TRUE;
  CONST BOOL returnValue = PeekMessage(&msg, NULL, 0, 0, PM_REMOVE);
  if (returnValue)
  {
    if (msg.message == WM_QUIT)
      threadCanLoop = FALSE;
    else
    {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return returnValue;
}

}; // namespace docwgl

#endif // DOCWGL_H_


//////////////////////////////////////////////
// TODO : 
// Fix noisy WNDPROC call for dummy Windows
