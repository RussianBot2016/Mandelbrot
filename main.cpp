/*
Mandelbrot Shader
target rect is (-2.2, -1.2) to (0.8, 1.2)



*/


#define WIN32_LEAN_AND_MEAN
#define WIN32_EXTRA_LEAN
//#define TEST
//#define DEBUG

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x020A
#endif

#define CLASSNAME "glslwindow"
#define BITMAP_ID 0x4D42

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>	//for _beginthread()
#include <gl/gl.h>
#include <gl/glu.h>
#include "glext.h"
#include "wglext.h"
#include "resource.h"
#include "shaders.h"

#ifdef DEBUG
	#include <crtdbg.h>
	#pragma comment(lib, "LIBCMTD.lib")
#endif

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")



//************************** prototypes ***********************
typedef struct{
	GLubyte *data;
	GLuint width;
	GLuint height;
	GLuint channels;
//	GLuint texID;
} Image;

//double buffer software rendering struct
typedef struct{
	HDC back_dc;
	HBITMAP back_bmp;
	HBITMAP old_bmp;
} scrBuffer;

enum renderer {ps3=0, ps2, fbo, cpu};

void LoadBMP(Image *img, const char *filename);
void LoadBMPResource(Image *img, WORD id);
bool CreateTexture(GLenum target, GLuint &texId, bool mipmap, GLenum filter, GLenum wrap, Image *img);
void Render();
void KillGLWindow();
void SizeGLScreen(int width,int height);
int InitGL();
bool CheckExtension(const char *search);
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
int MakeGLWindow(const char *name, int width, int height, int depth, bool fs);
void Calcfps();
void Init();
void Cleanup();
unsigned char *ReadShaderFile(const char *fileName);
int InitShaders();
int initFBO();
GLuint CreateBitmapFont(const char *fontName, int fontSize);
void glDrawText(int x, int y, char *str);
void DrawInfo();
void readInput();
void bechmark();
void resetView();
void renderSoftware();
void renderLine(LPVOID);
int round(double n);
int setMode(renderer r);
void writeLog();
int InitSR();
void DeleteSR();
void saveBuffer(const char* fileName, void* data, int length, int size);


//************************* global vars ************************

HINSTANCE g_hinstance;
HWND g_hwnd;
HDC g_hdc;
HGLRC g_hrc;
RECT g_rect;
DEVMODE saved;
POINT mouse, oldmouse;
HANDLE *hThreads;
SYSTEM_INFO sysInfo;
Image palette;
Image imgBuffer;
scrBuffer screen;
BITMAPINFO g_bitmapInfo = {0};

//config values
int win_width=1024;
int win_height=768;
int color_mode=32;
bool fullscreen=true;
bool useGL=true;
int useShaders = 1;
int singlePass = 1;
int showInfo = 1;
int dynamicBranch = 1;
int supportFBO=0;
int supportMT=0;
bool useMT=true;
bool showHelp=false;
int renderMode=4;    //later set 1..4 depending on rendering mode



//app variables
bool active;
float frameinterval;
char strfps[10];
char result[128];
char iterations[16];
char debug[32];
char status[64];
char *extensions;
int numThreads = 1;
int maxThreads = 1;
int lbutton = 0;
int benching=0;
double zDelta = 0;
float benchResults[4] = {0};
int benchFrames=0;
float benchTime=0;
char *helpShort = "F1 for help";
char *helpLong[] = {
	{"Esc: quit"},{"1-4: render modes"},{"*, /: iterations"},
	{"+, -, mouse wheel: zoom"},{"Arrows, left mouse: scroll"},
	{"Space, middle mouse: reset view"},{"pgup/pgdn: number threads"},
	{"Tab: toggle info"},{"Enter: benchmark"}
};


//mandelbrot set values
double g_scale=0;
double g_cntrx=0;
double g_cntry=0;
int g_iterations=0;

//GL variables
GLuint quadlist, fliplist;
GLuint fontlist;
GLuint palette_tex = 0;
GLhandleARB g_programObj, g_programObj2, g_programObj3, g_programObj4, g_programObj5;
GLhandleARB g_vertexShader;
GLhandleARB g_fragmentShader, g_fragmentShader2, g_fragmentShader3, g_fragmentShader4, g_fragmentShader5;
GLuint fb;
GLuint pingpongTexIDs[2];
static const GLenum source[] = { GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT };
int writeTex = 0;
int readTex = 1;
int shaderVersion = 0;








// GL function pointers
PFNGLWINDOWPOS2IARBPROC glWindowPos2iARB = NULL;
PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
// shader pointers
PFNGLCREATEPROGRAMOBJECTARBPROC  glCreateProgramObjectARB  = NULL;
PFNGLDELETEOBJECTARBPROC         glDeleteObjectARB         = NULL;
PFNGLUSEPROGRAMOBJECTARBPROC     glUseProgramObjectARB     = NULL;
PFNGLCREATESHADEROBJECTARBPROC   glCreateShaderObjectARB   = NULL;
PFNGLSHADERSOURCEARBPROC         glShaderSourceARB         = NULL;
PFNGLCOMPILESHADERARBPROC        glCompileShaderARB        = NULL;
PFNGLGETOBJECTPARAMETERIVARBPROC glGetObjectParameterivARB = NULL;
PFNGLATTACHOBJECTARBPROC         glAttachObjectARB         = NULL;
PFNGLGETINFOLOGARBPROC           glGetInfoLogARB           = NULL;
PFNGLLINKPROGRAMARBPROC          glLinkProgramARB          = NULL;
PFNGLGETUNIFORMLOCATIONARBPROC   glGetUniformLocationARB   = NULL;
PFNGLUNIFORM4FARBPROC            glUniform4fARB            = NULL;
PFNGLUNIFORM3FARBPROC            glUniform3fARB            = NULL;
PFNGLUNIFORM2FARBPROC            glUniform2fARB            = NULL;
PFNGLUNIFORM1FARBPROC            glUniform1fARB            = NULL;
PFNGLUNIFORM1IARBPROC            glUniform1iARB            = NULL;
PFNGLUNIFORM1FVARBPROC           glUniform1fvARB           = NULL;


//FBO pointers
PFNGLISRENDERBUFFEREXTPROC			glIsRenderbufferEXT = NULL;
PFNGLBINDRENDERBUFFEREXTPROC			glBindRenderbufferEXT = NULL;
PFNGLDELETERENDERBUFFERSEXTPROC		glDeleteRenderbuffersEXT = NULL;
PFNGLGENRENDERBUFFERSEXTPROC			glGenRenderbuffersEXT = NULL;
PFNGLRENDERBUFFERSTORAGEEXTPROC		glRenderbufferStorageEXT = NULL;
PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT = NULL;
PFNGLISFRAMEBUFFEREXTPROC			glIsFramebufferEXT = NULL;
PFNGLBINDFRAMEBUFFEREXTPROC			glBindFramebufferEXT = NULL;
PFNGLDELETEFRAMEBUFFERSEXTPROC		glDeleteFramebuffersEXT = NULL;
PFNGLGENFRAMEBUFFERSEXTPROC			glGenFramebuffersEXT = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC	glCheckFramebufferStatusEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE1DEXTPROC		glFramebufferTexture1DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE2DEXTPROC		glFramebufferTexture2DEXT = NULL;
PFNGLFRAMEBUFFERTEXTURE3DEXTPROC		glFramebufferTexture3DEXT = NULL;
PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC	glFramebufferRenderbufferEXT = NULL;
PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT = NULL;
PFNGLGENERATEMIPMAPEXTPROC			glGenerateMipmapEXT = NULL;

//multi-tex pointers
PFNGLMULTITEXCOORD2FARBPROC		glMultiTexCoord2fARB = NULL;
PFNGLACTIVETEXTUREARBPROC		glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC glClientActiveTextureARB = NULL;


//************************ windows functions ***********************

