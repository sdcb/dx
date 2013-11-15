cl /W4 MakeResource.cpp
MakeResource.exe Image.gif > Image.cpp

cl /W4 Animation.cpp
cl /W4 AnimationEfficient.cpp
cl /W4 BitmapBrush.cpp Image.cpp
cl /W4 CreateBitmapFromWicBitmap.cpp Image.cpp
cl /W4 CreateImageEncoder.cpp
cl /W4 DesktopDeviceContext.cpp
cl /W4 HelloWorld.cpp
cl /W4 HwndRenderTarget.cpp
cl /W4 Image.cpp
cl /W4 LinearGradient.cpp
cl /W4 RadialGradient.cpp
cl /W4 WicBitmapRenderTarget.cpp

del *.obj
