#define WINDOWS_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

// OpenGL Graphics
#include <GL/glew.h>
#include <GL/freeglut.h>

// system
#include <memory>
#include <iostream>
#include <sstream>
#include <cassert>

#if USE_GPU
#include <CL/opencl.h>
#endif 

// Kinect
#include "KinectWrapper.h"

// Constants
KinectWrapper* kinectWrapper = 0;

const int MAX_DEVICES     = 10;
const int MAX_SOURCE_SIZE = 65536;

#define GL_SHARING_EXTENSION "cl_khr_gl_sharing"

#if USE_GPU
cl_mem    ghDepth;

std::string getErrorDesc(int err)
{
	switch (err)
	{
	case CL_SUCCESS                        : return "CL_SUCCESS";
	case CL_DEVICE_NOT_FOUND               : return "CL_DEVICE_NOT_FOUND";
	case CL_COMPILER_NOT_AVAILABLE         : return "CL_COMPILER_NOT_AVAILABLE";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE  : return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case CL_OUT_OF_RESOURCES               : return "CL_OUT_OF_RESOURCES";
	case CL_OUT_OF_HOST_MEMORY             : return "CL_OUT_OF_HOST_MEMORY";
	case CL_PROFILING_INFO_NOT_AVAILABLE   : return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case CL_MEM_COPY_OVERLAP               : return "CL_MEM_COPY_OVERLAP";
	case CL_IMAGE_FORMAT_MISMATCH          : return "CL_IMAGE_FORMAT_MISMATCH";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED     : return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case CL_BUILD_PROGRAM_FAILURE          : return "CL_BUILD_PROGRAM_FAILURE";
	case CL_MAP_FAILURE                    : return "CL_MAP_FAILURE";

	case CL_INVALID_VALUE                  : return "CL_INVALID_VALUE";
	case CL_INVALID_DEVICE_TYPE            : return "CL_INVALID_DEVICE_TYPE";
	case CL_INVALID_PLATFORM               : return "CL_INVALID_PLATFORM";
	case CL_INVALID_DEVICE                 : return "CL_INVALID_DEVICE";
	case CL_INVALID_CONTEXT                : return "CL_INVALID_CONTEXT";
	case CL_INVALID_QUEUE_PROPERTIES       : return "CL_INVALID_QUEUE_PROPERTIES";
	case CL_INVALID_COMMAND_QUEUE          : return "CL_INVALID_COMMAND_QUEUE";
	case CL_INVALID_HOST_PTR               : return "CL_INVALID_HOST_PTR";
	case CL_INVALID_MEM_OBJECT             : return "CL_INVALID_MEM_OBJECT";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case CL_INVALID_IMAGE_SIZE             : return "CL_INVALID_IMAGE_SIZE";
	case CL_INVALID_SAMPLER                : return "CL_INVALID_SAMPLER";
	case CL_INVALID_BINARY                 : return "CL_INVALID_BINARY";
	case CL_INVALID_BUILD_OPTIONS          : return "CL_INVALID_BUILD_OPTIONS";
	case CL_INVALID_PROGRAM                : return "CL_INVALID_PROGRAM";
	case CL_INVALID_PROGRAM_EXECUTABLE     : return "CL_INVALID_PROGRAM_EXECUTABLE";
	case CL_INVALID_KERNEL_NAME            : return "CL_INVALID_KERNEL_NAME";
	case CL_INVALID_KERNEL_DEFINITION      : return "CL_INVALID_KERNEL_DEFINITION";
	case CL_INVALID_KERNEL                 : return "CL_INVALID_KERNEL";
	case CL_INVALID_ARG_INDEX              : return "CL_INVALID_ARG_INDEX";
	case CL_INVALID_ARG_VALUE              : return "CL_INVALID_ARG_VALUE";
	case CL_INVALID_ARG_SIZE               : return "CL_INVALID_ARG_SIZE";
	case CL_INVALID_KERNEL_ARGS            : return "CL_INVALID_KERNEL_ARGS";
	case CL_INVALID_WORK_DIMENSION         : return "CL_INVALID_WORK_DIMENSION";
	case CL_INVALID_WORK_GROUP_SIZE        : return "CL_INVALID_WORK_GROUP_SIZE";
	case CL_INVALID_WORK_ITEM_SIZE         : return "CL_INVALID_WORK_ITEM_SIZE";
	case CL_INVALID_GLOBAL_OFFSET          : return "CL_INVALID_GLOBAL_OFFSET";
	case CL_INVALID_EVENT_WAIT_LIST        : return "CL_INVALID_EVENT_WAIT_LIST";
	case CL_INVALID_OPERATION              : return "CL_INVALID_OPERATION";
	case CL_INVALID_GL_OBJECT              : return "CL_INVALID_GL_OBJECT";
	case CL_INVALID_BUFFER_SIZE            : return "CL_INVALID_BUFFER_SIZE";
	case CL_INVALID_MIP_LEVEL              : return "CL_INVALID_MIP_LEVEL";
	default: return "UNKNOWN";
	}
}

