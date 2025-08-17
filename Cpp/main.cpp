#include <iostream>
// OPENGL 
#include <GL/glut.h>
#include <GL/freeglut.h>
// GLM
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp> // Include GLM header for matrix operations
#include <glm/gtx/quaternion.hpp>
// boost
#include <boost/interprocess/windows_shared_memory.hpp>
#include <boost/interprocess/mapped_region.hpp>

#include <thread>
#include <ctime>
#include <pthread.h>
#include <queue>
#include <tuple>

using namespace boost::interprocess;

// helpers & variables

int g_argc;         // opengl
char **g_argv;

void* addr;

struct SharedData {
    char message[256];
    bool ready;
};


// threads
pthread_t opengl_thread;
pthread_t python_thread;

// window
int window = 0;
GLuint gl_depth_tex;
int windowWidth = 640;   // Update these with your window dimensions
int windowHeight = 480;

// camera
// Camera position (for translating the camera)
float cameraX = 0.0f, cameraY = 0.0f, cameraZ = -5.0f; // Start far enough to see the points
glm::quat rotation = glm::quat(-.064675, 0.782132, 0.019156, -0.372425);  // identity quaternion


// mouse
// // Last mouse position
int lastMouseX, lastMouseY;
bool leftMouseDown = false;
bool rightMouseDown = false;  // For rotating
// Scroll sensitivity for zooming
float zoomSensitivity = 0.5f;
int mouseX = 0, mouseY = 0;
float mouseWorldX = 0, mouseWorldY = 0 , mouseWorldZ = 0;

glm::vec3 getArcballVector(int x, int y, int width, int height) {
    glm::vec3 P(
        1.0f * x / width * 2 - 1.0f,
        1.0f * y / height * 2 - 1.0f,
        0.0f
    );
    // Invert y so that +Y is up
    P.y = -P.y;
    float OP_squared = P.x * P.x + P.y * P.y;
    if (OP_squared <= 1.0f)
        P.z = sqrt(1.0f - OP_squared);  // Pythagoras
    else
        P = glm::normalize(P);  // nearest point on sphere
    return P;
}

void GetMouseMotion(int x, int y) {
    mouseX = x;
    mouseY = windowHeight - y;  // Convert to OpenGL coordinate system

    // Read the depth buffer at the mouse position
    float depth;
    glReadPixels(mouseX, mouseY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

    // Convert depth to world coordinates
    glm::vec3 screenPos(mouseX, mouseY, depth);
    glm::mat4 projection, view;
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projection));
    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(view));
    
    glm::vec4 viewport(0, 0, windowWidth, windowHeight);
    glm::vec3 worldPos = glm::unProject(screenPos, view, projection, viewport);

    // Store world coordinates for rendering
    mouseWorldX = worldPos.x;
    mouseWorldY = worldPos.y;
    mouseWorldZ = worldPos.z;
}

// Function to handle mouse dragging
void mouseMotion(int x, int y) {
    int deltaX = x - lastMouseX;
    int deltaY = y - lastMouseY;

    if (leftMouseDown) {
        // Translate the camera position based on mouse movement
        cameraX += deltaX * 0.02f;  // Adjust sensitivity as needed
        cameraY -= deltaY * 0.02f;  // Inverted Y-axis
        glutPostRedisplay();
    }

    if (rightMouseDown) {
        // Arcball rotation update
        glm::vec3 va = getArcballVector(lastMouseX, lastMouseY, windowWidth, windowHeight);
        glm::vec3 vb = getArcballVector(x, y, windowWidth, windowHeight);
        // Compute the angle between the two vectors
        float angle = acos(std::min(1.0f, glm::dot(va, vb)));
        // Compute the rotation axis (it should be perpendicular to both)
        glm::vec3 axis = glm::cross(va, vb);
        if (glm::length(axis) > 1e-6) {  // Ensure non-zero rotation
            axis = glm::normalize(axis);
            // Create a quaternion for this incremental rotation
            glm::quat q = glm::angleAxis(angle, axis);
            // Update the global rotation (note the order: new rotation * current rotation)
            rotation = q * rotation;
        }
        glutPostRedisplay();
    }

    // Update last mouse position
    lastMouseX = x;
    lastMouseY = y;
}

// Function to handle mouse button events
void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            leftMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        } else if (state == GLUT_UP) {
            leftMouseDown = false;
        }
    }
    
    if (button == GLUT_RIGHT_BUTTON) {
        if (state == GLUT_DOWN) {
            rightMouseDown = true;
            lastMouseX = x;
            lastMouseY = y;
        } else if (state == GLUT_UP) {
            rightMouseDown = false;
        }
    }
}

// Function to handle the mouse scroll (zoom)
void mouseWheel(int button, int dir, int x, int y) {
    if (dir > 0) {
        // Scroll up - Zoom in
        cameraZ += zoomSensitivity;
    } else {
        // Scroll down - Zoom out
        cameraZ -= zoomSensitivity;
    }

    // Request a redraw
    glutPostRedisplay();
}

