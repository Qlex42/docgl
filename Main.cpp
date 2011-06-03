/* -------------------------------- . ---------------------------------------- .
| Filename : Main.cpp               | DOCGL3.3 sample file                     |
| Author   : Alexandre Buge         |                                          |
| Started  : 25/01/2011 10:09       |                                          |
` --------------------------------- . --------------------------------------- */

#ifdef WIN32
# include <windows.h>
#endif // WIN32

extern int bounceMain(const char* szClassName, int x, int y, int width, int height);
extern int superFormula(const char* szClassName, int x, int y, int width, int height);

#ifdef GLEW_MX
DWORD WINAPI threadedMain(LPVOID lpThreadParameter)
  {return superFormula("OGL_THREADED_CLASS", 50, 50, 800, 600);}
#endif // GLEW_MX

int main()
{
#ifdef GLEW_MX
  HANDLE thread = CreateThread(NULL, 0, threadedMain, NULL, 0, NULL); // launch threaded window
  Sleep(1000);
#endif // GLEW_MX
  int ret = superFormula("OGL_CLASS", 0, 0, 800, 600);  // launch current thread window

#ifdef GLEW_MX
  WaitForSingleObject(thread, INFINITE);    // wait end of threaded window thread
  CloseHandle(thread);
#endif //GLEW_MX
  return ret;
}