void Init(){
	SetTextColor(g_hdc, RGB(102,153,255));
	SetBkColor(g_hdc, RGB(0,0,0));
	GetSystemInfo(&sysInfo);
	if(useMT){
		maxThreads=(int)sysInfo.dwNumberOfProcessors;
		if(maxThreads < 1){
			maxThreads=1;
		}
		numThreads=maxThreads;
		TextOut(g_hdc, 0, 0, "detected cpu",12);
		
	}
	else{
		TextOut(g_hdc, 0, 0, "multithreading disabled",12);
	}
	//LoadBMP(&palette, "palette.bmp");
	LoadBMPResource(&palette, IDB_BITMAP1);
	TextOut(g_hdc, 0, 16, "loaded palette",14);
	imgBuffer.width=win_width;
	imgBuffer.height=win_height;
	imgBuffer.channels=3;
	imgBuffer.data=(GLubyte*)malloc(imgBuffer.width*imgBuffer.height
		*imgBuffer.channels*sizeof(GLubyte));
	TextOut(g_hdc, 0, 32, "created imgBuffer",17);

	hThreads=(HANDLE*)malloc((numThreads)*sizeof(HANDLE));
	if(useGL){
		fontlist=CreateBitmapFont("Arial", 16);
		if(!fontlist){
			MessageBox(NULL, "Error creating bitmap font", "Alert", MB_OK);
		}
		quadlist=glGenLists(1);
		fliplist=glGenLists(1);
		glNewList(quadlist,GL_COMPILE);
			glBegin(GL_QUADS);
			glTexCoord2f(0, 0);
			glVertex2f(-1, -1);
			glTexCoord2f(0, 1);
			glVertex2f(-1, 1);
			glTexCoord2f(1, 1);
			glVertex2f(1, 1);
			glTexCoord2f(1, 0);
			glVertex2f(1, -1);
			glEnd();
		glEndList();
		TextOut(g_hdc, 0, 64, "compiled lists",14);

		if(!CreateTexture(GL_TEXTURE_1D, palette_tex, false, GL_NEAREST, GL_CLAMP_TO_EDGE, &palette)){
			MessageBox(NULL,"Error creating palette texture", "Alert", MB_OK);
			PostQuitMessage(0);
		}
		TextOut(g_hdc, 0, 80, "created pal. texture",20);

		if(InitShaders()){
			TextOut(g_hdc, 0, 96, "created shaders",15);
			if(!initFBO()){
				supportFBO=0;
				TextOut(g_hdc, 0, 112, "no FBO support",14);
			}
			else{
				supportFBO=1;
				TextOut(g_hdc, 0, 112, "created FBO",11);
			}
			useShaders=1;
			singlePass=1;
			if(shaderVersion==3 && dynamicBranch==1){
				TextOut(g_hdc, 0, 128, "PS 3 supported",14);
				sprintf_s(status, "PS 3, single pass, dynamic branching");
				renderMode=1;
			}
			else{
				TextOut(g_hdc, 0, 128, "no PS 3 support",15);
				sprintf_s(status, "PS 2, single pass, no dynamic branching");
				renderMode=2;
			}
		}
		else{
			TextOut(g_hdc, 0, 96, "no shader support",17);
			useShaders=0;
			shaderVersion=0;
			sprintf_s(status, "CPU processing, %d threads", numThreads);
		}

		wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
		if(wglSwapIntervalEXT){
			wglSwapIntervalEXT(0);
		}
	}
	else{
		useShaders=0;
		shaderVersion=0;
		sprintf_s(status, "CPU processing, %d threads", numThreads);
	}

	resetView();

}

void Render(){	
	if(!useGL){
		//calculate and render using software
		FillRect(screen.back_dc,&g_rect,(HBRUSH)GetStockObject(BLACK_BRUSH));
		
		renderSoftware();
		
		SetDIBits(screen.back_dc, screen.back_bmp, 0, imgBuffer.height, (void*)imgBuffer.data, &g_bitmapInfo, DIB_RGB_COLORS);
		screen.old_bmp=(HBITMAP)SelectObject(screen.back_dc, screen.back_bmp);
		if(showInfo) 
			DrawInfo();
		
		BitBlt(g_hdc, g_rect.left, g_rect.top, g_rect.right, g_rect.bottom, screen.back_dc, 0, 0, SRCCOPY);
		//SelectObject(screen.back_dc,screen.old_bmp);
		


	}
	else{
		//calculate and/or draw using hardware
		glClear(GL_COLOR_BUFFER_BIT/* | GL_DEPTH_BUFFER_BIT*/);
		glLoadIdentity();

		if(useShaders){
			//singlepass rendering (PS2 and PS3)
			if(singlePass){
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glBindTexture(GL_TEXTURE_1D, palette_tex);
				glEnable(GL_TEXTURE_1D);
				//PS3.0 path
				if(dynamicBranch==1){
					glUseProgramObjectARB(g_programObj);
					glUniform2fARB(glGetUniformLocationARB(g_programObj, "center"), (float)g_cntrx, (float)g_cntry);
					glUniform1fARB(glGetUniformLocationARB(g_programObj, "scale"), (float)g_scale);
					glUniform1iARB(glGetUniformLocationARB(g_programObj, "palette"), 0);
					glUniform1fARB(glGetUniformLocationARB(g_programObj, "maxIterations"), (float)g_iterations);
					glUniform3fARB(glGetUniformLocationARB(g_programObj, "innerColor"), 0, 0, 0);
				}
				//PS2.0 path
				else{
					glUseProgramObjectARB(g_programObj2);
					glUniform2fARB(glGetUniformLocationARB(g_programObj2, "center"), (float)g_cntrx, (float)g_cntry);
					glUniform1fARB(glGetUniformLocationARB(g_programObj2, "scale"), (float)g_scale);
					glUniform1iARB(glGetUniformLocationARB(g_programObj2, "palette"), 0);
					glUniform1iARB(glGetUniformLocationARB(g_programObj2, "maxIterations"), g_iterations);
					glUniform3fARB(glGetUniformLocationARB(g_programObj2, "innerColor"), 0, 0, 0);
				}
				glCallList(quadlist);
				glUseProgramObjectARB(NULL);
			}
			//ping-pong multipass rendering
			else if(supportFBO && supportMT){
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glEnable(GL_TEXTURE_2D);
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, source[0], 
										GL_TEXTURE_2D, pingpongTexIDs[0], 0);
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, source[1], 
										GL_TEXTURE_2D, pingpongTexIDs[1], 0);
				glDrawBuffer(source[0]);
				glUseProgramObjectARB(g_programObj3);
				glUniform2fARB(glGetUniformLocationARB(g_programObj3, "center"), (float)g_cntrx, (float)g_cntry);
				glUniform1fARB(glGetUniformLocationARB(g_programObj3, "scale"), (float)g_scale);			
				glCallList(quadlist);
				
				int mpLoops = (int)g_iterations/5;
				int i;
				for(i=0; i<mpLoops; ++i){
					glUseProgramObjectARB(g_programObj4);
					glUniform2fARB(glGetUniformLocationARB(g_programObj4, "center"), (float)g_cntrx, (float)g_cntry);
					glUniform1fARB(glGetUniformLocationARB(g_programObj4, "scale"), (float)g_scale);
					glUniform1iARB(glGetUniformLocationARB(g_programObj4, "inputTex"), 0);
					glUniform1fARB(glGetUniformLocationARB(g_programObj4, "curLoop"), (float)i);
					if ((i % 2)==1){ //odd
						glBindTexture(GL_TEXTURE_2D, pingpongTexIDs[1]);
						glDrawBuffer(source[0]);

					}else{ //even
						glBindTexture(GL_TEXTURE_2D, pingpongTexIDs[0]);
						glDrawBuffer(source[1]);
						
					}
					glCallList(quadlist); //flipslist
				}

				glBindTexture(GL_TEXTURE_2D, pingpongTexIDs[i % 2]);

				//bind color sampler texture
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glEnable(GL_TEXTURE_1D);
				glBindTexture(GL_TEXTURE_1D, palette_tex);	
				
				glUseProgramObjectARB(g_programObj5);
				glUniform1iARB(glGetUniformLocationARB(g_programObj5, "inputTex"), 0);
				glUniform1iARB(glGetUniformLocationARB(g_programObj5, "palette"), 1);
				glUniform2fARB(glGetUniformLocationARB(g_programObj5, "center"), (float)g_cntrx, (float)g_cntry);
				glUniform1fARB(glGetUniformLocationARB(g_programObj5, "scale"), (float)g_scale);
				glUniform1fARB(glGetUniformLocationARB(g_programObj5, "maxIterations"), (float)g_iterations);
				glUniform3fARB(glGetUniformLocationARB(g_programObj5, "innerColor"), 0,0,0);
				//glUniform1fARB(glGetUniformLocationARB(g_programObj5, "step"), step);
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0); 			
				glCallList(quadlist);
				glUseProgramObjectARB(NULL);
			}		
		}
		//CPU processing
		else{
			if(supportMT){
				glActiveTextureARB(GL_TEXTURE1_ARB);
				glDisable(GL_TEXTURE_1D);
				glActiveTextureARB(GL_TEXTURE0_ARB);
				glDisable(GL_TEXTURE_2D);
				glDisable(GL_TEXTURE_1D);
			}
			
			if(glWindowPos2iARB){
				glWindowPos2iARB(0,0);	//bottom-left corner
			}
			else{
				glRasterPos2f(-1.0f,1.0f);
			}
			renderSoftware();
			glDrawPixels(imgBuffer.width, imgBuffer.height, GL_RGB, GL_UNSIGNED_BYTE, imgBuffer.data);
		}

		if(showInfo) 
			DrawInfo();
		glFinish();
		SwapBuffers(g_hdc);
	}
}


