/* -------------------------------- . ---------------------------------------- .
| Filename : DocglWindowCallback.h  | D-LABS DocGL Windows callback            |
| Author   : Alexandre Buge         |                                          |
| Started  : 03/06/2011 17:05       |                                          |
` --------------------------------- . ----------------------------------------*/
#ifndef DOCGL_WINDOW_CALLBACK_H_
# define DOCGL_WINDOW_CALLBACK_H_

/**
** OpenGL Window message callback class.
** All callback function is called in the dispatching thread with current window OpenGL context.
*/
class OpenWGLWindowCallback
{
public:
  virtual ~OpenWGLWindowCallback() {}
  virtual void draw() {}
  virtual void keyPressed(unsigned char /* key */) {}
  virtual void resized(int /* width */ , int /* height */) {}
  virtual void mouseMove(int /* x */ , int /* y */) {}
  virtual void closed() {}
};

#endif // DOCGL_WINDOW_CALLBACK_H_
