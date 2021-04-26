#include<windows.h>
#include<stdio.h>
#include<gl\glew.h>
#include<gl/GL.h>
#include<string.h>
#include <stdlib.h>
#include"vmath.h"
#include"dds_types.h"
#include"libdds.h"
#include"libdds_opengl.h"


#define WIN_WIDTH	800
#define WIN_HEIGHT	600


#pragma comment(lib,"user32.lib")
#pragma comment(lib,"kernel32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"glew32.lib")
#pragma comment(lib,"opengl32.lib")

enum{
	SSK_ATTRIBUTE_VERTEX=0,
	SSK_ATTRIBUTE_COLOR,
	SSK_ATTRIBUTE_NORMAL,
	SSK_ATTRIBUTE_TEXTURE0,
};


FILE *gpFile=NULL;
FILE *gpddsFile=NULL;
HWND ghwnd=NULL;
HDC ghdc=NULL;
HGLRC ghrc=NULL;
DWORD dwStyle;
WINDOWPLACEMENT wpPrev={sizeof(WINDOWPLACEMENT)};

bool gbActiveWindow=false;
bool gbEscapeKeyIsPressed=false;
bool gbFullscreen=false;

GLuint gVertexShaderObject;
GLuint gFragmentShaderObject;
GLuint gShaderProgramObject;

GLuint gMVPUniform;
GLuint gVao_square;
GLuint gVbo_square_position;
GLuint gVbo_square_texture;
GLuint gTexture_sampler_uniform;

vmath::mat4 gPerspectiveProjectionMatrix;
LRESULT CALLBACK WndProc(HWND,UINT,WPARAM,LPARAM);

//const char* imagename="fungus.dds";
const char* imagename="water.dds";
//const char* imagename="desertcube1024.dds";
//const char* imagename="sunsetcube1024.dds";
//const char* imagename="desertcube1024.dds";
static DDS_GL_TextureInfo texture;

/*	functions declaration ___START___ */
dds_uint dds_load (const char* filename, DDSTextureInfo* texture);
void dds_free (DDSTextureInfo* texinfo);
const char* dds_error_string (int err);
void ddsGL_free (DDS_GL_TextureInfo* texture);
dds_uint ddsGL_load (const char* filename, DDS_GL_TextureInfo* texture);
/*	functions declaration ___END___ */