void Cleanup(){
	for(int i=0; i<numThreads-1; ++i){
			CloseHandle(hThreads[i]);
	}
	free(hThreads);
	if(palette.data){
		free(palette.data);
	}
	if(imgBuffer.data){
		free(imgBuffer.data);
	}
	if(useGL){
		glDeleteTextures(1, &palette_tex);
		glDeleteLists(quadlist,1);
		glDeleteLists(fliplist,1);
		glDeleteLists(fontlist,96);
		if(fb!=NULL){
			glDeleteFramebuffersEXT(1, &fb);
			glDeleteTextures(1, &pingpongTexIDs[0]);
			glDeleteTextures(1, &pingpongTexIDs[1]);
		}
		if(shaderVersion > 1){
			glDeleteObjectARB(g_vertexShader);
			glDeleteObjectARB(g_fragmentShader);
			glDeleteObjectARB(g_fragmentShader2);
			glDeleteObjectARB(g_fragmentShader3);
			glDeleteObjectARB(g_fragmentShader4);
			glDeleteObjectARB(g_fragmentShader5);
			glDeleteObjectARB(g_programObj);
			glDeleteObjectARB(g_programObj2);
			glDeleteObjectARB(g_programObj3);
			glDeleteObjectARB(g_programObj4);
			glDeleteObjectARB(g_programObj5);
		}
	}
	else{
		//free software buffers
		DeleteSR();
	}
}


int InitSR(){
	g_bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	g_bitmapInfo.bmiHeader.biCompression = BI_RGB;
	g_bitmapInfo.bmiHeader.biHeight = imgBuffer.height;
	g_bitmapInfo.bmiHeader.biWidth = imgBuffer.width;	
	g_bitmapInfo.bmiHeader.biBitCount = 24;
	g_bitmapInfo.bmiHeader.biClrUsed = 0;
	g_bitmapInfo.bmiHeader.biPlanes = 1;
	screen.back_dc=CreateCompatibleDC(g_hdc);
	if(screen.back_dc == NULL)
		return 0;
	screen.back_bmp=CreateDIBSection(NULL, &g_bitmapInfo, DIB_RGB_COLORS, NULL, 0, 0);
	if(screen.back_bmp == NULL)
		return 0;
	SetTextColor(screen.back_dc, RGB(102,153,255));
	SetBkColor(screen.back_dc, RGB(0,0,0));
	return 1;
}


void DeleteSR(){
	if(screen.old_bmp){
		SelectObject(screen.back_dc,screen.old_bmp);
	}
	if(screen.back_bmp){
		DeleteObject(screen.back_bmp);
	}
	if(screen.back_dc){
		DeleteDC(screen.back_dc);
	}	
	memset(&screen,0,sizeof(scrBuffer));

}

void benchmark(){
	//SM3
	if(useGL && setMode(ps3) && benchResults[0]==0){
		if(g_scale > 0.3f){
			g_scale -= g_scale*frameinterval*0.2f;
		}
		else{
			benchResults[0]=(float)benchFrames/benchTime;
			resetView();
			benchFrames=0;
			benchTime=0;
		}
	}
	//SM2
	else if(useGL && setMode(ps2) && benchResults[1]==0){
		if(g_scale > 0.3f){
			g_scale -= g_scale*frameinterval*0.2f;
		}
		else{
			benchResults[1]=(float)benchFrames/benchTime;
			resetView();
			benchFrames=0;
			benchTime=0;
		}
	}
	//SM2-FBO
	else if(useGL && setMode(fbo) && benchResults[2]==0){
		if(g_scale > 0.3f){
			g_scale -= g_scale*frameinterval*0.2f;
		}
		else{
			benchResults[2]=(float)benchFrames/benchTime;
			resetView();
			benchFrames=0;
			benchTime=0;
		}
	}
	//CPU
	else if(benchResults[3]==0){
		if(imgBuffer.width>512){
			free(imgBuffer.data);
			imgBuffer.width=512;
			imgBuffer.height=384;
			imgBuffer.data=(GLubyte*)malloc(imgBuffer.width*imgBuffer.height
					*imgBuffer.channels*sizeof(GLubyte));

		}
		setMode(cpu);
		if(g_scale > 0.3f){
			g_scale -= g_scale*frameinterval*0.2f;
		}
		else{
			benchResults[3]=(float)benchFrames/benchTime;
			resetView();
			benchFrames=0;
			benchTime=0;
		}
	}
	else{
		benching=0;
		sprintf_s(result,"SM3=%.2f, SM2=%.2f, FBO=%.2f, CPU=%.2f", benchResults[0], benchResults[1], benchResults[2], benchResults[3]);
		resetView();
		showInfo=1;
		free(imgBuffer.data);
		imgBuffer.width=win_width;
		imgBuffer.height=win_height;
		imgBuffer.data=(GLubyte*)malloc(imgBuffer.width*imgBuffer.height
				*imgBuffer.channels*sizeof(GLubyte));
		if(shaderVersion > 1){
			setMode(ps2);
		}
		if(shaderVersion > 2){
			setMode(ps3);
		}
	}

}


void DrawInfo(){
	if(useGL){
		//glDisable(GL_DEPTH_TEST);
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_1D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glDisable(GL_TEXTURE_2D);
		glDisable(GL_TEXTURE_1D);
		glColor3f(0.4f, 0.6f, 1.0f);
		if(!showHelp){
			glDrawText(2,2,(char *)glGetString(GL_RENDERER));
			glDrawText(2,18,iterations);
			glDrawText(2,34,strfps);
			glDrawText(2,50,status);
			glDrawText(2,66,result);
			glDrawText(2,82,helpShort);
			//glDrawText(2,98,debug);
			glColor3f(0.0f, 0.0f, 0.0f);
		}
		else{
			glDrawText(2,2,helpLong[0]);
			glDrawText(2,18,helpLong[1]);
			glDrawText(2,34,helpLong[2]);
			glDrawText(2,50,helpLong[3]);
			glDrawText(2,66,helpLong[4]);
			glDrawText(2,82,helpLong[5]);
			glDrawText(2,98,helpLong[6]);
			glDrawText(2,114,helpLong[7]);
			glDrawText(2,130,helpLong[8]);
		}
	}
	else{
		if(!showHelp){
			TextOut(screen.back_dc, 0, 0, "GDI renderer", 12);
			TextOut(screen.back_dc, 0, 16, strfps, strlen(strfps));
			TextOut(screen.back_dc, 0, 32, iterations, strlen(iterations));
			TextOut(screen.back_dc, 0, 48, status, strlen(status));
			TextOut(screen.back_dc, 0, 64, result, strlen(result));
			TextOut(screen.back_dc, 0, 80, helpShort, strlen(helpShort));
		}
		else{
			TextOut(screen.back_dc, 0, 0, helpLong[0], strlen(helpLong[0]));
			TextOut(screen.back_dc, 0, 16, helpLong[1], strlen(helpLong[1]));
			TextOut(screen.back_dc, 0, 32, helpLong[2], strlen(helpLong[2]));
			TextOut(screen.back_dc, 0, 48, helpLong[3], strlen(helpLong[3]));
			TextOut(screen.back_dc, 0, 64, helpLong[4], strlen(helpLong[4]));
			TextOut(screen.back_dc, 0, 80, helpLong[5], strlen(helpLong[5]));
			TextOut(screen.back_dc, 0, 96, helpLong[6], strlen(helpLong[6]));
			TextOut(screen.back_dc, 0, 112, helpLong[7], strlen(helpLong[7]));
			TextOut(screen.back_dc, 0, 128, helpLong[8], strlen(helpLong[7]));
		}
	}
}

void resetView(){
	g_scale=2.4;
	g_cntrx=0.7;
	g_cntry=0.0;
	g_iterations=255;
	sprintf_s(iterations, "iterations: %d", (int)g_iterations);
}