#define CHECKSTATUS( stmt, status, functor ) \
{ \
	int __status = stmt; \
	if( __status != status ) { \
	std::stringstream __s; \
	__s << "==> " #stmt "\n"; \
	__s << "ERROR : " << getErrorDesc(__status) << "\n" ; \
	__s << "<== " #stmt "\n"; \
	std::cout << __s.str().c_str() << std::endl; \
	functor(__status); \
	} \
}

const char* loadFromFile( const std::string& filename, size_t& length )
{
	// Load the kernel source code into the array source_str
	FILE *fp = 0;
	char *source_str = 0;

	fopen_s( &fp, filename.c_str(), "r");
	if( fp == 0 ) 
	{
		std::cout << "Failed to load kernel " << filename.c_str() << std::endl;
	}
	else 
	{
		source_str = (char*)malloc(MAX_SOURCE_SIZE);
		length = fread( source_str, 1, MAX_SOURCE_SIZE, fp);
		fclose( fp );
	}
	return source_str;
}
#endif

// Constants, defines, typedefs and global declarations
//*****************************************************************************
#define REFRESH_DELAY	  10 //ms

// Rendering window vars
const unsigned int window_width = 1024;
const unsigned int window_height = 768;

#if USE_GPU
// OpenCL vars
cl_platform_id clPlatforms[MAX_DEVICES];
cl_context clContext = 0;
cl_device_id clDevices[MAX_DEVICES];
cl_uint uiDevCount;
cl_command_queue clCommandQueue;
cl_kernel clKernel;
cl_mem clBuffer = 0;
cl_program clProgram;
size_t szGlobalWorkSize[] = {mesh_width, mesh_height};
const char* cExecutableName = 0;
GLuint glBuffer(0);
#endif

// clBuffer variables
int iGLUTWindowHandle(0);          // handle to the GLUT window

// mouse controls
int mouse_old_x, mouse_old_y;
int mouse_buttons = 0;

float rotate_x = 0.0;
float rotate_y = 0.0;
float rotate_z = 0.0;
float translate_z = -3.0;

// Cursor info
int gMouseTimer = 0;
int gMouseClickDelay = 50;
int gMouseX = 0;
int gMouseNewX = 0;
int gMouseY = 0;
int gMouseNewY = 0;
int gMousePrecision = 5;

float gScale = 0.f; 
float gScaleAnim = 0.f; 
float gPointSize = 1.f; 
bool gActiveSqueleton = false;
bool gActiveColor= true;
bool gMouseControl = false;

// Sim and Auto-Verification parameters 
float gAnim = 0.f;
float gAngle= 0.f;
int iFrameCount = 0;                // FPS count for averaging
int iFrameTrigger = 90;             // FPS trigger for sampling
int iFramesPerSec = 0;              // frames per second
int iTestSets = 3;
int g_Index = 0;
bool bQATest = false;
bool bNoPrompt = false;  

int *pArgc = 0;
char **pArgv = 0;

// Forward Function declarations
//*****************************************************************************
// OpenCL functionality
void runKernel();

// GL functionality
void InitGL(int* argc, char** argv);
void DisplayGL();
void KeyboardGL(unsigned char key, int x, int y);
void mouse(int button, int state, int x, int y);
void motion(int x, int y);
void timerEvent(int value);