void drawArrowHead(float x, float y, float z, float r, float g, float b) {
    glColor3f(r, g, b);
    glPushMatrix();
    glTranslatef(x, y, z);
    glutSolidCone(0.05, 0.1, 10, 10); // Puntas pequeñas
    glPopMatrix();
}

void drawXYZAxes() { 
    glLineWidth(2.0f);

    glBegin(GL_LINES);

    // Eje X en rojo
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);

    // Eje Y en verde
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);

    // Eje Z en azul
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 1.0f);

    glEnd();

    // Dibujar puntas de flecha
    drawArrowHead(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f); // X (rojo)
    drawArrowHead(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f); // Y (verde)
    drawArrowHead(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f); // Z (azul)
}

void reshape(int width, int height) {
    if (height == 0) height = 1;
    windowWidth = width;
    windowHeight = height;

    float aspectRatio = static_cast<float>(width) / height;
    glViewport(0, 0, width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float nearPlane = 0.01f;
    float farPlane = 10000.0f;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, nearPlane, farPlane);
    glLoadMatrixf(glm::value_ptr(projection));

    glMatrixMode(GL_MODELVIEW);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) { // 27 is the ASCII code for the Escape key
        exit(0);
        glutLeaveMainLoop();
        glutDestroyWindow(glutGetWindow());
    }
}

void DrawGLScene() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // RGBA (white, fully opaque)

    // Apply camera transformations
    glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(cameraX, cameraY, cameraZ));
    view *= glm::mat4_cast(rotation); // Apply quaternion rotation
    glLoadMatrixf(glm::value_ptr(view));

    // Draw XYZ axes
    drawXYZAxes();

    // Draw a simple point cloud (random cluster near origin)
    glPointSize(4.0f); // Size of points
    glBegin(GL_POINTS);
    for (int i = 0; i < 500; i++) {
        float x = ((rand() % 100) / 100.0f - 0.5f) * 2.0f; // random [-1,1]
        float y = ((rand() % 100) / 100.0f - 0.5f) * 2.0f;
        float z = ((rand() % 100) / 100.0f - 0.5f) * 2.0f;

        glColor3f((x+1.0f)/2.0f, (y+1.0f)/2.0f, (z+1.0f)/2.0f); // RGB based on position
        glVertex3f(x, y, z);
    }
    glEnd();

    glutSwapBuffers();
}

void* RealSenseThread(void* arg) {
    // Register the cleanup function
    // atexit(cleanup);
    
    printf("GL thread\n");
    glutInit(&g_argc, g_argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
    glutInitWindowSize(640, 480);
    glutInitWindowPosition(0, 0);
    window = glutCreateWindow("Realsense Point Cloud");
    glutDisplayFunc(&DrawGLScene);
    glutIdleFunc(&DrawGLScene);
	glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutMouseWheelFunc(mouseWheel);  // Register the mouse scroll function
    glutMotionFunc(mouseMotion);
    glutPassiveMotionFunc(GetMouseMotion);
    glutKeyboardFunc(keyboard);
    glEnable(GL_DEPTH_TEST);
    glClearDepth(1.0);

    // — RGB window —
    // glutInitWindowSize(640, 480);
    // glutInitWindowPosition(1300, 100);
    // rgbWindow = glutCreateWindow("RGB Frame Viewer");
    // glutDisplayFunc(&DrawRGBWindow);
    // InitRGBGL();  // your RGB GL init (rgb texture, etc.)

    // — Single idle to drive BOTH windows —
    glutIdleFunc([](){
        // queue redraw of point‑cloud
        glutSetWindow(window);
        glutPostRedisplay();
        // queue redraw of RGB
        // glutSetWindow(rgbWindow);
        // glutPostRedisplay();
    });

    // Enter the GLUT main loop (never returns)
    glutMainLoop();
    return NULL;
}

void* PythonCom(void* arg) {
   
    // Create Windows shared memory (same name Python expects)
    windows_shared_memory shm(create_only, "MySharedMemory", read_write, 1024);
    mapped_region region(shm, read_write);

    char* mem = static_cast<char*>(region.get_address());

    int counter = 0;
    while (true) {
        std::string msg = "Hello from C++ " + std::to_string(counter++);
        std::strncpy(mem, msg.c_str(), 1024);
        std::cout << "C++ wrote: " << msg << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return nullptr; // never reached
}

// void Init(){
    
// }

int main(int argc, char* argv[]) {

    g_argc = argc;
    g_argv = argv;
    
    std::cout << "hello\n";

    // Init();

    // // Create the second thread
    // if (pthread_create(&opengl_thread, NULL, RealSenseThread, NULL) != 0) {
    //     std::cerr << "Error creating RealSenseThread thread!" << std::endl;
    //     return -1;
    // }

    // Python comm
    if (pthread_create(&python_thread, NULL, PythonCom, NULL) != 0) {
        std::cerr << "Error creating PythonCom thread!" << std::endl;
        return -1;
    }


    // pthread_join(opengl_thread, NULL);
    pthread_join(python_thread, NULL);
    
    return 0;
}