//set rendering mode, as defined by r, return 1 if successful, 0 if mode is not supported
int setMode(renderer r){
	if(r == ps3){
		if(shaderVersion < 3){
			sprintf_s(status, "Hardware does not support PS 3");
			return 0;
		}
		useShaders=1;
		singlePass=1;
		dynamicBranch=1;
		renderMode=1;
		sprintf_s(status, "PS 3, single pass, dynamic branching");
		return 1;

	}
	if(r == ps2){
		if(shaderVersion < 2){
			sprintf_s(status, "Hardware does not support PS 2");
			return 0;
		}
		useShaders=1;
		singlePass=1;
		dynamicBranch=0;
		renderMode=2;
		sprintf_s(status, "PS 2, single pass, no dynamic branching");
		return 1;

	}
	if(r == fbo){
		if(shaderVersion < 2){
			sprintf_s(status, "Hardware does not support PS 2");
			return 0;
		}
		if(!supportFBO){
			sprintf_s(status, "Hardware does not support FBO");
			return 0;
		}
		if(!supportMT){
			sprintf_s(status, "Hardware does not support multitexturing");
			return 0;
		}
		useShaders=1;
		singlePass=0;
		dynamicBranch=0;
		renderMode=3;
		sprintf_s(status, "PS 2, multipass, FBO");
		return 1;
	}
	if(r == cpu){
		useShaders=0;
		singlePass=0;
		dynamicBranch=0;
		renderMode=4;
		sprintf_s(status, "CPU processing, %d threads", numThreads);
		return 1;
	}
	return 0;
}


int InitGL(){
	//if(!useGL) return 0;
	GLuint PixelFormat;
	static PIXELFORMATDESCRIPTOR pfd={
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW |
		PFD_SUPPORT_OPENGL |
		PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		color_mode,				//color mode
		0,0,0,0,0,0,			//color bits
		0,						//alpha buffer
		0,						//shift bit
		0,						//accumulation buffer
		0,0,0,0,				//accumulation bits
		24,						//depth buffer bits
		0,						//stencil buffer bits
		0,						//aux buffer
		PFD_MAIN_PLANE,
		0,
		0,0,0
	};
	PixelFormat=ChoosePixelFormat(g_hdc,&pfd);
	SetPixelFormat(g_hdc,PixelFormat,&pfd);
	if(!PixelFormat){
		return 0;
	}
	g_hrc=wglCreateContext(g_hdc);
	if(!g_hrc){
		return 0;
	}
	wglMakeCurrent(g_hdc, g_hrc);
	extensions=(char*)glGetString(GL_EXTENSIONS);
	char *renderer = (char *)glGetString(GL_RENDERER);
	char *version = (char *)glGetString(GL_VERSION);
	if(strstr(renderer, "GDI Generic")){
		return 0;
	}
	if(strncmp(version, "1.1", 3)==0){
		return 0;
	}

	glClearColor(0.0f,0.0f,0.0f,0.0f);
//	glShadeModel(GL_SMOOTH);
	glDisable(GL_DEPTH_TEST);
//	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_CULL_FACE);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);  //needed for driver 3d mode switch


	if(CheckExtension("GL_ARB_window_pos")){
		glWindowPos2iARB = (PFNGLWINDOWPOS2IARBPROC) wglGetProcAddress("glWindowPos2iARB");
	}
	if(CheckExtension("GL_ARB_multitexture")){
		glMultiTexCoord2fARB = (PFNGLMULTITEXCOORD2FARBPROC) wglGetProcAddress("glMultiTexCoord2fARB");
		glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");
		glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress("glClientActiveTextureARB");
		supportMT=1;
	}
	else{
		supportMT=0;
	}
	if(!CheckExtension("GL_ARB_shading_language_100") || !CheckExtension("GL_ARB_shader_objects")){
		shaderVersion=0;
	}

	return 1;

}

void SizeGLScreen(int width, int height){
	if(height==0)
		height=1;
	if(useGL){
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		win_width = width;
		win_height = height;
		glViewport(0,0,width,height);
	}
	free(imgBuffer.data);
	imgBuffer.width=win_width;
	imgBuffer.height=win_height;
	imgBuffer.data=(GLubyte*)malloc(imgBuffer.width*imgBuffer.height
			*imgBuffer.channels*sizeof(GLubyte));
	GetClientRect(g_hwnd,&g_rect);
}



int initFBO(){

	if(CheckExtension("GL_EXT_framebuffer_object")){
		glIsRenderbufferEXT = (PFNGLISRENDERBUFFEREXTPROC)wglGetProcAddress("glIsRenderbufferEXT");
		glBindRenderbufferEXT = (PFNGLBINDRENDERBUFFEREXTPROC)wglGetProcAddress("glBindRenderbufferEXT");
		glDeleteRenderbuffersEXT = (PFNGLDELETERENDERBUFFERSEXTPROC)wglGetProcAddress("glDeleteRenderbuffersEXT");
		glGenRenderbuffersEXT = (PFNGLGENRENDERBUFFERSEXTPROC)wglGetProcAddress("glGenRenderbuffersEXT");
		glRenderbufferStorageEXT = (PFNGLRENDERBUFFERSTORAGEEXTPROC)wglGetProcAddress("glRenderbufferStorageEXT");
		glGetRenderbufferParameterivEXT = (PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC)wglGetProcAddress("glGetRenderbufferParameterivEXT");
		glIsFramebufferEXT = (PFNGLISFRAMEBUFFEREXTPROC)wglGetProcAddress("glIsFramebufferEXT");
		glBindFramebufferEXT = (PFNGLBINDFRAMEBUFFEREXTPROC)wglGetProcAddress("glBindFramebufferEXT");
		glDeleteFramebuffersEXT = (PFNGLDELETEFRAMEBUFFERSEXTPROC)wglGetProcAddress("glDeleteFramebuffersEXT");
		glGenFramebuffersEXT = (PFNGLGENFRAMEBUFFERSEXTPROC)wglGetProcAddress("glGenFramebuffersEXT");
		glCheckFramebufferStatusEXT = (PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC)wglGetProcAddress("glCheckFramebufferStatusEXT");
		glFramebufferTexture1DEXT = (PFNGLFRAMEBUFFERTEXTURE1DEXTPROC)wglGetProcAddress("glFramebufferTexture1DEXT");
		glFramebufferTexture2DEXT = (PFNGLFRAMEBUFFERTEXTURE2DEXTPROC)wglGetProcAddress("glFramebufferTexture2DEXT");
		glFramebufferTexture3DEXT = (PFNGLFRAMEBUFFERTEXTURE3DEXTPROC)wglGetProcAddress("glFramebufferTexture3DEXT");
		glFramebufferRenderbufferEXT = (PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC)wglGetProcAddress("glFramebufferRenderbufferEXT");
		glGetFramebufferAttachmentParameterivEXT = (PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC)wglGetProcAddress("glGetFramebufferAttachmentParameterivEXT");
		glGenerateMipmapEXT = (PFNGLGENERATEMIPMAPEXTPROC)wglGetProcAddress("glGenerateMipmapEXT");

		if( !glIsRenderbufferEXT || !glBindRenderbufferEXT || !glDeleteRenderbuffersEXT || 
			!glGenRenderbuffersEXT || !glRenderbufferStorageEXT || !glGetRenderbufferParameterivEXT || 
			!glIsFramebufferEXT || !glBindFramebufferEXT || !glDeleteFramebuffersEXT || 
			!glGenFramebuffersEXT || !glCheckFramebufferStatusEXT || !glFramebufferTexture1DEXT || 
			!glFramebufferTexture2DEXT || !glFramebufferTexture3DEXT || !glFramebufferRenderbufferEXT||  
			!glGetFramebufferAttachmentParameterivEXT || !glGenerateMipmapEXT )
		{
			return 0;
		}
	}


	//generate FBO and 2 render textures
	glGenFramebuffersEXT(1, &fb);
	glGenTextures(1, &pingpongTexIDs[0]);
	glGenTextures(1, &pingpongTexIDs[1]);
	
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	//SizeGLScreen(win_width,win_height); //this distorts ping-pong rendering!!!

	//set up FBO with 1st texture
	glBindTexture(GL_TEXTURE_2D, pingpongTexIDs[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, win_width, win_height, 0, 
            GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, 
                          GL_TEXTURE_2D, pingpongTexIDs[0], 0);
	if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_UNSUPPORTED_EXT){
		return 0;
	}

	//set up 2nd FBO with 2nd texture
	glBindTexture(GL_TEXTURE_2D, pingpongTexIDs[1]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F_ARB, win_width, win_height, 0, 
             GL_RGBA, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, 
                          GL_TEXTURE_2D, pingpongTexIDs[1], 0);
	if(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT) == GL_FRAMEBUFFER_UNSUPPORTED_EXT){
		return 0;
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return 1;
}