// Helpers
void Cleanup(int iExitCode);

// Main program
//*****************************************************************************
int main(int argc, char** argv)
{
	InitGL(&argc, argv);

#if USE_GPU
	cl_int status = CL_SUCCESS;
	int selectedPlatform=1;
	int selectedDevice=0;

	//Get the NVIDIA platform
	cl_uint platforms;
	CHECKSTATUS(clGetPlatformIDs(MAX_DEVICES, clPlatforms, &platforms), CL_SUCCESS, Cleanup);

	// Create the device list
	CHECKSTATUS(clGetDeviceIDs(clPlatforms[selectedPlatform], CL_DEVICE_TYPE_ALL, 1, clDevices, &uiDevCount), CL_SUCCESS, Cleanup);

	cl_context_properties props[] = 
	{
		CL_GL_CONTEXT_KHR,   (cl_context_properties)wglGetCurrentContext(), 
		CL_WGL_HDC_KHR,      (cl_context_properties)wglGetCurrentDC(), 
		CL_CONTEXT_PLATFORM, (cl_context_properties)clPlatforms,
		0
	};

	clContext = clCreateContext(props, 1, &clDevices[selectedDevice], 0, 0, &status);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);

	// create a command-queue
	clCommandQueue = clCreateCommandQueue(clContext, clDevices[selectedDevice], 0, &status);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);

	// Program Setup
	size_t program_length;
	const char *source = loadFromFile("KinectTornado.cl", program_length);

	// create the program
	clProgram = clCreateProgramWithSource(clContext, 1, (const char **) &source, &program_length, &status);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);

	// build the program
	CHECKSTATUS(clBuildProgram(clProgram, 0, 0, "-cl-fast-relaxed-math", 0, 0), CL_SUCCESS, Cleanup);

	// create the kernel
	clKernel = clCreateKernel(clProgram, "sine_wave", &status);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);

	// create clBuffer (if using standard GL or CL-GL interop), otherwise create Cl buffer
	unsigned int size = mesh_width * mesh_height * 4 * sizeof(float);

	// create buffer object
	glGenBuffers(1, &glBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, glBuffer);

	// initialize buffer object
	glBufferData(GL_ARRAY_BUFFER, size, 0, GL_DYNAMIC_DRAW);

	// create OpenCL buffer from GL clBuffer
	clBuffer = clCreateFromGLBuffer(clContext, CL_MEM_WRITE_ONLY, glBuffer, &status);
	CHECKSTATUS( status, CL_SUCCESS, Cleanup );

	// set the args values 
	status  = clSetKernelArg(clKernel, 0, sizeof(cl_mem), (void *) &clBuffer);
	status |= clSetKernelArg(clKernel, 1, sizeof(unsigned int), &mesh_width);
	status |= clSetKernelArg(clKernel, 2, sizeof(unsigned int), &mesh_height);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);
#endif

	// --------------------------------------------------------------------------------
	// Initialize Kinect
	// --------------------------------------------------------------------------------
	kinectWrapper = new KinectWrapper();
	kinectWrapper->initialize();
#if USE_GPU
	ghDepth = clCreateBuffer( clContext, CL_MEM_READ_ONLY , gDepthWidth*gDepthHeight*gKinectColorDepth, 0, 0);
#endif
	// --------------------------------------------------------------------------------

	// Start main GLUT rendering loop for processing and rendering,
	glutMainLoop();

	// Normally unused return path
	Cleanup(EXIT_SUCCESS);
}

// Initialize GL
//*****************************************************************************
void InitGL(int* argc, char** argv)
{
	// initialize GLUT 
	glutInit(argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE);
	glutInitWindowPosition (glutGet(GLUT_SCREEN_WIDTH)/2 - window_width/2, 
		glutGet(GLUT_SCREEN_HEIGHT)/2 - window_height/2);
	glutInitWindowSize(window_width, window_height);
	iGLUTWindowHandle = glutCreateWindow("Kinect Demo");
	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// register GLUT callback functions
	glutDisplayFunc(DisplayGL);
	glutKeyboardFunc(KeyboardGL);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutTimerFunc(REFRESH_DELAY, timerEvent,0);

	// initialize necessary OpenGL extensions
	glewInit();
	if( !glewIsSupported("GL_VERSION_2_0 GL_ARB_pixel_buffer_object") )
	{
		Cleanup(-1);
	}

	// default initialization
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glDisable(GL_DEPTH_TEST);

	// viewport
	glViewport(0, 0, window_width, window_height);

	// projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, (GLfloat)window_width / (GLfloat) window_height, 0.1, 10.0);

	return;
}

