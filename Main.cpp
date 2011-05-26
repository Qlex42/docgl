
#include <windows.h>

extern int bounceMain(LPCTSTR szClassName);
extern int superFormula(LPCTSTR szClassName, CONST RECT& clientRect);


DWORD WINAPI threadedMain(LPVOID lpThreadParameter)
{
  CONST RECT clientRect = {1920, 0, 1920 + 800, 600};
  return superFormula(TEXT("OGL_THREADED_CLASS"), clientRect);
}

int main()
{
  CONST RECT clientRect = {0, 0, 800, 600};
#ifdef GLEW_MX
  HANDLE thread = CreateThread(NULL, 0, threadedMain, NULL, 0, NULL); // launch threaded window
#endif // GLEW_MX
  Sleep(1000);
  int ret = superFormula(TEXT("OGL_CLASS"), clientRect);  // launch current thread window

#ifdef GLEW_MX
  WaitForSingleObject(thread, INFINITE);    // wait end of threaded window thread
  CloseHandle(thread);
#endif //GLEW_MX
  return ret;
}

