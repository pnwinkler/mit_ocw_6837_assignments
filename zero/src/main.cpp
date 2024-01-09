// #include <GL/glu.h>
#include <fstream>
#include <filesystem>
#include <GL/freeglut.h>
#include <iostream>
#include <vector>
#include <sstream>
// todo: fix the make file(s)/CMake/build configuration instead of include .cpp files here
#include "../vecmath/Vector3f.cpp"
#include "../vecmath/Vector2f.h"
#include "../vecmath/Vector2f.cpp"
using namespace std;

// Globals
static int currentWindowWidth = 520;
static int currentWindowHeight = 520;
const unsigned short MAX_BUFFER_SIZE = 4096;
struct MeshData
{
    // a structure holding mesh data, specifically:
    // a vector of faces vecf (indices into vecv and vecn),
    // and normals (3D vectors) vecn and vecv
    vector<Vector3f> vecv;
    vector<Vector3f> vecn;
    vector<vector<unsigned>> vecf;
    string loaded_from = "";
};
MeshData mesh_data;

// color setting
// I chose unsigned because it doesn't wrap around to a negative number
static unsigned int target_color_idx = 0;
GLfloat DIFFCOLORS[10][4] = {
    {0.2, 0.5, 0.1, 0.0},
    {0.4, 0.5, 0.6, 0.2},
    {0.3, 0.6, 0.1, 1.0},
    {0.9, 0.4, 0.3, 0.5},
    {0.3, 0.4, 0.5, 1.0},
    {0.5, 0.7, 0.8, 0.3},
    {0.5, 0.9, 0.3, 1.0},
    {0.5, 0.5, 0.9, 1.0},
    {0.9, 0.5, 0.5, 1.0},
    {0.3, 0.8, 0.9, 1.0}};
// these vars are used to fade between colors. Start with an arbitrary default target color
GLfloat current_color[4] = {0.2, 0.2, 0.2, 0.2};
GLfloat target_color[4];

// the color between the current and target colors
GLfloat intermediate_color[4] = {};

// the ID of the shape that we currently have loaded
static int current_shape_id;

// Light position
GLfloat Lt0pos[] = {1.0, 1.0, 5.0, 1.0};
static int leftRightMultiplier = 1;
static int upDownMultiplier = 1;

// Rotation
static short angleRotationX = 0;
static short angleRotationY = 0;
static short angleRotationZ = 0;

// where our camera is positioned
static float lookZ = 5.0f;

// vars used for mouse controls
static int leftMouseButtonActionPending = 0;
static int cursorLastUpdatedX = 0;
static int cursorLastUpdatedY = 0;

// These are convenience functions which allow us to call OpenGL methods on Vec3d objects
inline void glVertex(const Vector3f &a)
{
    GLfloat arr[] = {a[0], a[1], a[2]};
    glVertex3fv(arr);
}

inline void glNormal(const Vector3f &a)
{
    GLfloat arr[] = {a[0], a[1], a[2]};
    glNormal3fv(arr);
}

void _cachedLoadFromFile(string filename)
{
    // load from file, create and return MeshData object
    if (mesh_data.loaded_from == filename)
    {
        return;
    }

    ifstream myfile("../" + filename);
    if (!myfile.is_open())
    {
        filesystem::path cwd = std::filesystem::current_path();
        cout << "Unable to open file. Current directory is " << cwd << endl;
        exit(1);
    }

    stringstream ss;
    char buffer[MAX_BUFFER_SIZE];
    while (myfile.getline(buffer, MAX_BUFFER_SIZE))
    {
        ss << buffer << endl;
    }
    myfile.close();
    ss.seekg(0);

    mesh_data.vecn.clear();
    mesh_data.vecv.clear();
    mesh_data.vecf.clear();

    string line;
    string type;
    Vector3f v;
    while (getline(ss, line))
    {
        istringstream line_ss(line);
        line_ss >> type;
        if (type == "v")
        {
            line_ss >> v[0] >> v[1] >> v[2];
            mesh_data.vecv.push_back(v);
        }
        else if (type == "vn")
        {
            line_ss >> v[0] >> v[1] >> v[2];
            mesh_data.vecn.push_back(v);
        }
        else if (type == "f")
        {
            vector<unsigned> vf;
            string raw;
            unsigned int val;
            // split on space
            while (line_ss >> raw)
            {
                // split on "/" and save as floats
                int first_sep = raw.find_first_of('/');
                int last_sep = raw.find_last_of('/');
                string first_chunk = raw.substr(0, first_sep);
                string second_chunk = raw.substr(first_sep + 1, last_sep - first_sep - 1);
                string third_chunk = raw.substr(last_sep + 1);
                vf.push_back(stof(first_chunk));
                vf.push_back(stof(second_chunk));
                vf.push_back(stof(third_chunk));
            }
            mesh_data.vecf.push_back(vf);
        }
    }
    // log the filename from which we loaded the data
    mesh_data.loaded_from = filename;
}