//check if specifed extension is supported
bool CheckExtension(const char *search){
	unsigned int idx=0;
	char *str=extensions;
	if (str==NULL)
		return false;
	char *end=str+strlen(str);
	// loop while we haven't reached the end of the string
	while (str<end){
		// find where a space is located
		idx=strcspn(str," ");
		//found extension
		if((strlen(search)==idx) && (strncmp(search,str,idx)==0)){
			return true;
		}
		//didn't find extension, move pointer to the next string to search
		str+=(idx + 1);
	}
	return false;
}


void KillGLWindow(){
/*
	if(fullscreen){
		if(!ChangeDisplaySettings(NULL,CDS_TEST)){
			ChangeDisplaySettings(NULL,CDS_RESET);
			ChangeDisplaySettings(&saved,CDS_RESET);
		}
		else
			ChangeDisplaySettings(NULL,CDS_RESET);
	}
*/

	if(useGL && g_hrc){
		wglMakeCurrent(g_hdc, NULL);
		wglDeleteContext(g_hrc);
	}
	if(!useGL){
		DeleteSR();
	}
	if(g_hdc){
		ReleaseDC(g_hwnd,g_hdc);
	}
	if(fullscreen){
		ChangeDisplaySettings(NULL,0);
		ShowCursor(true);
	}
	DestroyWindow(g_hwnd);
	UnregisterClass(CLASSNAME, g_hinstance);
}