// Run the OpenCL part of the computation
//*****************************************************************************
void runKernel()
{

	// map OpenGL buffer object for writing from OpenCL
	glFinish();
#if USE_GPU
	cl_int status = CL_SUCCESS;
	status = clEnqueueAcquireGLObjects(clCommandQueue, 1, &clBuffer, 0,0,0);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);
#endif

	// Get Depth Map from kinect
	BYTE* depth = kinectWrapper->getDepthFrame();

	//kinectWrapper->getClick();

#if USE_GPU
	if( depth ) clEnqueueWriteBuffer( clCommandQueue, ghDepth, CL_FALSE, 0, gKinectColorDepth*gDepthWidth*gDepthHeight, depth, 0, 0, 0);

	// Set arg 3 and execute the kernel
	status = clSetKernelArg(clKernel, 3, sizeof(float),  &anim);
	status = clSetKernelArg(clKernel, 4, sizeof(cl_mem), &ghDepth);
	status |= clEnqueueNDRangeKernel(clCommandQueue, clKernel, 2, 0, szGlobalWorkSize, 0, 0,0,0 );
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);

	// unmap buffer object
	status = clEnqueueReleaseGLObjects(clCommandQueue, 1, &clBuffer, 0,0,0);
	CHECKSTATUS(status, CL_SUCCESS, Cleanup);
	clFinish(clCommandQueue);
#endif
}