void displayTeapot()
{
    // if we don't clear previously loaded mesh data, it'll be immediately drawn again
    // by our drawMesh function
    mesh_data.vecn.clear();
    mesh_data.vecv.clear();
    mesh_data.vecf.clear();
    mesh_data.loaded_from = "";
    glutSolidTeapot(1.0);
}

void drawMesh()
{
    if (mesh_data.vecf.empty() || mesh_data.vecn.empty() || mesh_data.vecv.empty())
    {
        // cout << "No file data loaded. Draw function is falling back to default object";
        displayTeapot();
        return;
    }

    // we have the object already loaded into all of vecf, vecn, vecv
    glBegin(GL_TRIANGLES);
    for (vector v : mesh_data.vecf)
    {
        int a = v[0];
        int c = v[2];
        int d = v[3];
        int f = v[5];
        int g = v[6];
        int i = v[8];

        // the shape faces have vertices and normals indexed from 1, whereas C/C++ indexes from 0
        // each face value gives the vecn or vecf line number where we find the correct values to use
        // e.g.  f 5/0/2 3550/0/4 3549/0/3 means get the 5th vecv line, the 2nd vecn, the 3550th vecv
        // the 4th vecn, etc.
        glVertex3d(mesh_data.vecv[a - 1][0], mesh_data.vecv[a - 1][1], mesh_data.vecv[a - 1][2]);
        glNormal3d(mesh_data.vecn[c - 1][0], mesh_data.vecn[c - 1][1], mesh_data.vecn[c - 1][2]);
        glVertex3d(mesh_data.vecv[d - 1][0], mesh_data.vecv[d - 1][1], mesh_data.vecv[d - 1][2]);
        glNormal3d(mesh_data.vecn[f - 1][0], mesh_data.vecn[f - 1][1], mesh_data.vecn[f - 1][2]);
        glVertex3d(mesh_data.vecv[g - 1][0], mesh_data.vecv[g - 1][1], mesh_data.vecv[g - 1][2]);
        glNormal3d(mesh_data.vecn[i - 1][0], mesh_data.vecn[i - 1][1], mesh_data.vecn[i - 1][2]);
    }
    glEnd();
}

void loadObject(int shape_id, bool cycle)
{
    // load and draw a shape's mesh, based on the passed-in number.
    // if cycle is set to True, then wraparound out of range shape ids,
    // to instead serve shapes we do support.
    if (cycle)
    {
        shape_id = shape_id % 4;
    }

    current_shape_id = shape_id;
    cout << current_shape_id << endl;

    // note that the glutSolidTeapot won't be displayed over previously loaded meshes
    switch (shape_id)
    {
    case 0:
        _cachedLoadFromFile("garg.obj");
        break;
    case 1:
        _cachedLoadFromFile("sphere.obj");
        break;
    case 2:
        _cachedLoadFromFile("torus.obj");
        break;
    case 3:
        displayTeapot();
        break;
    default:
        displayTeapot();
        current_shape_id = 0;
        cerr << "Support for drawing mesh number " << shape_id << " not yet implemented" << endl;
        return;
    }
}

void loadObject(int argc, char **argv)
{
    // let us load shapes from the command line
    if (argc < 2)
    {
        loadObject(0, false);
        return;
    }
    string filename = argv[1];
    _cachedLoadFromFile(filename);
}

void updateIntermediateColor()
{
    // compute the color between the current color and the target color, constrained by
    // the maximum change allowed. For every property of those colors, if the difference
    // is sufficiently small, then use the target color's property value in the return color.

    assert(sizeof(current_color) / sizeof(float) == 4);
    assert(sizeof(target_color) / sizeof(float) == 4);

    // arbitrary values
    const float tolerance = 0.005f;
    const float max_change = 0.05f;

    for (int i = 0; i < 4; i++)
    {
        float diff = target_color[i] - current_color[i];

        if (abs(diff) > max_change)
        {
            if (diff < 0)
            {
                diff = -1 * max_change;
            }
            else
            {
                diff = max_change;
            }
        }

        // cout << current_color[i] << endl;
        // cout << target_color[i] << endl;
        // cout << "diff=" << diff << endl;

        intermediate_color[i] = current_color[i] + diff;
        if (abs(diff) <= tolerance)
        {
            intermediate_color[i] = target_color[i];
        }
    }
}

