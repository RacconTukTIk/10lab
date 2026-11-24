#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// В системном gl.h только OpenGL 1.1. Константы и точки входа шейдеров и VBO
// объявлены здесь и загружаются через wglGetProcAddress.

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif
#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif
#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif
#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif
#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif
#ifndef GL_ARRAY_BUFFER
#define GL_ARRAY_BUFFER 0x8892
#endif
#ifndef GL_STATIC_DRAW
#define GL_STATIC_DRAW 0x88E4
#endif

#define WGL_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB 0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB 0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001

using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC(WINAPI*)(HDC, HGLRC, const int*);

using PFNGLCREATESHADERPROC = unsigned int(APIENTRY*)(unsigned int type);
using PFNGLSHADERSOURCEPROC = void(APIENTRY*)(unsigned int shader, int count, const char* const* source, const int* length);
using PFNGLCOMPILESHADERPROC = void(APIENTRY*)(unsigned int shader);
using PFNGLGETSHADERIVPROC = void(APIENTRY*)(unsigned int shader, unsigned int pname, int* params);
using PFNGLGETSHADERINFOLOGPROC = void(APIENTRY*)(unsigned int shader, int maxLength, int* length, char* infoLog);
using PFNGLDELETESHADERPROC = void(APIENTRY*)(unsigned int shader);
using PFNGLCREATEPROGRAMPROC = unsigned int(APIENTRY*)();
using PFNGLATTACHSHADERPROC = void(APIENTRY*)(unsigned int program, unsigned int shader);
using PFNGLLINKPROGRAMPROC = void(APIENTRY*)(unsigned int program);
using PFNGLGETPROGRAMIVPROC = void(APIENTRY*)(unsigned int program, unsigned int pname, int* params);
using PFNGLGETPROGRAMINFOLOGPROC = void(APIENTRY*)(unsigned int program, int maxLength, int* length, char* infoLog);
using PFNGLUSEPROGRAMPROC = void(APIENTRY*)(unsigned int program);
using PFNGLDELETEPROGRAMPROC = void(APIENTRY*)(unsigned int program);
using PFNGLGETATTRIBLOCATIONPROC = int(APIENTRY*)(unsigned int program, const char* name);
using PFNGLGENBUFFERSPROC = void(APIENTRY*)(int n, unsigned int* buffers);
using PFNGLBINDBUFFERPROC = void(APIENTRY*)(unsigned int target, unsigned int buffer);
using PFNGLBUFFERDATAPROC = void(APIENTRY*)(unsigned int target, std::intptr_t size, const void* data, unsigned int usage);
using PFNGLDELETEBUFFERSPROC = void(APIENTRY*)(int n, const unsigned int* buffers);
using PFNGLENABLEVERTEXATTRIBARRAYPROC = void(APIENTRY*)(unsigned int index);
using PFNGLVERTEXATTRIBPOINTERPROC = void(APIENTRY*)(unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void* pointer);
using PFNGLGENVERTEXARRAYSPROC = void(APIENTRY*)(int n, unsigned int* arrays);
using PFNGLBINDVERTEXARRAYPROC = void(APIENTRY*)(unsigned int array);
using PFNGLDELETEVERTEXARRAYSPROC = void(APIENTRY*)(int n, const unsigned int* arrays);

PFNGLCREATESHADERPROC glCreateShader = nullptr;
PFNGLSHADERSOURCEPROC glShaderSource = nullptr;
PFNGLCOMPILESHADERPROC glCompileShader = nullptr;
PFNGLGETSHADERIVPROC glGetShaderiv = nullptr;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog = nullptr;
PFNGLDELETESHADERPROC glDeleteShader = nullptr;
PFNGLCREATEPROGRAMPROC glCreateProgram = nullptr;
PFNGLATTACHSHADERPROC glAttachShader = nullptr;
PFNGLLINKPROGRAMPROC glLinkProgram = nullptr;
PFNGLGETPROGRAMIVPROC glGetProgramiv = nullptr;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog = nullptr;
PFNGLUSEPROGRAMPROC glUseProgram = nullptr;
PFNGLDELETEPROGRAMPROC glDeleteProgram = nullptr;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation = nullptr;
PFNGLGENBUFFERSPROC glGenBuffers = nullptr;
PFNGLBINDBUFFERPROC glBindBuffer = nullptr;
PFNGLBUFFERDATAPROC glBufferData = nullptr;
PFNGLDELETEBUFFERSPROC glDeleteBuffers = nullptr;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray = nullptr;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer = nullptr;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays = nullptr;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray = nullptr;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays = nullptr;