//load bitmap image from file
void LoadBMP(Image *img, const char *filename){
	FILE *filePtr;
	BITMAPINFOHEADER bitmapInfoHeader;
	BITMAPFILEHEADER bitmapFileHeader;
	GLuint imageIdx = 0;			// image index counter
	GLubyte tempRGB;				// swap variable
	fopen_s(&filePtr, filename, "rb");
	if (filePtr == NULL){
		MessageBox(NULL,"Texture file not found", "ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	// read the bitmap file header
	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);	
	// verify that this is a bitmap by checking for the universal bitmap id
	if (bitmapFileHeader.bfType != BITMAP_ID){
		fclose(filePtr);
		MessageBox(NULL,"Invalid texture format", "ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	// read the bitmap information header
	fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	// move file pointer to beginning of bitmap data
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);
	if(bitmapInfoHeader.biSizeImage == 0){
		bitmapInfoHeader.biSizeImage = ((((bitmapInfoHeader.biWidth * bitmapInfoHeader.biBitCount) + 31) & ~31) / 8)
						* bitmapInfoHeader.biHeight;
	}
	// allocate enough memory for the bitmap image data
	img->data=(GLubyte*)malloc(bitmapInfoHeader.biSizeImage);
	// verify memory allocation
	if (!img->data){
		free(img->data);
		fclose(filePtr);
		MessageBox(NULL,"Error allocating texture read buffer", "ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	// read in the bitmap image data
	// make sure bitmap image data was read
	if (fread(img->data, 1, bitmapInfoHeader.biSizeImage, filePtr)!=bitmapInfoHeader.biSizeImage){		
		if(img->data != NULL)
			free(img->data);
		fclose(filePtr);
		MessageBox(NULL,"Error reading texture file", "ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	if(useGL){
	// swap the R and B values to get RGB since the bitmap color format is in BGR
		for (imageIdx = 0; imageIdx < bitmapInfoHeader.biSizeImage; imageIdx+=3){
			tempRGB = img->data[imageIdx];
			img->data[imageIdx] = img->data[imageIdx + 2];
			img->data[imageIdx + 2] = tempRGB;
		}
	}
	fclose(filePtr);

	// Assign the channels, width, height to Image struct
	img->channels = 3;
	img->width = bitmapInfoHeader.biWidth;
	img->height = bitmapInfoHeader.biHeight;

}

void LoadBMPResource(Image *img, WORD id){
	HBITMAP hBmp;
	BITMAP	bmp;
	GLuint imageIdx = 0;			// image index counter
	GLubyte tempRGB;				// swap variable
	int imgSize;
	hBmp=(HBITMAP)LoadImage(GetModuleHandle(NULL),MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
	if (!hBmp){
		return;
	}
	GetObject(hBmp,sizeof(bmp), &bmp);
	imgSize=bmp.bmHeight * bmp.bmWidth * bmp.bmBitsPixel/8;
	img->data=(GLubyte*)malloc(imgSize);
	if(!img->data){
		MessageBox(NULL,"Error allocating texture read buffer", "ERROR",MB_OK|MB_ICONEXCLAMATION);
		return;
	}
	memcpy(img->data, bmp.bmBits, imgSize);
	if(useGL){
	// swap the R and B values to get RGB since the bitmap color format is in BGR
		for (imageIdx = 0; imageIdx < imgSize; imageIdx+=3){
			tempRGB = img->data[imageIdx];
			img->data[imageIdx] = img->data[imageIdx + 2];
			img->data[imageIdx + 2] = tempRGB;
		}
	}
	img->height=bmp.bmHeight;
	img->width=bmp.bmWidth;
	img->channels=bmp.bmBitsPixel/8;
	DeleteObject(hBmp);

}


//create texture from file
bool CreateTexture(GLenum target, GLuint &texID, bool mipmap, GLenum filter, GLenum wrap, Image *img){
	GLuint type;
	glGenTextures(1, &texID);
	glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(target, texID);
	if(img->channels==3)
		type=GL_RGB;
	else
		type=GL_RGBA;
	if(!mipmap){
		if(target==GL_TEXTURE_1D){
			glTexImage1D(GL_TEXTURE_1D, 0, img->channels, img->width, 0, type, GL_UNSIGNED_BYTE, img->data);
		}
		else if(target==GL_TEXTURE_2D){
			glTexImage2D(GL_TEXTURE_2D, 0, img->channels, img->width, img->height, 0, type, GL_UNSIGNED_BYTE, img->data);
		}
	}
	else if(mipmap && target==GL_TEXTURE_2D){
		gluBuild2DMipmaps(GL_TEXTURE_2D, img->channels, img->width, 
			img->height, type, GL_UNSIGNED_BYTE, img->data);
	}
	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, filter);
//	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(target, GL_TEXTURE_WRAP_S, wrap);
	glTexParameteri(target, GL_TEXTURE_WRAP_T, wrap);
	//fprintf(pfile, "CreateTexture: creation complete\n");
	return true;
}


int MakeGLWindow(const char *name, int width, int height, int depth, bool fs){
	WNDCLASS wc;
	DWORD dwStyle;
	g_hinstance=GetModuleHandle(NULL);	
	//fill out window class struct
	memset(&wc, 0, sizeof(WNDCLASS));
	wc.style=CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc=WndProc;
	wc.hInstance=g_hinstance;
	wc.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wc.hCursor=LoadCursor(NULL,IDC_ARROW);
	wc.lpszMenuName=NULL;
	wc.lpszClassName=CLASSNAME;
	//register class
	if(!RegisterClass(&wc)){
		MessageBox(NULL, "Could not register the application class.", "Error", MB_OK);
		return 0;
	}

	fullscreen=fs;
	if (fullscreen){
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &saved);
		dwStyle=WS_POPUP;
		DEVMODE dmScreenSettings;
		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);	
		dmScreenSettings.dmPelsWidth = width;
		dmScreenSettings.dmPelsHeight = height;
		dmScreenSettings.dmBitsPerPel = depth;
		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL){
			// setting display mode failed, switch to windowed
			MessageBox(NULL, "Fullscreen display mode failed", NULL, MB_OK);
			fullscreen=false;	
		}
		else{
			fullscreen=true;
			ShowCursor(false);
		}
	}
	else{
		dwStyle =WS_OVERLAPPEDWINDOW/* | WS_CAPTION | WS_SYSMENU*/;
		ShowCursor(true);
	}
	g_rect.left=(long)0;
	g_rect.right=(long)width;
	g_rect.top=(long)0;
	g_rect.bottom=(long)height;
	AdjustWindowRect(&g_rect, dwStyle, false);
	//create window
	g_hwnd=CreateWindow(CLASSNAME,name,
		dwStyle | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
		0,0,
		g_rect.right-g_rect.left,
		g_rect.bottom-g_rect.top,
		NULL,
		NULL,
		g_hinstance,
		NULL);
	if(!g_hwnd){
		MessageBox(NULL, "Could not create application window.", "Error", MB_OK);
		PostQuitMessage(0);
		return 0;
	}
	
	ShowWindow(g_hwnd,SW_SHOW);
	SetForegroundWindow(g_hwnd);
	SetFocus(g_hwnd);
	UpdateWindow(g_hwnd);
	g_hdc = GetDC(g_hwnd);
	if(!g_hdc){
		MessageBox(NULL, "Could not create device context.", "Error", MB_OK);
		return 0;
	}
	if(useGL && !InitGL()){
		MessageBox(NULL, "OpenGL could not be initialized.\nReverting to software rendering.", "Error", MB_OK);
		useGL=false;
	}
	if(!useGL && !InitSR()){

		MessageBox(NULL, "Software renderer could not be initialized.\nThe application will terminate", "Error", MB_OK);
		PostQuitMessage(0);
		return 0;

	}
	return 1;
}

void Calcfps(){
	static float fps=0.0f;
	static float lasttime=0.0f;
	static float frametime=0.0f;
	float currtime=GetTickCount()*0.001f;
	frameinterval=currtime-frametime;
	frametime=currtime;
	fps++;
	if(benching){
		++benchFrames;
		benchTime+=frameinterval;
	}
	if(currtime-lasttime > 1.0f){
		lasttime=currtime;
		sprintf_s(strfps,"fps: %d",int(fps));
		fps=0;
	}
}


unsigned char *ReadShaderFile(const char *fileName){
	unsigned char *content=0;
	int count=0;
	FILE *file=fopen(fileName,"r");
    if(file==NULL){
        MessageBox( NULL, "Cannot open shader file!", "ERROR",
            MB_OK | MB_ICONEXCLAMATION );
		return 0;
    }
	fseek(file, 0, SEEK_END);
	count=ftell(file);
	rewind(file);
	if(count>0){
		content=(unsigned char*)malloc(sizeof(char)*(count+1));
		count = fread(content,sizeof(char),count,file);
		content[count] = '\0';
	}
	fclose(file);
	return content;
}


int InitShaders(void){

    if(! CheckExtension("GL_ARB_shading_language_100")){
        return 0;
    }

    if(! CheckExtension("GL_ARB_shader_objects")){
        return 0;
    }

	/*
	this is needed because some drivers claim support for the above 2 extensions
	even though the hardware doesnt support them
	*/
	if(! CheckExtension("GL_ARB_fragment_program")){
		return 0;
	}

    glCreateProgramObjectARB  = (PFNGLCREATEPROGRAMOBJECTARBPROC)wglGetProcAddress("glCreateProgramObjectARB");
    glDeleteObjectARB         = (PFNGLDELETEOBJECTARBPROC)wglGetProcAddress("glDeleteObjectARB");
    glUseProgramObjectARB     = (PFNGLUSEPROGRAMOBJECTARBPROC)wglGetProcAddress("glUseProgramObjectARB");
    glCreateShaderObjectARB   = (PFNGLCREATESHADEROBJECTARBPROC)wglGetProcAddress("glCreateShaderObjectARB");
    glShaderSourceARB         = (PFNGLSHADERSOURCEARBPROC)wglGetProcAddress("glShaderSourceARB");
    glCompileShaderARB        = (PFNGLCOMPILESHADERARBPROC)wglGetProcAddress("glCompileShaderARB");
    glGetObjectParameterivARB = (PFNGLGETOBJECTPARAMETERIVARBPROC)wglGetProcAddress("glGetObjectParameterivARB");
    glAttachObjectARB         = (PFNGLATTACHOBJECTARBPROC)wglGetProcAddress("glAttachObjectARB");
    glGetInfoLogARB           = (PFNGLGETINFOLOGARBPROC)wglGetProcAddress("glGetInfoLogARB");
    glLinkProgramARB          = (PFNGLLINKPROGRAMARBPROC)wglGetProcAddress("glLinkProgramARB");
    glGetUniformLocationARB   = (PFNGLGETUNIFORMLOCATIONARBPROC)wglGetProcAddress("glGetUniformLocationARB");
    glUniform4fARB            = (PFNGLUNIFORM4FARBPROC)wglGetProcAddress("glUniform4fARB");
	glUniform3fARB            = (PFNGLUNIFORM3FARBPROC)wglGetProcAddress("glUniform3fARB");
	glUniform1iARB            = (PFNGLUNIFORM1IARBPROC)wglGetProcAddress("glUniform1iARB");
	glUniform2fARB            = (PFNGLUNIFORM2FARBPROC)wglGetProcAddress("glUniform2fARB");
	glUniform1fARB            = (PFNGLUNIFORM1FARBPROC)wglGetProcAddress("glUniform1fARB");
	glUniform1fvARB           = (PFNGLUNIFORM1FVARBPROC)wglGetProcAddress("glUniform1fvARB");

    if( !glCreateProgramObjectARB || !glDeleteObjectARB || !glUseProgramObjectARB ||
        !glCreateShaderObjectARB || !glCreateShaderObjectARB || !glCompileShaderARB || 
        !glGetObjectParameterivARB || !glAttachObjectARB || !glGetInfoLogARB || 
        !glLinkProgramARB || !glGetUniformLocationARB || !glUniform4fARB ||
		!glUniform1iARB || !glUniform2fARB || !glUniform1fARB || !glUniform3fARB || !glUniform1fvARB)
    {
        return 0;
    }

    const char *vertexShaderStrings[1];
    const char *fragmentShaderStrings[5];
    GLint bVertCompiled;
    GLint bFragCompiled;
    GLint bLinked;
    char str[4096];

	// Create the vertex shader...
    g_vertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);

	//unsigned char *vertexShaderAssembly = ReadShaderFile("vertex_shader.vert");
    //vertexShaderStrings[0] = (char*)vertexShaderAssembly;
	vertexShaderStrings[0] = vertexShaderAssembly;
    glShaderSourceARB(g_vertexShader, 1, vertexShaderStrings, NULL);
    glCompileShaderARB(g_vertexShader);
//    free(vertexShaderAssembly);

    glGetObjectParameterivARB(g_vertexShader, GL_OBJECT_COMPILE_STATUS_ARB, 
                               &bVertCompiled);
    if(bVertCompiled==false){
		glGetInfoLogARB(g_vertexShader, sizeof(str), NULL, str);
		MessageBox(NULL, str, "Vertex Shader 1 Compile Error", MB_OK|MB_ICONEXCLAMATION);
	}


	// Create the fragment shader...
    g_fragmentShader=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	g_fragmentShader2=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	g_fragmentShader3=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	g_fragmentShader4=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	g_fragmentShader5=glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);

    //unsigned char *fragmentShaderAssembly = ReadShaderFile("fragment_shader.frag");
    //fragmentShaderStrings[0] = (char*)fragmentShaderAssembly;
	fragmentShaderStrings[0] = fragmentShaderAssembly;
	fragmentShaderStrings[1] = fragmentShaderAssembly2;
	fragmentShaderStrings[2] = fragShaderMPInit;
	fragmentShaderStrings[3] = fragShaderMP;
	fragmentShaderStrings[4] = fragShaderMPFinish;
	glShaderSourceARB(g_fragmentShader, 1, &fragmentShaderStrings[0], NULL);
	glShaderSourceARB(g_fragmentShader2, 1, &fragmentShaderStrings[1], NULL);
	glShaderSourceARB(g_fragmentShader3, 1, &fragmentShaderStrings[2], NULL);
	glShaderSourceARB(g_fragmentShader4, 1, &fragmentShaderStrings[3], NULL);
	glShaderSourceARB(g_fragmentShader5, 1, &fragmentShaderStrings[4], NULL);

	glCompileShaderARB(g_fragmentShader);
//    free(fragmentShaderAssembly);
	glGetObjectParameterivARB(g_fragmentShader, GL_OBJECT_COMPILE_STATUS_ARB, 
							   &bFragCompiled);
	if(bFragCompiled==false){
		glGetInfoLogARB(g_fragmentShader, sizeof(str), NULL, str );
		MessageBox(NULL, str, "Fragment Shader 1 Compile Error", MB_OK|MB_ICONEXCLAMATION);
	}

	glCompileShaderARB(g_fragmentShader2);
	glGetObjectParameterivARB(g_fragmentShader2, GL_OBJECT_COMPILE_STATUS_ARB, 
							   &bFragCompiled);
	if(bFragCompiled==false){
		glGetInfoLogARB(g_fragmentShader2, sizeof(str), NULL, str );
		MessageBox(NULL, str, "Fragment Shader 2 Compile Error", MB_OK|MB_ICONEXCLAMATION);
	}

	glCompileShaderARB(g_fragmentShader3);
	glGetObjectParameterivARB(g_fragmentShader3, GL_OBJECT_COMPILE_STATUS_ARB, 
							   &bFragCompiled);
	if(bFragCompiled==false){
		glGetInfoLogARB(g_fragmentShader3, sizeof(str), NULL, str );
		MessageBox(NULL, str, "Fragment Shader 3 Compile Error", MB_OK|MB_ICONEXCLAMATION);
	}

	glCompileShaderARB(g_fragmentShader4);
	glGetObjectParameterivARB(g_fragmentShader4, GL_OBJECT_COMPILE_STATUS_ARB, 
							   &bFragCompiled);
	if(bFragCompiled==false){
		glGetInfoLogARB(g_fragmentShader4, sizeof(str), NULL, str );
		MessageBox(NULL, str, "Fragment Shader 4 Compile Error", MB_OK|MB_ICONEXCLAMATION);
	}

	glCompileShaderARB(g_fragmentShader5);
	glGetObjectParameterivARB(g_fragmentShader5, GL_OBJECT_COMPILE_STATUS_ARB, 
							   &bFragCompiled);
	if(bFragCompiled==false){
		glGetInfoLogARB(g_fragmentShader5, sizeof(str), NULL, str );
		MessageBox(NULL, str, "Fragment Shader 5 Compile Error", MB_OK|MB_ICONEXCLAMATION);
	}


    // Create a program object and attach the two compiled shaders...
    g_programObj=glCreateProgramObjectARB();
    glAttachObjectARB(g_programObj, g_vertexShader);
    glAttachObjectARB(g_programObj, g_fragmentShader);

    g_programObj2=glCreateProgramObjectARB();
    glAttachObjectARB(g_programObj2, g_vertexShader);
    glAttachObjectARB(g_programObj2, g_fragmentShader2);

    g_programObj3=glCreateProgramObjectARB();
    glAttachObjectARB(g_programObj3, g_vertexShader);
    glAttachObjectARB(g_programObj3, g_fragmentShader3);

    g_programObj4=glCreateProgramObjectARB();
    glAttachObjectARB(g_programObj4, g_vertexShader);
    glAttachObjectARB(g_programObj4, g_fragmentShader4);

    g_programObj5=glCreateProgramObjectARB();
    glAttachObjectARB(g_programObj5, g_vertexShader);
    glAttachObjectARB(g_programObj5, g_fragmentShader5);


    // Link the program object and print out the info log...
    glLinkProgramARB(g_programObj);
    glGetObjectParameterivARB(g_programObj, GL_OBJECT_LINK_STATUS_ARB, &bLinked);
	glGetInfoLogARB(g_programObj, sizeof(str), NULL, str);
    if(bLinked==false){	
		MessageBox(NULL, str, "Linking Error - program 1", MB_OK|MB_ICONEXCLAMATION);
	}
	if(strstr(str,"software")){
		shaderVersion=2;
		dynamicBranch=0;
	}
	else{
		shaderVersion=3;
		dynamicBranch=1;
	}

	glLinkProgramARB(g_programObj2);
    glGetObjectParameterivARB(g_programObj2, GL_OBJECT_LINK_STATUS_ARB, &bLinked);

    if(bLinked==false){
		glGetInfoLogARB(g_programObj2, sizeof(str), NULL, str);
		MessageBox(NULL, str, "Linking Error - program 2", MB_OK|MB_ICONEXCLAMATION);
	}

	glLinkProgramARB(g_programObj3);
    glGetObjectParameterivARB(g_programObj3, GL_OBJECT_LINK_STATUS_ARB, &bLinked);

    if(bLinked==false){
		glGetInfoLogARB(g_programObj3, sizeof(str), NULL, str);
		MessageBox(NULL, str, "Linking Error - program 3", MB_OK|MB_ICONEXCLAMATION);
	}

	glLinkProgramARB(g_programObj4);
    glGetObjectParameterivARB(g_programObj4, GL_OBJECT_LINK_STATUS_ARB, &bLinked);

    if(bLinked==false){
		glGetInfoLogARB(g_programObj4, sizeof(str), NULL, str);
		MessageBox(NULL, str, "Linking Error - program 4", MB_OK|MB_ICONEXCLAMATION);
	}

	glLinkProgramARB(g_programObj5);
    glGetObjectParameterivARB(g_programObj5, GL_OBJECT_LINK_STATUS_ARB, &bLinked);

    if(bLinked==false){
		glGetInfoLogARB(g_programObj5, sizeof(str), NULL, str);
		MessageBox(NULL, str, "Linking Error - program 5", MB_OK|MB_ICONEXCLAMATION);
	}
	return 1;
}


