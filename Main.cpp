/* -------------------------------- . ---------------------------------------- .
| Filename : Main.cpp               | DOCGL3.3 sample file                     |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */

extern int bounceMain(const char* szClassName, int x, int y, int width, int height);
extern int superFormula(const char* szClassName, int x, int y, int width, int height);

#if defined(GLEW_MX)
# ifdef __MINGW32__
#  include <mingw-std-threads/mingw.thread.h>
# else //__MINGW32__
#  include <thread>
# endif //__MINGW32__
# include <chrono>

void threadedMain()
  {superFormula("OGL_THREADED_CLASS", 50, 50, 800, 600);}

#endif // GLEW_MX

int main()
{
#if defined(GLEW_MX)
  std::thread thread(threadedMain);
  std::this_thread::sleep_for(std::chrono::seconds(1));
#endif // GLEW_MX

  int ret = superFormula("OGL_CLASS", 0, 0, 800, 600);  // launch current thread window

#if defined(GLEW_MX)
  thread.join();
#endif //GLEW_MX
  return ret;
}