// Display callback
//*****************************************************************************
void DisplayGL()
{
	// run OpenCL kernel to generate vertex positions
	runKernel();

	float3 positions[NUI_SKELETON_POSITION_COUNT];
	kinectWrapper->getSkeletonPosisions( positions );

	// set view matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
	glEnable(GL_DEPTH_TEST);

	if( gActiveSqueleton )
	{
		glLineWidth(gPointSize);
		glBegin(GL_LINE_STRIP);
		glColor3f(0.f,1.f,1.f);
		glVertex3f( positions[NUI_SKELETON_POSITION_HAND_LEFT].x, positions[NUI_SKELETON_POSITION_HAND_LEFT].y, -positions[NUI_SKELETON_POSITION_HAND_LEFT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_ELBOW_LEFT].x, positions[NUI_SKELETON_POSITION_ELBOW_LEFT].y, -positions[NUI_SKELETON_POSITION_ELBOW_LEFT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_SHOULDER_LEFT].x, positions[NUI_SKELETON_POSITION_SHOULDER_LEFT].y, -positions[NUI_SKELETON_POSITION_SHOULDER_LEFT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].x, positions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].y, -positions[NUI_SKELETON_POSITION_SHOULDER_RIGHT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_ELBOW_RIGHT].x, positions[NUI_SKELETON_POSITION_ELBOW_RIGHT].y, -positions[NUI_SKELETON_POSITION_ELBOW_RIGHT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_HAND_RIGHT].x, positions[NUI_SKELETON_POSITION_HAND_RIGHT].y, -positions[NUI_SKELETON_POSITION_HAND_RIGHT].z-translate_z );
		glEnd();
		glBegin(GL_LINE_STRIP);
		glColor3f(1.f,0.f,0.f);
		glVertex3f( positions[NUI_SKELETON_POSITION_HEAD].x, positions[NUI_SKELETON_POSITION_HEAD].y, -positions[NUI_SKELETON_POSITION_HEAD].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_SPINE].x, positions[NUI_SKELETON_POSITION_SPINE].y, -positions[NUI_SKELETON_POSITION_SPINE].z-translate_z );
		glEnd();
		glBegin(GL_LINE_STRIP);
		glColor3f(1.f,1.f,0.f);
		glVertex3f( positions[NUI_SKELETON_POSITION_FOOT_LEFT].x, positions[NUI_SKELETON_POSITION_FOOT_LEFT].y, -positions[NUI_SKELETON_POSITION_FOOT_LEFT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_KNEE_LEFT].x, positions[NUI_SKELETON_POSITION_KNEE_LEFT].y, -positions[NUI_SKELETON_POSITION_KNEE_LEFT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_HIP_LEFT].x, positions[NUI_SKELETON_POSITION_HIP_LEFT].y, -positions[NUI_SKELETON_POSITION_HIP_LEFT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_HIP_CENTER].x, positions[NUI_SKELETON_POSITION_HIP_CENTER].y, -positions[NUI_SKELETON_POSITION_HIP_CENTER].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_HIP_RIGHT].x, positions[NUI_SKELETON_POSITION_HIP_RIGHT].y, -positions[NUI_SKELETON_POSITION_HIP_RIGHT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_KNEE_RIGHT].x, positions[NUI_SKELETON_POSITION_KNEE_RIGHT].y, -positions[NUI_SKELETON_POSITION_KNEE_RIGHT].z-translate_z );
		glVertex3f( positions[NUI_SKELETON_POSITION_FOOT_RIGHT].x, positions[NUI_SKELETON_POSITION_FOOT_RIGHT].y, -positions[NUI_SKELETON_POSITION_FOOT_RIGHT].z-translate_z );
		glEnd();

	}

	glColor3f(0.5f,0.5f,0.5f);
	if( gMouseControl )
	{
		gMouseNewX = positions[NUI_SKELETON_POSITION_HAND_RIGHT].x*1000.f;
		gMouseNewY = positions[NUI_SKELETON_POSITION_HAND_RIGHT].y*1000.f;
		SetCursorPos(960.f+gMouseNewX*2.f,540.f-gMouseNewY*2.f);

		if( abs(gMouseX-gMouseNewX) < gMousePrecision && abs(gMouseY-gMouseNewY) < gMousePrecision )
		{
			float r = (float)gMouseTimer/(float)gMouseClickDelay;
			glColor3f(0.0f,1.f-r,0.f);
			gMouseTimer++;
			if( gMouseTimer > gMouseClickDelay)
			{
				glColor3f(1.0,0.f,0.f);
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, 0);
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
				gMouseTimer = 0;
			}
		}
		else
		{
			gMouseTimer=0;
		}
		gMouseX = gMouseNewX;
		gMouseY = gMouseNewY;
	}

	glMatrixMode(GL_MODELVIEW);

	glLoadIdentity();
	glTranslatef(0.0, 0.0, translate_z);

	gAngle += gAnim;
	gScale += gScaleAnim;
	if( gScale >= 0.001f ) gScaleAnim = 0.f;
	if( gScale < 0.f ) gScaleAnim = 0.f;
	//glRotatef(anim*2, 1.0, 0.0, 0.0);
	glRotatef(gAngle, 0.0, 1.0, 0.0);
	//glRotatef(anim*9, 0.0, 0.0, 1.0);

#if USE_GPU
	glBindBuffer(GL_ARRAY_BUFFER, glBuffer);
	glVertexPointer(4, GL_FLOAT, 0, 0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glColor3f(1.0, 1.0, 1.0);
	glDrawArrays(GL_POINTS, 0, mesh_width * mesh_height);
	glDisableClientState(GL_VERTEX_ARRAY);
#else
	const int step = 1;
	BYTE* colors = kinectWrapper->getVideoFrame();
	BYTE* depth = kinectWrapper->getDepthFrame();
	glPointSize(gPointSize);
	glBegin(GL_POINTS);
	for( int y(0); y<kinectWrapper->gVideoHeight; y+=step)
	{
		for( int x(0); x<kinectWrapper->gVideoWidth; x+=step)
		{
			int depthIndex = ((y/2-12)*kinectWrapper->gDepthWidth+(x/2-12))*kinectWrapper->gKinectDepthDepth;
			if( depthIndex >= 0 )
			{
				depthIndex = depthIndex%(kinectWrapper->gDepthWidth*kinectWrapper->gDepthHeight*kinectWrapper->gKinectDepthDepth);

				bool drawPoint = true;
				unsigned char da = depth[depthIndex];
				unsigned char s = da & 7;

				if( gActiveSqueleton )
				{
					drawPoint = ( s != 0 );
				}

				if( drawPoint )
				{
					unsigned char db = depth[depthIndex+1];
					unsigned short d1 = da >> 3;
					unsigned short d2 = db << 5;
					unsigned short d = d1|d2;

					if( d!=0 || gScale==0.f )
					{
						if( gActiveColor ) 
						{
							int index = (y*kinectWrapper->gVideoWidth+x)*kinectWrapper->gKinectVideoDepth;
							unsigned char r=colors[index+2];
							unsigned char g=colors[index+1];
							unsigned char b=colors[index+0];
							glColor3f( r/255.f, g/255.f, b/255.f);
						}

						float distance = 2000.f-d;
						glVertex3f( 
							(x-(kinectWrapper->gVideoWidth/2))*0.005f, 
							((kinectWrapper->gVideoHeight/2)-y)*0.005f, 
							distance*gScale );
					}
				}
			}
		}
	}
	glEnd();
#endif

	// flip backbuffer to screen
	glutSwapBuffers();
}