HWND g_hwnd = nullptr;
HDC g_hdc = nullptr;
HGLRC g_glrc = nullptr;
int g_width = 800;
int g_height = 600;
bool g_running = true;

unsigned int g_program = 0;
unsigned int g_vbo = 0;
unsigned int g_vao = 0;
int g_posAttrib = -1;

const char* kVertexShader = R"GLSL(
#version 330 core
in vec2 aPos;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)GLSL";

const char* kFragmentShader = R"GLSL(
#version 330 core
out vec4 fragColor;
void main() {
    fragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
)GLSL";

const float kTriangle[] = {
     0.0f,  0.5f,
    -0.5f, -0.5f,
     0.5f, -0.5f,
};

void ShaderLog(unsigned int shader)
{
    int infologLen = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
        int charsWritten = 0;
        std::vector<char> infoLog(infologLen);
        glGetShaderInfoLog(shader, infologLen, &charsWritten, infoLog.data());
        std::cout << "InfoLog: " << infoLog.data() << std::endl;
    }
}

void ProgramLog(unsigned int program)
{
    int infologLen = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLen);
    if (infologLen > 1)
    {
        int charsWritten = 0;
        std::vector<char> infoLog(infologLen);
        glGetProgramInfoLog(program, infologLen, &charsWritten, infoLog.data());
        std::cout << "InfoLog: " << infoLog.data() << std::endl;
    }
}

bool hadGlError(const char* step)
{
    bool bad = false;
    GLenum err = glGetError();
    while (err != GL_NO_ERROR)
    {
        bad = true;
        std::cout << step << ": OpenGL error 0x" << std::hex << err << std::dec << std::endl;
        err = glGetError();
    }
    return bad;
}

template <typename T>
T loadProc(const char* name)
{
    PROC proc = wglGetProcAddress(name);
    const auto value = reinterpret_cast<std::uintptr_t>(proc);
    if (proc == nullptr || value == 0x1 || value == 0x2 || value == 0x3 || value == static_cast<std::uintptr_t>(-1))
    {
        proc = GetProcAddress(GetModuleHandleA("opengl32.dll"), name);
    }
    if (proc == nullptr)
    {
        std::cout << "Не удалось загрузить функцию " << name << std::endl;
    }
    return reinterpret_cast<T>(proc);
}

bool loadGlFunctions()
{
    glCreateShader = loadProc<PFNGLCREATESHADERPROC>("glCreateShader");
    glShaderSource = loadProc<PFNGLSHADERSOURCEPROC>("glShaderSource");
    glCompileShader = loadProc<PFNGLCOMPILESHADERPROC>("glCompileShader");
    glGetShaderiv = loadProc<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    glGetShaderInfoLog = loadProc<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    glDeleteShader = loadProc<PFNGLDELETESHADERPROC>("glDeleteShader");
    glCreateProgram = loadProc<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    glAttachShader = loadProc<PFNGLATTACHSHADERPROC>("glAttachShader");
    glLinkProgram = loadProc<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    glGetProgramiv = loadProc<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    glGetProgramInfoLog = loadProc<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    glUseProgram = loadProc<PFNGLUSEPROGRAMPROC>("glUseProgram");
    glDeleteProgram = loadProc<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    glGetAttribLocation = loadProc<PFNGLGETATTRIBLOCATIONPROC>("glGetAttribLocation");
    glGenBuffers = loadProc<PFNGLGENBUFFERSPROC>("glGenBuffers");
    glBindBuffer = loadProc<PFNGLBINDBUFFERPROC>("glBindBuffer");
    glBufferData = loadProc<PFNGLBUFFERDATAPROC>("glBufferData");
    glDeleteBuffers = loadProc<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    glEnableVertexAttribArray = loadProc<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");
    glVertexAttribPointer = loadProc<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    glGenVertexArrays = loadProc<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");
    glBindVertexArray = loadProc<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");
    glDeleteVertexArrays = loadProc<PFNGLDELETEVERTEXARRAYSPROC>("glDeleteVertexArrays");

    return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && glGetShaderInfoLog
        && glDeleteShader && glCreateProgram && glAttachShader && glLinkProgram && glGetProgramiv
        && glGetProgramInfoLog && glUseProgram && glDeleteProgram && glGetAttribLocation
        && glGenBuffers && glBindBuffer && glBufferData && glDeleteBuffers
        && glEnableVertexAttribArray && glVertexAttribPointer
        && glGenVertexArrays && glBindVertexArray && glDeleteVertexArrays;
}