GLuint CreateBitmapFont(const char *fontName, int fontSize){
	HFONT hFont;				// windows font
	GLuint base;
	base = glGenLists(96);      // create storage for 96 characters
	if (stricmp(fontName, "symbol") == 0){
	     hFont = CreateFont(fontSize, 0, 0, 0, FALSE, FALSE, FALSE, FALSE, SYMBOL_CHARSET, 
							OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
							FF_DONTCARE | DEFAULT_PITCH, fontName);
	}
	else{
		 hFont = CreateFont(fontSize, 0, 0, 0, FALSE, FALSE, FALSE, FALSE, ANSI_CHARSET, 
							OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
							FF_DONTCARE | DEFAULT_PITCH, fontName);
	}
	if (!hFont)
		return 0;
	SelectObject(g_hdc, hFont);
	wglUseFontBitmaps(g_hdc, 32, 96, base);
	return base;
}

void glDrawText(int x, int y, char *str){
	if ((fontlist == 0) || (fontlist == NULL))
		return;
	glPushAttrib(GL_LIST_BIT);
	glListBase(fontlist - 32);
	if(glWindowPos2iARB){
		glWindowPos2iARB(x,win_height-y-10);	//make y=0 the top
	}
	else{
		glRasterPos2f(2*x/(float)win_width-1.0f,(2*y-(float)win_height+21)/(float)win_height);
	}
	glCallLists(strlen(str), GL_UNSIGNED_BYTE, str);
	glPopAttrib();
}


void readInput(){
	static float speed = g_scale*frameinterval*0.5f;
	if(lbutton){
		GetCursorPos(&mouse);
		double dx=(double)(oldmouse.x - mouse.x);
		double dy=(double)(oldmouse.y - mouse.y);
		if(dx != 0 || dy != 0){
			oldmouse.x=mouse.x;
			oldmouse.y=mouse.y;
			g_cntrx +=dx*g_scale*0.002;
			g_cntry +=dy*g_scale*0.002;
			return;
		}		
	}
	//zoom
	if(GetAsyncKeyState(VK_ADD) & 0x8000)
		g_scale -=speed;//g_scale*frameinterval;
	if(GetAsyncKeyState(VK_SUBTRACT) & 0x8000)
		g_scale +=speed;//g_scale*frameinterval;

	speed=g_scale*frameinterval*0.5f;
	//vertical scroll
	if(GetAsyncKeyState(VK_UP) & 0x8000)
		g_cntry +=speed;
	if(GetAsyncKeyState(VK_DOWN) & 0x8000)
		g_cntry -=speed;
	//prevent opposite directions glitch
	if(GetAsyncKeyState(VK_LEFT) & 0x8000 && GetAsyncKeyState(VK_RIGHT) & 0x8000 )
		return;
	//horizontal scroll
	if(GetAsyncKeyState(VK_LEFT) & 0x8000)
		g_cntrx +=speed;
	if(GetAsyncKeyState(VK_RIGHT) & 0x8000)
		g_cntrx -=speed;

}