int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpszCmdLine,int iCmdShow){
	void initialize(void);
	void display(void);
	void uninitialize(void);

	WNDCLASSEX wndclassex;
	HWND hwnd;
	MSG msg;
	TCHAR szClassName[]=TEXT("07- DDS Texture Window");
	bool bDone=false;

	if(fopen_s(&gpddsFile,"SSK_DDS_Log File.txt","w")!=0){
		MessageBox(NULL,TEXT("Log File cannot be created\n..Exiting Now..."),TEXT("Error"),MB_OK| MB_TOPMOST | MB_ICONSTOP);
		exit(EXIT_FAILURE);
	}else{
		fprintf(gpddsFile,"Log File of DDS is Successfully Opened\n");
	}



	if(fopen_s(&gpFile,"SSK_Log File.txt","w")!=0){
		MessageBox(NULL,TEXT("Log File cannot be created\n..Exiting Now..."),TEXT("Error"),MB_OK| MB_TOPMOST | MB_ICONSTOP);
		exit(EXIT_FAILURE);
	}else{
		fprintf(gpFile,"Log File is Successfully Opened\n");
	}

	wndclassex.cbSize=sizeof(WNDCLASSEX);
	wndclassex.style=CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclassex.cbClsExtra=0;
	wndclassex.cbWndExtra=0;
	wndclassex.hInstance=hInstance;
	wndclassex.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclassex.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	wndclassex.hIconSm=LoadIcon(NULL,IDI_APPLICATION);
	wndclassex.hCursor=LoadCursor(NULL,IDC_ARROW);
	wndclassex.lpfnWndProc=WndProc;
	wndclassex.lpszClassName=szClassName;
	wndclassex.lpszMenuName=NULL;

	RegisterClassEx(&wndclassex);

	hwnd=CreateWindow(szClassName,szClassName,WS_OVERLAPPEDWINDOW,
						100,100,WIN_WIDTH,WIN_HEIGHT,
						NULL,NULL,hInstance,NULL);
	ghwnd=hwnd;

	ShowWindow(hwnd,iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	initialize();

	while(bDone==false){
		if(PeekMessage(&msg,NULL,0,0,PM_REMOVE)){
			if(msg.message==WM_QUIT){
				bDone=true;
			}else{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}else{
			if(gbActiveWindow==true){
				if(gbEscapeKeyIsPressed==true){
					bDone=true;
				}
			}
			display();
		}
	}
	uninitialize();
	return((int)msg.wParam);
}

LRESULT CALLBACK WndProc(HWND hwnd,UINT iMsg,WPARAM wParam,LPARAM lParam){
	void resize(int,int);
	void uninitialize(void);
	void ToggleFullscreen(void);

	switch(iMsg){
		case WM_ACTIVATE:
			if(HIWORD(wParam)==0){
				gbActiveWindow=true;
			}else{
				gbActiveWindow=false;
			}
			break;
		case WM_SIZE:
			resize(LOWORD(lParam),HIWORD(lParam));
			break;
		case WM_KEYDOWN:
			switch(wParam){
				case VK_ESCAPE:
					if(gbEscapeKeyIsPressed==false){
						gbEscapeKeyIsPressed=true;
					}
					break;
				case 0x46:
					if(gbFullscreen==false){
						ToggleFullscreen();
						gbFullscreen=true;
					}else{
						ToggleFullscreen();
						gbFullscreen=false;
					}
					break;
				default:
					break;
			}
			break;
		case WM_CLOSE:
			uninitialize();
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
		default:
			break;
	}
	return (DefWindowProc(hwnd,iMsg,wParam,lParam));
}

void ToggleFullscreen(void){
	MONITORINFO mi;
	bool bGetWindowPlacement=false;
	bool bGetMonitorInfo=false;
	if(gbFullscreen==false){
		dwStyle=GetWindowLong(ghwnd,GWL_STYLE);
		if(dwStyle & WS_OVERLAPPEDWINDOW){
			mi={sizeof(MONITORINFO)};
			bGetWindowPlacement=GetWindowPlacement(ghwnd,&wpPrev);
			bGetMonitorInfo=GetMonitorInfo(MonitorFromWindow(ghwnd,MONITORINFOF_PRIMARY),&mi);
			if(bGetWindowPlacement && bGetMonitorInfo){
				SetWindowLong(ghwnd,GWL_STYLE,dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(ghwnd,HWND_TOP,mi.rcMonitor.left,mi.rcMonitor.top,
							mi.rcMonitor.right-mi.rcMonitor.left,
							mi.rcMonitor.bottom-mi.rcMonitor.top,
							SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}
		ShowCursor(FALSE);
	}else{
		SetWindowLong(ghwnd,GWL_STYLE,dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd,&wpPrev);
		SetWindowPos(ghwnd,HWND_TOP,0,0,0,0,
					SWP_NOMOVE | SWP_NOSIZE |
					SWP_NOOWNERZORDER | SWP_NOZORDER |
					SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}
}

void initialize(void){
	PIXELFORMATDESCRIPTOR pfd;
	int iPixelFormatIndex;


	//int LoadGLTextures(GLuint *,TCHAR[]);
	void uninitialize(void);
	void resize(int,int);
	
	ZeroMemory(&pfd,sizeof(PIXELFORMATDESCRIPTOR));

	pfd.nSize=sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion=1;
	pfd.dwFlags=PFD_DRAW_TO_WINDOW | 
				PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL ;
	pfd.iPixelType=PFD_TYPE_RGBA;
	pfd.cColorBits=32;
	pfd.cBlueBits=8;
	pfd.cRedBits=8;
	pfd.cGreenBits=8;
	pfd.cAlphaBits=8;
	pfd.cDepthBits=32;

	ghdc=GetDC(ghwnd);
	iPixelFormatIndex=ChoosePixelFormat(ghdc,&pfd);
	if(iPixelFormatIndex==0){
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	if(SetPixelFormat(ghdc,iPixelFormatIndex,&pfd)==false){
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	ghrc=wglCreateContext(ghdc);
	if(ghrc==NULL){
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	if(wglMakeCurrent(ghdc,ghrc)==false){
		wglDeleteContext(ghrc);
		ghrc=NULL;
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}

	GLenum glew_error=glewInit();
	if(glew_error!=GLEW_OK){
		wglDeleteContext(ghrc);
		ghrc=NULL;
		ReleaseDC(ghwnd,ghdc);
		ghdc=NULL;
	}


	gVertexShaderObject=glCreateShader(GL_VERTEX_SHADER);
	const GLchar *vertexShaderSourceCode=
		"#version 130" \
		"\n" \
		"in vec4 vPosition;" \
		"in vec2 vTexture0_Coord;" \
		"out vec2 out_texture0_coord;" \
		"uniform mat4 u_mvp_matrix;" \
		"void main(void)" \
		"{" \
		"gl_Position=u_mvp_matrix * vPosition;" \
		"out_texture0_coord=vTexture0_Coord;" \
		"}";

	glShaderSource(gVertexShaderObject,1,(const GLchar**)
					&vertexShaderSourceCode,NULL);
	glCompileShader(gVertexShaderObject);
	GLint iInfoLogLength=0;
	GLint iShaderCompiledStatus=0;
	char *szInfoLog=NULL;
	glGetShaderiv(gVertexShaderObject,
				GL_COMPILE_STATUS,&iShaderCompiledStatus);
	if(iShaderCompiledStatus==GL_FALSE){
		glGetShaderiv(gVertexShaderObject,
						GL_INFO_LOG_LENGTH,&iInfoLogLength);
		if(iInfoLogLength>0){
			szInfoLog=(char *)malloc(iInfoLogLength);
			if(szInfoLog!=NULL){
				GLsizei written;
				glGetShaderInfoLog(gVertexShaderObject,iInfoLogLength,
									&written,szInfoLog);
				fprintf(gpFile,"Vertex Shader Compilation Log: %s\n",szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(0);
			}
		}
	}

	gFragmentShaderObject=glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar *fragmentShaderSourceCode=
		"#version 130" \
		"\n" \
		"in vec2 out_texture0_coord;" \
		"out vec4 FragColor;" \
		"uniform sampler2D u_texture0_sampler;" \
		"void main(void)" \
		"{" \
		"FragColor= texture(u_texture0_sampler,out_texture0_coord);" \
		"}";

	glShaderSource(gFragmentShaderObject,1,(const GLchar**)
					&fragmentShaderSourceCode,NULL);
	glCompileShader(gFragmentShaderObject);
	glGetShaderiv(gFragmentShaderObject,GL_COMPILE_STATUS,
					&iShaderCompiledStatus);
	if(iShaderCompiledStatus==GL_FALSE){
		glGetShaderiv(gFragmentShaderObject,GL_INFO_LOG_LENGTH,
					&iInfoLogLength);
		if(iInfoLogLength > 0){
			szInfoLog=(char*)malloc(iInfoLogLength);
			if(szInfoLog!=NULL){
				GLsizei written;
				glGetShaderInfoLog(gFragmentShaderObject,iInfoLogLength,
									&written,szInfoLog);
				fprintf(gpFile,"Fragment Shader Compilation Log: %s \n",
						szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(EXIT_FAILURE);
			}
		}
	}

	gShaderProgramObject=glCreateProgram();
	glAttachShader(gShaderProgramObject,gVertexShaderObject);
	glAttachShader(gShaderProgramObject,gFragmentShaderObject);
	glBindAttribLocation(gShaderProgramObject,SSK_ATTRIBUTE_VERTEX,
						"vPosition");
	glBindAttribLocation(gShaderProgramObject,SSK_ATTRIBUTE_TEXTURE0,
						"vTexture0_Coord");

	glLinkProgram(gShaderProgramObject);
	GLint iShaderProgramLinkStatus=0;
	glGetProgramiv(gShaderProgramObject,GL_LINK_STATUS,
					&iShaderProgramLinkStatus);
	if(iShaderProgramLinkStatus==GL_FALSE){
		glGetProgramiv(gShaderProgramObject,GL_INFO_LOG_LENGTH,
						&iInfoLogLength);
		if(iInfoLogLength>0){
			szInfoLog=(char *)malloc(iInfoLogLength);
			if(szInfoLog!=NULL){
				GLsizei written;
				glGetProgramInfoLog(gShaderProgramObject,iInfoLogLength,&written,
									szInfoLog);
				fprintf(gpFile,"Shader Program Link Log : %s\n",szInfoLog);
				free(szInfoLog);
				uninitialize();
				exit(EXIT_FAILURE);
			}
		}
	}

	gMVPUniform=glGetUniformLocation(gShaderProgramObject,"u_mvp_matrix");
	gTexture_sampler_uniform=glGetUniformLocation(gShaderProgramObject,
												"u_texture0_sampler");

	const GLfloat squareVertices[]=	{	
		1.0f,1.0f,0.0f,
		-1.0f,1.0f,0.0f,
		-1.0f,-1.0f,0.0f,
		1.0f,-1.0f,0.0f

	};

/*	const GLfloat squareTexcoords[]={
		1.0f,1.0f,
		0.0f,1.0f,
		0.0f,0.0f,
		1.0f,0.0f,
	};
*/

	const GLfloat squareTexcoords[]={
		1.0f,0.0f,
		0.0f,0.0f,
		0.0f,1.0f,
		1.0f,1.0f,
	};

	glGenVertexArrays(1,&gVao_square);
		glBindVertexArray(gVao_square);
			glGenBuffers(1,&gVbo_square_position);
			glBindBuffer(GL_ARRAY_BUFFER,gVbo_square_position);
			glBufferData(GL_ARRAY_BUFFER,sizeof(squareVertices),
						squareVertices,GL_STATIC_DRAW);
			glVertexAttribPointer(SSK_ATTRIBUTE_VERTEX,3,GL_FLOAT,
								GL_FALSE,0,NULL);
			glEnableVertexAttribArray(SSK_ATTRIBUTE_VERTEX);
		glBindBuffer(GL_ARRAY_BUFFER,0);

		glGenBuffers(1,&gVbo_square_texture);
		glBindBuffer(GL_ARRAY_BUFFER,gVbo_square_texture);
			glBufferData(GL_ARRAY_BUFFER,sizeof(squareTexcoords),
						squareTexcoords,GL_STATIC_DRAW);
			glVertexAttribPointer(SSK_ATTRIBUTE_TEXTURE0,2,GL_FLOAT,
								GL_FALSE,0,NULL);
			glEnableVertexAttribArray(SSK_ATTRIBUTE_TEXTURE0);
		glBindBuffer(GL_ARRAY_BUFFER,0);
	glBindVertexArray(0);


	glShadeModel(GL_SMOOTH);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	
//	loadDDS("C:/Users/SharK/Documents/Visual Studio 2015/Projects/01-RealTimeRendering-2017/02-RTR/04-MyStudy/08-DDSTexture map/water.dds");
//	nv_ddsLoader("water.dds");
//	glEnable(GL_TEXTURE_2D);


	ddsGL_load (imagename, &texture);
	
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	gPerspectiveProjectionMatrix=vmath::mat4::identity();
	resize(WIN_WIDTH,WIN_HEIGHT);
}

void display(void){
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT |
			GL_STENCIL_BUFFER_BIT);

	glUseProgram(gShaderProgramObject);
		vmath::mat4 modelViewMatrix;
		vmath::mat4 modelViewProjectionMatrix;

		modelViewMatrix=vmath::mat4::identity();
		modelViewMatrix=vmath::translate(0.0f,0.0f,-6.0f);
		modelViewProjectionMatrix=gPerspectiveProjectionMatrix * modelViewMatrix;
		glUniformMatrix4fv(gMVPUniform,1,GL_FALSE,modelViewProjectionMatrix);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,texture.id);
		glUniform1i(gTexture_sampler_uniform,0);

		glBindVertexArray(gVao_square);
			glDrawArrays(GL_TRIANGLE_FAN,0,4);
		glBindVertexArray(0);
	glUseProgram(0);

	SwapBuffers(ghdc);
}

void resize(int width,int height){
	if(height==0){
		height=1;
	}

	glViewport(0,0,(GLsizei)width,(GLsizei)height);
 
	if(width<=height){
		gPerspectiveProjectionMatrix=vmath::perspective(45.0f,
														(GLfloat)height/(GLfloat)width,
														0.1f,100.0f);
	}else{
		gPerspectiveProjectionMatrix=vmath::perspective(45.0f,
														(GLfloat)width/(GLfloat)height,
														0.1f,100.0f);
	}
}

void uninitialize(void){
	if(gbFullscreen==false){
		dwStyle=GetWindowLong(ghwnd,GWL_STYLE);
		SetWindowLong(ghwnd,GWL_STYLE,dwStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(ghwnd,&wpPrev);
		SetWindowPos(ghwnd,HWND_TOP,
					0,0,0,0,
					SWP_NOMOVE | SWP_NOSIZE |
					SWP_NOOWNERZORDER | SWP_NOZORDER |
					SWP_FRAMECHANGED);
		ShowCursor(TRUE);
	}

	if(gVao_square){
		glDeleteVertexArrays(1,&gVao_square);
		gVao_square=0;
	}

	if(gVbo_square_position){
		glDeleteBuffers(1,&gVbo_square_position);
		gVbo_square_position=0;
	}

	if(gVbo_square_texture){
		glDeleteBuffers(1,&gVbo_square_texture);
		gVbo_square_texture=0;
	}

	glDetachShader(gShaderProgramObject,gVertexShaderObject);
	glDetachShader(gShaderProgramObject,gFragmentShaderObject);

	glDeleteShader(gVertexShaderObject);
	gVertexShaderObject=0;
	glDeleteShader(gFragmentShaderObject);
	gFragmentShaderObject=0;

	glDeleteProgram(gShaderProgramObject);
	gShaderProgramObject=0;

	glUseProgram(0);

	wglMakeCurrent(NULL,NULL);
	wglDeleteContext(ghrc);
	ghrc=NULL;

	ReleaseDC(ghwnd,ghdc);
	ghdc=NULL;
	ghwnd=NULL;

	if(gpFile){
		fprintf(gpFile,"Log file is Successfully closed!!\n");
		fclose(gpFile);
		gpFile=NULL;
	}
	if(gpddsFile){
		fprintf(gpddsFile,"19. Log File of DDS is Successfully closed\n");
		fclose(gpddsFile);
    	gpddsFile=NULL;
    }
}




/* Load DDS file */
dds_uint dds_load (const char* filename, DDSTextureInfo* texture) {
	
	fprintf(gpddsFile,"3. Entering in dds_load() function \n");
    
    if(texture==NULL){
		printf("dds_load: invalid surface!\n");
    }
    FILE* file;
    char magic[4];
    dds_long bufferSize, curr, end;    
    file = fopen (filename, "rb");
    if (!file) {
        return DDS_CANNOT_OPEN;
    }

	fprintf(gpddsFile,"4. .dds file opened \n");
    
    fread (&magic, sizeof (char), 4, file);

	fprintf(gpddsFile,"5. magic number reading \n");

    if (strncmp (magic, "DDS ", 4) != 0) {
        texture->ok = DDS_FALSE;
        fclose (file);
        return DDS_INVALID_FILE;
    }

	fprintf(gpddsFile,"6. Checking magic number is valid or not \n");

    fread (&texture->surface, sizeof (DDSurfaceDesc), 1, file);

	fprintf(gpddsFile,"7. Getting surface descriptor \n");

    curr = ftell (file);
    fseek (file, 0, SEEK_END);
    end = ftell (file);
    fseek (file, curr, SEEK_SET);
    bufferSize = end - curr;
    texture->buffer_size = bufferSize;
		
	fprintf(gpddsFile,"8. Calculating pixel data size \n");

    texture->texels = (dds_ubyte*)malloc (bufferSize * sizeof (dds_ubyte));
    fread (texture->texels, sizeof (dds_ubyte), bufferSize, file);

    texture->ok = DDS_TRUE;

	fprintf(gpddsFile,"9. Reading pixel data (with mipmaps) \n");

    fclose (file);
    fprintf(gpddsFile,"10. leaving dds_load() function \n");
    return DDS_OK;
	
}

/* Free DDS texture
 */
void dds_free (DDSTextureInfo* texinfo) {
    if (texinfo->ok) {
        free (texinfo->texels);
        texinfo->texels = NULL;
        texinfo->ok = DDS_FALSE;
    }
}



dds_uint ddsGL_load (const char* filename, DDS_GL_TextureInfo* texture) {
   
   	fprintf(gpddsFile,"1. Entering in ddsGL_load() function\n");

    DDSTextureInfo textureInfo;
    fprintf(gpddsFile,"2. dds_load() function call made \n");
    dds_int error = dds_load (filename, &textureInfo);
    if (error != DDS_OK) {
        return -1;
    }
    fprintf(gpddsFile,"11. dds_load() function call finished \n");

    texture->width = textureInfo.surface.width;
    texture->height = textureInfo.surface.height;
    texture->num_mipmaps = textureInfo.surface.mip_level;
	
	fprintf(gpddsFile,"12. Filled members of local texture(filled by dds_load function) into global instance of texture(width,height,mipmaps) \n");
    switch (textureInfo.surface.format.fourcc) {
        case DDS_FOURCC_DXT1:
        	fprintf(gpddsFile,"13. In DXT1 case filling format and internal format\n");
            texture->format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
            texture->internal_format = 3;
            break;
        case DDS_FOURCC_DXT3:
        	fprintf(gpddsFile,"13. In DXT3 case filling format and internal format\n");
            texture->format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
            texture->internal_format = 4;
            break;
        case DDS_FOURCC_DXT5:
        	fprintf(gpddsFile,"13. In DXT5 case filling format and internal format\n");
            texture->format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
            texture->internal_format = 4;
            break;
        case DDS_FOURCC_BC4U:
        	fprintf(gpddsFile,"13. In BC4U case filling format and internal format\n");
            texture->format = GL_COMPRESSED_RED_RGTC1_EXT;
            texture->internal_format = 1;
            break;
        case DDS_FOURCC_BC4S:
        	fprintf(gpddsFile,"13. In BC4S case filling format and internal format\n");
            texture->format = GL_COMPRESSED_SIGNED_RED_RGTC1_EXT;
            texture->internal_format = 1;
            break;
        case DDS_FOURCC_BC5S:
        	fprintf(gpddsFile,"13. In BC5S case filling format and internal format\n");
            texture->format = GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT;
            texture->internal_format = 2;
            break;
        default:
            dds_free (&textureInfo);
            return DDS_BAD_COMPRESSION;
    }

    fprintf(gpddsFile,"14. Generating new texture, Binding texture,Filtering\n");

    glGenTextures (1, &texture->id);
    glEnable (GL_TEXTURE_2D);
    glBindTexture (GL_TEXTURE_2D, texture->id);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    size_t blockSize = texture->format == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ? 8 : 16;
    size_t offset = 0;
    dds_uint mipWidth = texture->width,
            mipHeight = texture->height,
            mipSize;

    fprintf(gpddsFile,"15. Filling the format in blocksize by checking it with GL_COMPRESSED_RGBA_S3TC_DXT1_EXT if true then 8 else 16 \n");

    fprintf(gpddsFile,"16. Uploading mipmaps to the video memory \n");
    size_t mip;
    
    for (mip = 0; mip < texture->num_mipmaps; mip++) {
        mipSize = ((mipWidth + 3) / 4) * ((mipHeight + 3) / 4) * blockSize;

//        ddsglCompressedTexImage2D (GL_TEXTURE_2D, mip, texture->format,mipWidth, mipHeight, 0, mipSize,textureInfo.texels + offset);
        glCompressedTexImage2D(GL_TEXTURE_2D, mip, texture->format,mipWidth, mipHeight, 0, mipSize,textureInfo.texels + offset);

        mipWidth = DDS_MAX (mipWidth >> 1, 1);
        mipHeight = DDS_MAX (mipHeight >> 1, 1);

        offset += mipSize;
    }
    fprintf(gpddsFile,"17. A loop calling glCompresedTexImage2D() which specifies a 2D texture image in compressed format\n");
    fprintf(gpddsFile,"18. Calling dds_free() function\n");
    dds_free (&textureInfo);
    return DDS_OK;
}


void ddsGL_free (DDS_GL_TextureInfo* texture) {
    if (texture->id) {
        glDeleteTextures (1, &texture->id);
    }
}

const char* dds_error_string (int err) {
    switch (err) {
        case DDS_OK:
            return "No error";
        case DDS_CANNOT_OPEN:
            return "Cannot open image file";
        case DDS_INVALID_FILE:
            return "Given file does not appear to be a valid DDS file";
        case DDS_BAD_COMPRESSION:
            return "Given file does not appear to be compressed with DXT1, DXT3 or DXT5";
        case DDS_NO_GL_SUPPORT:
            return "Seems that OpenGL has no \"glCompressedTexImage2D\" extension";
        case DDS_CANNOT_WRITE_FILE:
            return "Cannot write file";
        case DDS_BAD_TEXINFO:
            return "Bad texture given";
        default:
            return "Unknown error";
    }
}