unsigned int compileShader(unsigned int type, const char* source)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    ShaderLog(shader);

    int status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        std::cout << "Шейдер не скомпилировался" << std::endl;
        glDeleteShader(shader);
        return 0;
    }
    if (hadGlError(type == GL_VERTEX_SHADER ? "glCompileShader(vertex)" : "glCompileShader(fragment)"))
    {
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool initShadersAndBuffers()
{
    unsigned int vertex = compileShader(GL_VERTEX_SHADER, kVertexShader);
    unsigned int fragment = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (vertex == 0 || fragment == 0)
    {
        if (vertex != 0)
        {
            glDeleteShader(vertex);
        }
        if (fragment != 0)
        {
            glDeleteShader(fragment);
        }
        return false;
    }

    g_program = glCreateProgram();
    glAttachShader(g_program, vertex);
    glAttachShader(g_program, fragment);
    glLinkProgram(g_program);
    ProgramLog(g_program);

    int linked = 0;
    glGetProgramiv(g_program, GL_LINK_STATUS, &linked);
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    if (linked != GL_TRUE || hadGlError("glLinkProgram"))
    {
        std::cout << "Шейдерная программа не слинковалась" << std::endl;
        return false;
    }

    g_posAttrib = glGetAttribLocation(g_program, "aPos");
    if (g_posAttrib == -1 || hadGlError("glGetAttribLocation"))
    {
        std::cout << "glGetAttribLocation вернул -1: аттрибут aPos не найден" << std::endl;
        return false;
    }

    glGenVertexArrays(1, &g_vao);
    glBindVertexArray(g_vao);

    glGenBuffers(1, &g_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kTriangle), kTriangle, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    if (hadGlError("glBufferData"))
    {
        return false;
    }
    return true;
}

void drawScene()
{
    glViewport(0, 0, g_width, g_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(g_program);
    glBindVertexArray(g_vao);
    glEnableVertexAttribArray(static_cast<unsigned int>(g_posAttrib));
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glVertexAttribPointer(static_cast<unsigned int>(g_posAttrib), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    hadGlError("glDrawArrays");
}

void cleanupGl()
{
    if (g_vbo != 0 && glDeleteBuffers != nullptr)
    {
        glDeleteBuffers(1, &g_vbo);
        g_vbo = 0;
    }
    if (g_vao != 0 && glDeleteVertexArrays != nullptr)
    {
        glDeleteVertexArrays(1, &g_vao);
        g_vao = 0;
    }
    if (g_program != 0 && glDeleteProgram != nullptr)
    {
        glDeleteProgram(g_program);
        g_program = 0;
    }
}

void releaseGl()
{
    cleanupGl();
    if (g_glrc != nullptr)
    {
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(g_glrc);
        g_glrc = nullptr;
    }
    if (g_hdc != nullptr && g_hwnd != nullptr)
    {
        ReleaseDC(g_hwnd, g_hdc);
        g_hdc = nullptr;
    }
}

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SIZE:
        g_width = LOWORD(lParam);
        g_height = HIWORD(lParam);
        if (g_width < 1)
        {
            g_width = 1;
        }
        if (g_height < 1)
        {
            g_height = 1;
        }
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        releaseGl();
        g_running = false;
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

bool createGlWindow(HINSTANCE instance)
{
    WNDCLASSW wc = {};
    wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = windowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"GreenTriangleWindow";
    wc.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    if (RegisterClassW(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
    {
        std::cout << "RegisterClassW не удался" << std::endl;
        return false;
    }

    RECT rect = {0, 0, g_width, g_height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"Зелёный треугольник",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        instance,
        nullptr);
    if (g_hwnd == nullptr)
    {
        std::cout << "CreateWindowExW не удался" << std::endl;
        return false;
    }

    g_hdc = GetDC(g_hwnd);
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.iLayerType = PFD_MAIN_PLANE;

    const int format = ChoosePixelFormat(g_hdc, &pfd);
    if (format == 0 || SetPixelFormat(g_hdc, format, &pfd) == FALSE)
    {
        std::cout << "Не удалось выбрать формат пикселей" << std::endl;
        return false;
    }

    HGLRC legacy = wglCreateContext(g_hdc);
    if (legacy == nullptr || wglMakeCurrent(g_hdc, legacy) == FALSE)
    {
        std::cout << "wglCreateContext не удался" << std::endl;
        return false;
    }

    auto createAttribs = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
        wglGetProcAddress("wglCreateContextAttribsARB"));
    if (createAttribs != nullptr)
    {
        const int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0};
        HGLRC modern = createAttribs(g_hdc, nullptr, attribs);
        if (modern != nullptr)
        {
            wglMakeCurrent(g_hdc, modern);
            wglDeleteContext(legacy);
            g_glrc = modern;
        }
    }
    if (g_glrc == nullptr)
    {
        g_glrc = legacy;
    }

    const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
    std::cout << "OpenGL " << (version != nullptr ? version : "?")
              << " / " << (renderer != nullptr ? renderer : "?") << std::endl;

    if (!loadGlFunctions())
    {
        return false;
    }
    while (glGetError() != GL_NO_ERROR)
    {
    }
    return initShadersAndBuffers();
}

void destroyGlWindow()
{
    if (g_hwnd != nullptr && IsWindow(g_hwnd))
    {
        HWND hwnd = g_hwnd;
        DestroyWindow(hwnd);
    }
    releaseGl();
    g_hwnd = nullptr;
}

bool centerPixelIsGreen()
{
    unsigned char pixel[4] = {};
    unsigned char corner[4] = {};
    glReadPixels(g_width / 2, g_height / 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    glReadPixels(2, 2, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, corner);
    std::cout << "Центр RGBA: " << static_cast<int>(pixel[0]) << " " << static_cast<int>(pixel[1])
              << " " << static_cast<int>(pixel[2]) << " " << static_cast<int>(pixel[3]) << std::endl;
    std::cout << "Угол RGBA: " << static_cast<int>(corner[0]) << " " << static_cast<int>(corner[1])
              << " " << static_cast<int>(corner[2]) << " " << static_cast<int>(corner[3]) << std::endl;

    const bool green = pixel[0] < 40 && pixel[1] > 200 && pixel[2] < 40;
    const bool black = corner[0] < 20 && corner[1] < 20 && corner[2] < 20;
    return green && black && !hadGlError("glReadPixels");
}

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    const bool check = argc > 1 && std::string(argv[1]) == "--check";
    if (check)
    {
        g_width = 200;
        g_height = 200;
    }

    if (!createGlWindow(GetModuleHandleW(nullptr)))
    {
        destroyGlWindow();
        return 1;
    }

    RECT client = {};
    GetClientRect(g_hwnd, &client);
    g_width = client.right - client.left;
    g_height = client.bottom - client.top;

    int exitCode = 0;
    if (check)
    {
        drawScene();
        if (!centerPixelIsGreen())
        {
            std::cout << "Проверка не пройдена" << std::endl;
            exitCode = 1;
        }
        else
        {
            std::cout << "Проверка пройдена: зелёный треугольник на чёрном фоне" << std::endl;
        }
        destroyGlWindow();
        return exitCode;
    }

    std::cout << "Esc закрывает окно" << std::endl;
    MSG msg = {};
    while (g_running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                g_running = false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if (!g_running)
        {
            break;
        }
        drawScene();
        SwapBuffers(g_hdc);
    }

    destroyGlWindow();
    return exitCode;
}