//draw the image every Nth line, N=number of threads
//param specifies line to render [0-image height]
void renderLine(void *param){
	for(GLuint y=(unsigned long)param; y<imgBuffer.height; y+=numThreads){
		GLubyte *dest = imgBuffer.data + (y*imgBuffer.width*imgBuffer.channels);
		double cy = ((double)y / (double)imgBuffer.height - 0.5) * g_scale + g_cntry;
		double cy2 = ((double)y / (double)imgBuffer.height - 0.5) * g_scale + g_cntry;
		for(GLuint x=0; x<imgBuffer.width-1;x+=2){
			double cx = (1.3333333333 * ((double)x / imgBuffer.width - 0.5) * g_scale) - g_cntrx;
			double cx2 = (1.3333333333 * ((double)(x+1) / imgBuffer.width - 0.5) * g_scale) - g_cntrx;
			double zx = 0;
			double zy = 0;
			double zx2 = 0;
			double zy2 = 0;
			int d1=0;
			int d2=0;
			int pixRemain=2;
			for(int i = -2;i < g_iterations;++i){	//start with -2 to better match GPU colors
				double a=zx*zx;
				double b=zy*zy;
				double c=zx*zy;
				c=c+c;
				zx = a - b + cx;
				zy = c + cy;
				if(a+b>4.0){
					d1=i;
					zx=zy=0;
					--pixRemain;
				}

				double a2=zx2*zx2;
				double b2=zy2*zy2;
				double c2=zx2*zy2;
				c2=c2+c2;
				zx2 = a2 - b2 + cx2;
				zy2 = c2 + cy2;
				if(a2+b2>4.0){
					d2=i;
					zx2=zy2=0;
					--pixRemain;
				}

				if(pixRemain==0) break;

			}

			if(d1==g_iterations){
				*dest++ = 0;
				*dest++ = 0;
				*dest++ = 0;
			}
			else{
				double offset = (double)d1/g_iterations*(palette.width);
				if(offset<0) offset=0;
				if(offset>palette.width-1) offset=palette.width-1;
				*dest++ = palette.data[palette.channels * ((int)(offset))];
				*dest++ = palette.data[palette.channels * ((int)(offset)) + 1];
				*dest++ = palette.data[palette.channels * ((int)(offset)) + 2];
			}
			if(d2==g_iterations){
				*dest++ = 0;
				*dest++ = 0;
				*dest++ = 0;
			}
			else{
				double offset = (double)d2/g_iterations*(palette.width);
				if(offset<0) offset=0;
				if(offset>palette.width-1) offset=palette.width-1;
				*dest++ = palette.data[palette.channels * ((int)(offset))];
				*dest++ = palette.data[palette.channels * ((int)(offset)) + 1];
				*dest++ = palette.data[palette.channels * ((int)(offset)) + 2];
			}
		}
	}
	
	//return (DWORD)param;
}



void renderSoftware(){
	int i;
	//memset(imgBuffer.data, 0, imgBuffer.width*imgBuffer.height*imgBuffer.channels*sizeof(GLubyte));
	for(i=0;i<numThreads;++i){
		hThreads[i]=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)renderLine,(void*)i,0,NULL);
	}
	WaitForMultipleObjects(numThreads, hThreads, TRUE, INFINITE);
	for(i=0; i<numThreads; ++i){
		CloseHandle(hThreads[i]);
	}

}


int round(double n){
	double frac=0;
	modf(n,&frac);
	if(frac > 0.5){
		return (int)n+1;
	}
	return (int)n;
}




void saveBuffer(const char* fileName, void* data, int length, int size){
	FILE* fp=fopen(fileName,"w");
	fwrite(data, size, length, fp);
	fclose(fp);

}




LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam){
	PAINTSTRUCT ps;
	switch(message){
		case WM_KEYDOWN:
			switch(wparam){
				case VK_ESCAPE:
					PostQuitMessage(0);
					break;

				case VK_TAB:
					showInfo=!showInfo;
					break;

				case VK_F1:
					if(benching) break;
					//show help
					showHelp = !showHelp;
					break;

				case VK_MULTIPLY:
					if(benching) break;
					g_iterations=(g_iterations<250 ? g_iterations+5 : 255);
					sprintf_s(iterations, "iterations: %d", g_iterations);
					break;

				case VK_DIVIDE:
					if(benching) break;
					g_iterations=(g_iterations>10 ? g_iterations-5 : 5);
					sprintf_s(iterations, "iterations: %d", g_iterations);
					break;

				case '1':
					if(benching) break;
					setMode(ps3);
					break;

				case '2':
					if(benching) break;
					setMode(ps2);
					break;

				case '3':
					if(benching) break;
					setMode(fbo);
					break;

				case '4':
					if(benching) break;
					setMode(cpu);
					break;
/*
				case VK_F12:
					if(benching) break;
					saveBuffer("imgBuffer.raw", (void*)imgBuffer.data, imgBuffer.height*imgBuffer.width, imgBuffer.channels);
					break;
*/
				case VK_ADD:
					if(benching) break;
					g_scale -=g_scale*0.05;
					break;

				case VK_SUBTRACT:
					if(benching) break;
					g_scale +=g_scale*0.05;
					break;

				case VK_SPACE:
					if(benching) break;
					resetView();
					break;

				case VK_NEXT:
					if(benching) break;
					if(renderMode != 4) break;
					if(numThreads > 1){
						--numThreads;
						sprintf_s(status, "CPU processing, %d threads", numThreads);
					}
					break;

				case VK_PRIOR:
					if(benching) break;
					if(renderMode != 4) break;
					if(numThreads < maxThreads){
						++numThreads;
						sprintf_s(status, "CPU processing, %d threads", numThreads);
					}
					break;

				case VK_RETURN:
					if(benchFrames==0 && benching==0){
						benching=1;
						resetView();
					}
					break;
				}
			return 0;

		case WM_SYSCOMMAND:
			switch (wparam){
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
				return 0;
			}
			break;

		case WM_LBUTTONDOWN:
			if(benching) break;
			lbutton=1;
			GetCursorPos(&oldmouse);
			break;

		case WM_LBUTTONUP:
			if(benching) break;
			lbutton=0;
			if(fullscreen){
				oldmouse.x=win_width/2;
				oldmouse.y=win_height/2;
			}
			SetCursorPos(oldmouse.x,oldmouse.y);
			//sprintf(debug,"mouse: %d,%d",(int)oldmouse.x,(int)oldmouse.y);
			break;

		case WM_MBUTTONDOWN:
			if(benching) break;
			resetView();
			break;

		case WM_MOUSEWHEEL:
			if(benching) break;
			zDelta = (short)HIWORD(wparam);
			if(zDelta < 0){
				g_scale +=g_scale*0.05;
				//g_scaled +=g_scaled*0.05;
			}
			if(zDelta > 0){
				g_scale -=g_scale*0.05;
				//g_scaled -=g_scaled*0.05;
			}
			//sprintf(debug,"scale: %f",g_scale);
			break;

		case WM_SIZE:
			SizeGLScreen(LOWORD(lparam),HIWORD(lparam));
			return 0;
			break;

		case WM_PAINT:
			BeginPaint(g_hwnd, &ps);

			

			//SwapBuffers(g_hdc);
			EndPaint(g_hwnd, &ps);
			break;

		case WM_CLOSE:
		case WM_QUIT:
			PostQuitMessage(0);
			return 0;
	}
	return(DefWindowProc(hwnd,message,wparam,lparam));
}


int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprev, LPSTR cmdline, int show){

	#ifdef DEBUG
		int flag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); // Get current flag
		flag |= _CRTDBG_LEAK_CHECK_DF; // Turn on leak-checking bit
		_CrtSetDbgFlag(flag); // Set flag to the new value
		fullscreen=0;
		win_width=640;
		win_height=480;
	#endif

	MSG msg;
	if(strstr(cmdline, "-debug") || strstr(cmdline, "-gdi")){
		fullscreen=0;
		win_width=640;
		win_height=480;
		if(strstr(cmdline, "-gdi")){
			useGL=false;
		}
	}
	if(strstr(cmdline, "-nomt")){
		useMT=false;
	}

	if(!MakeGLWindow("",win_width,win_height,color_mode,fullscreen)){
		return 0;
	}
	Init();
	int done=0;
	while(!done){
		if(PeekMessage(&msg,NULL,NULL,NULL,PM_REMOVE)){
			if(msg.message==WM_QUIT){
				done=1;
			}
			//else{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			//}
		}
		else{
			//if(g_hwnd==GetFocus()){
				Render();
				Calcfps();
				if(!benching){
					readInput();
				}
				else{
					benchmark();
				}
			//}
			//else{
				//WaitMessage();
			//}
		}
	}
	Cleanup();
	KillGLWindow();

	//fclose(pfile);

	return msg.wParam;
}