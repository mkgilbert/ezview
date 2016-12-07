#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "linmath.h"
#include "ppmrw.h"

typedef struct {
    float Position[2];
    float TexCoord[2];
} Vertex;

// (-1, 1)  (1, 1)
// (-1, -1) (1, -1)

Vertex vertexes[] = {
        {{1, -1}, {0, 0.99999}},
        {{1, 1},  {0.99999, 0.99999}},
        {{-1, 1}, {0.99999, 0}},
        {{-1, -1}, {0, 0}}
};

static const char* vertex_shader_text =
        "uniform mat4 MVP;\n"
                "attribute vec2 TexCoordIn;\n"
                "attribute vec2 vPos;\n"
                "varying vec2 TexCoordOut;\n"
                "void main()\n"
                "{\n"
                "    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
                "    TexCoordOut = TexCoordIn;\n"
                "}\n";

static const char* fragment_shader_text =
        "varying vec2 TexCoordOut;\n"
                "uniform sampler2D Texture;\n"
                "void main()\n"
                "{\n"
                "    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
                "}\n";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void glCompileShaderOrDie(GLuint shader) {
    GLint compiled;
    glCompileShader(shader);
    glGetShaderiv(shader,
                  GL_COMPILE_STATUS,
                  &compiled);
    if (!compiled) {
        GLint infoLen = 0;
        glGetShaderiv(shader,
                      GL_INFO_LOG_LENGTH,
                      &infoLen);
        char* info = malloc(infoLen+1);
        GLint done;
        glGetShaderInfoLog(shader, infoLen, &done, info);
        printf("Unable to compile shader: %s\n", info);
        exit(1);
    }
}

// 4 x 4 image..
/*unsigned char image[] = {
        255, 0, 0, 255,
        255, 0, 0, 255,
        255, 0, 0, 255,
        255, 0, 0, 255,

        0, 255, 0, 255,
        0, 255, 0, 255,
        0, 255, 0, 255,
        0, 255, 0, 255,

        0, 0, 255, 255,
        0, 0, 255, 255,
        0, 0, 255, 255,
        0, 0, 255, 255,

        255, 0, 255, 255,
        255, 0, 255, 255,
        255, 0, 255, 255,
        255, 0, 255, 255
};*/

int main(int argc, char *argv[]) {

    FILE *in_ptr;
    int ret_val;
    char c;

    in_ptr = fopen(argv[1], "rb");  // input file pointer

    // error check the file pointers
    if (in_ptr == NULL) {
        fprintf(stderr, "Error: main: Input file can't be opened\n");
        return 1;
    }
    // allocate space for header information
    header *hdr = (header *)malloc(sizeof(header));


    /******************************//**
     * read data from input file
     *********************************/

    // read header of input file
    ret_val = read_header(in_ptr, hdr);

    if (ret_val < 0) {
        fprintf(stderr, "Error: main: Problem reading header\n");
        return 1;
    }

    // store the file type of the origin file so we know what we're converting from
    int origin_file_type = hdr->file_type;

    // create img struct to store relevant image info
    image image;
    image.width = hdr->width;
    image.height = hdr->height;
    image.max_color_val = hdr->max_color_val;
    image.pixmap = malloc(sizeof(RGBPixel) * image.width * image.height);

    // read image data (pixels)
    if (origin_file_type == 3)
        ret_val = read_p3_data(in_ptr, &image);
    else
        ret_val = read_p6_data(in_ptr, &image);

    if (ret_val < 0) {
        fprintf(stderr, "Error: main: Problem reading image data\n");
        return -1;
    }

    /* OpenGL setup */
    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(image.width, image.height, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex),
                          (void*) (sizeof(float) * 2));

    //int image_width = 4;
    //int image_height = 4;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width, image.height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, image.pixmap);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    free(image.pixmap);
    free(hdr);
    fclose(in_ptr);
    exit(EXIT_SUCCESS);
}