void updateCurrentColor(int value)
{
    const int rows = sizeof(DIFFCOLORS) / sizeof(DIFFCOLORS[0]);
    for (int i = 0; i < 4; i++)
    {
        target_color[i] = DIFFCOLORS[target_color_idx % rows][i];
    }

    updateIntermediateColor();
    current_color[0] = intermediate_color[0];
    current_color[1] = intermediate_color[1];
    current_color[2] = intermediate_color[2];
    current_color[3] = intermediate_color[3];

    for (int i = 0; i < 4; i++)
    {
        if (fabs(current_color[i] - target_color[i]) > 0.01f)
        {
            glutTimerFunc(200, updateCurrentColor, 0);
            break;
        }
    }
}

// This function is called whenever a "Normal" key press is received.
void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
    case 27:
        // escape key
        exit(0);
    case 'q':
        exit(0);
    case 'c':
        target_color_idx += 1;
        updateCurrentColor(0);
        break;
    case 'm':
        current_shape_id += 1;
        loadObject(current_shape_id, true);
        break;

    // handle positive rotation
    case 'r':
        angleRotationX = (angleRotationX + 5) % 360;
        angleRotationY = (angleRotationY + 5) % 360;
        angleRotationZ = (angleRotationZ + 5) % 360;
        // cout << angleRotationX << "," << angleRotationY << "," << angleRotationZ << endl;
        break;
    case 'x':
        angleRotationX = (angleRotationX + 5) % 360;
        break;
    case 'y':
        angleRotationY = (angleRotationY + 5) % 360;
        break;
    case 'z':
        angleRotationZ = (angleRotationZ + 5) % 360;
        break;

    // handle negative rotation
    case 'R':
        angleRotationX = angleRotationX <= -355 ? 0 : angleRotationX - 5;
        angleRotationY = angleRotationY <= -355 ? 0 : angleRotationY - 5;
        angleRotationZ = angleRotationZ <= -355 ? 0 : angleRotationZ - 5;
        // cout << angleRotationX << "," << angleRotationY << "," << angleRotationZ << endl;
        break;
    case 'X':
        angleRotationX = angleRotationX <= -355 ? 0 : angleRotationX - 5;
        break;
    case 'Y':
        angleRotationY = angleRotationY <= -355 ? 0 : angleRotationY - 5;
        break;
    case 'Z':
        angleRotationZ = angleRotationZ <= -355 ? 0 : angleRotationZ - 5;
        break;

    default:
        cout << "Unhandled key press " << key << "." << endl;
    }
}

// This function is called whenever a "Special" key press is received.
void specialFunc(int key, int x, int y)
{
    // change light position
    switch (key)
    {
    case GLUT_KEY_UP:
        upDownMultiplier += 1;
        break;
    case GLUT_KEY_DOWN:
        upDownMultiplier -= 1;
        break;
    case GLUT_KEY_LEFT:
        leftRightMultiplier -= 1;
        break;
    case GLUT_KEY_RIGHT:
        leftRightMultiplier += 1;
        break;
    }
}

GLfloat *getLight0Position()
{
    // left right is idx 0, up down is idx 1
    Lt0pos[0] = 0.5 * leftRightMultiplier;
    Lt0pos[1] = 0.5 * upDownMultiplier;
    return Lt0pos;
}