void timerEvent(int value)
{
	glutPostRedisplay();
	glutTimerFunc(REFRESH_DELAY, timerEvent,0);
}

// Keyboard events handler
//*****************************************************************************
void KeyboardGL(unsigned char key, int x, int y)
{
	switch(key) 
	{
	case '1'   : gAnim = -1.f; break;
	case '2'   : gAnim =  0.f; gAngle = 0.f; break;
	case '3'   : gAnim =  1.0f; break;
	case '4'   : gAnim =  0.f; gAngle = -45.f; break;
	case '5'   : gScaleAnim = 0.00005f; break;
	case '6'   : gScale = 0.f; gScaleAnim = 0.f; break;
	case '+'   : gPointSize += 1.f; break;
	case '-'   : gPointSize -= 1.f; if(gPointSize<0.f) gPointSize = 0.f; break;
	case 's'   : gActiveSqueleton = !gActiveSqueleton; break;
	case 'c'   : gActiveColor = !gActiveColor; break;
	case 'm'   : gMouseControl = !gMouseControl; break;
	case '\033': // escape quits
	case '\015': // Enter quits    
	case 'Q':    // Q quits
	case 'q':    // q (or escape) quits
		// Cleanup up and quit
		bNoPrompt = true;
		Cleanup(EXIT_SUCCESS);
		break;
	}
}

// Mouse event handlers
//*****************************************************************************
void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN) {
		mouse_buttons |= 1<<button;
	} else if (state == GLUT_UP) {
		mouse_buttons = 0;
	}

	mouse_old_x = x;
	mouse_old_y = y;
}

void motion(int x, int y)
{
	float dx, dy;
	dx = (float)(x - mouse_old_x);
	dy = (float)(y - mouse_old_y);

	if (mouse_buttons & 1) {
		rotate_x += dy * 0.2f;
		rotate_y += dx * 0.2f;
	} else if (mouse_buttons & 4) {
		translate_z += dy * 0.01f;
	}

	mouse_old_x = x;
	mouse_old_y = y;

	// set view matrix
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, translate_z);
	glRotatef( rotate_x, 1.0, 0.0, 0.0);
	glRotatef(-rotate_y, 0.0, 1.0, 0.0);
}

// Function to clean up and exit
//*****************************************************************************
void Cleanup(int iExitCode)
{
	// Kinect
	delete kinectWrapper;

#if USE_GPU
	// Cleanup allocated objects
	if(clKernel)       clReleaseKernel(clKernel); 
	if(clProgram)      clReleaseProgram(clProgram);
	if(clCommandQueue) clReleaseCommandQueue(clCommandQueue);
	if(clBuffer)
	{
		glBindBuffer(1, glBuffer);
		glDeleteBuffers(1, &glBuffer);
		clBuffer = 0;
	}
	if(clBuffer)clReleaseMemObject(clBuffer);
	if(ghDepth)clReleaseMemObject(ghDepth);
	if(clContext)clReleaseContext(clContext);
#endif 
	exit (iExitCode);
}
