cl /nologo /W4 MakeResource.cpp
MakeResource.exe Image.gif > Image.cpp

cl /nologo /W4 Animation.cpp
cl /nologo /W4 AnimationEfficient.cpp
cl /nologo /W4 BitmapBrush.cpp Image.cpp
cl /nologo /W4 CreateBitmapFromWicBitmap.cpp Image.cpp
cl /nologo /W4 CreateImageEncoder.cpp
cl /nologo /W4 DesktopDeviceContext.cpp
cl /nologo /W4 HelloWorld.cpp
cl /nologo /W4 HwndRenderTarget.cpp
cl /nologo /W4 LinearGradient.cpp
cl /nologo /W4 RadialGradient.cpp
cl /nologo /W4 WicBitmapRenderTarget.cpp
cl /nologo /W4 3DCube.cpp

del *.obj