// This function is responsible for displaying the object.
void drawScene(void)
{
    // Clear the rendering window
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Rotate the image
    glMatrixMode(GL_MODELVIEW); // Current matrix affects objects positions
    glLoadIdentity();           // Initialize to the identity

    // Position the camera at [0, 0, lookZ], looking at [0, 0, 0],
    // with [0, 1, 0] as the up direction.
    gluLookAt(0.0, 0.0, lookZ,
              0.0, 0.0, 0.0,
              0.0, 1.0, 0.0);

    glRotatef(angleRotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(angleRotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(angleRotationZ, 0.0f, 0.0f, 1.0f);

    // Set material properties of object

    // Here we use the first color entry as the diffuse color
    // glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, diffColors[0]);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, current_color);

    // Define specular color and shininess
    GLfloat specColor[] = {1.0, 1.0, 1.0, 1.0};
    GLfloat shininess[] = {100.0};

    // Note that the specular color and shininess can stay constant
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);

    // Set light properties

    // Light color (RGBA)
    GLfloat Lt0diff[] = {1.0, 1.0, 1.0, 1.0};

    glLightfv(GL_LIGHT0, GL_DIFFUSE, Lt0diff);
    glLightfv(GL_LIGHT0, GL_POSITION, getLight0Position());

    // draw an already loaded object. We need to draw again because we called
    // glClear earlier
    drawMesh();

    // Dump the image to the screen.
    glutSwapBuffers();
}

// Initialize OpenGL's rendering modes
void initRendering()
{
    glEnable(GL_DEPTH_TEST); // Depth testing must be turned on
    glEnable(GL_LIGHTING);   // Enable lighting calculations
    glEnable(GL_LIGHT0);     // Turn on light #0.
}

// Called when the window is resized
// w, h - width and height of the window in pixels.
void reshapeFunc(int w, int h)
{
    // Always use the largest square viewport possible
    if (w > h)
    {
        glViewport((w - h) / 2, 0, h, h);
    }
    else
    {
        glViewport(0, (h - w) / 2, w, w);
    }

    // Set up a perspective view, with square aspect ratio
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // 50 degree fov, uniform aspect ratio, near = 1, far = 100
    gluPerspective(50.0, 1.0, 1.0, 100.0);

    currentWindowWidth = w;
    currentWindowHeight = h;
}

void delayedRedisplay(int value)
{
    glutPostRedisplay();
    // set target refresh rate
    glutTimerFunc(1000 / 60, delayedRedisplay, 0);
}

void onMouseMove(int x, int y)
{
    // use LMB to rotate the object. Ways it could be improved:
    // - update live (during drag), instead of at end of drag after a delay
    // - fix values for rotation so that a drag across the entire width or height rotates obj once
    if (leftMouseButtonActionPending)
    {
        int dx = cursorLastUpdatedX - x;
        int dy = cursorLastUpdatedY - y;
        // cout << dx; // << "," << dy;
        angleRotationY -= dx / (currentWindowWidth / 360);
        angleRotationX -= dy / (currentWindowWidth / 360);
        cout << angleRotationX << "," << angleRotationY << endl;
    }
    cursorLastUpdatedX = x;
    cursorLastUpdatedY = y;
    leftMouseButtonActionPending = 0;
    // cout << "Mouse position - X: " << x << ", Y: " << y << endl;
}

void onMouseClick(int button, int state, int x, int y)
{
    // cursorLastUpdatedX = x;
    // cursorLastUpdatedY = y;
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        leftMouseButtonActionPending = 1;
        // std::cout << "Left mouse button clicked at - X: " << x << ", Y: " << y << std::endl;
    }
}

// Function to handle mouse wheel scrolling
void onMouseWheel(int wheel, int direction, int x, int y)
{
    const float scrollAmount = 0.3f;
    if (wheel == 0)
    { // Scroll wheel event
        if (direction > 0)
        {
            // zoom in
            lookZ -= scrollAmount;
            // std::cout << "Mouse wheel scrolled up" << std::endl;
        }
        else
        {
            // zoom out
            lookZ += scrollAmount;
            // std::cout << "Mouse wheel scrolled down" << std::endl;
        }
    }
}

// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    loadObject(argc, argv);

    // We're going to animate it, so double buffer
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Initial parameters for window position and size
    glutInitWindowPosition(60, 60);
    glutInitWindowSize(currentWindowWidth, currentWindowHeight);
    glutCreateWindow("Assignment 0");

    // Initialize OpenGL parameters.
    initRendering();

    // Set up callback functions for key presses
    glutKeyboardFunc(keyboardFunc); // Handles "normal" ascii symbols
    glutSpecialFunc(specialFunc);   // Handles "special" keyboard keys

    // Set up the callback function for resizing windows
    glutReshapeFunc(reshapeFunc);

    // Call this whenever window needs redrawing
    glutDisplayFunc(drawScene);
    delayedRedisplay(0);

    // when mouse is moved and no button is pressed
    glutPassiveMotionFunc(onMouseMove);
    glutMouseFunc(onMouseClick);
    glutMouseWheelFunc(onMouseWheel);

    // Start the main loop.  glutMainLoop never returns.
    glutMainLoop();

    return 0; // This line is never reached.
}
// note to self...
// ls ../src/main.cpp | entr -p sh -c 'cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug && echo "<--YE-->" && cmake --build . --config Debug && echo "<--RUNNING PROGRAM-->" && ./MyProject'
