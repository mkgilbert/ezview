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
        {{1, -1}, {0.99999, 0.99999}},
        {{1, 1},  {0.99999, 0}},
        {{-1, 1}, {0, 0}},
        {{-1, -1}, {0, 0.99999}}
};

// global variables representing translations
float rotation_angle_rad = 0;
float delta_x = 0;
float delta_y = 0;
float x_pos = 0;
float y_pos = 0;
float x_tilt = 0;
float y_tilt = 0;
float scale_factor = 1;
float shear_x = 0;
float shear_y = 0;
float translation_incr = 0.1;
float scale_incr = 0.05;
float shear_incr = 0.1;
float rotation_incr = 0.1;

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

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        rotation_angle_rad = 0;
        x_pos = 0;
        y_pos = 0;
        x_tilt = 0;
        y_tilt = 0;
        scale_factor = 1;
        shear_x = 0;
        shear_y = 0;
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        x_pos -= translation_incr;
    
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        x_pos += translation_incr;

    if (key == GLFW_KEY_W && action == GLFW_PRESS)
        y_pos += translation_incr;

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
        y_pos -= translation_incr;

    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        x_tilt -= translation_incr;

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        x_tilt += translation_incr;

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        y_tilt += translation_incr;

    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        y_tilt -= translation_incr;

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
        rotation_angle_rad -= rotation_incr;

    if (key == GLFW_KEY_E && action == GLFW_PRESS)
        rotation_angle_rad += rotation_incr;

    if (key == GLFW_KEY_EQUAL && action == GLFW_PRESS)
        scale_factor += scale_incr;

    if (key == GLFW_KEY_MINUS && action == GLFW_PRESS) {
        if (scale_factor < 0.00001)
            scale_factor = 0;
        else
            scale_factor -= scale_incr;
    }

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        shear_x += shear_incr;

    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        shear_x -= shear_incr;

    if (key == GLFW_KEY_V && action == GLFW_PRESS)
        shear_y += shear_incr;

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        shear_y -= shear_incr;
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
        fprintf(stderr, "Error: Unable to compile shader: %s\n", info);
        exit(1);
    }
}

void glLinkProgramOrDie(GLuint program) {
    GLint linked;
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        GLint infoLen = 0;
        char *info = malloc(infoLen+1);
        GLint done;
        glGetProgramInfoLog(program, infoLen, &done, info);
        fprintf(stderr, "Error: Unable to link program: %s\n", info);
    }
}
/************************************************
 * Main function - loads image and starts loop
 ************************************************/
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

    // file cleanup
    free(hdr);
    fclose(in_ptr);

    /***********************************
     * OpenGL setup
     ***********************************/
    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLuint mvp_location, vpos_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(image.width, image.height, "ezview", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);      // hopefully this will fix the tilted image result I'm getting

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
    glLinkProgramOrDie(program);

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLuint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLuint tex_location = glGetUniformLocation(program, "Texture");
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
        int width, height;
        mat4x4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        mat4x4 s = {
                {scale_factor, 0,            0, 0},
                {0,            scale_factor, 0, 0},
                {0,            0,            0, 0},
                {0,            0,            0, 1}
        };

        mat4x4 h = {
                {1,       shear_x, 0, 0},
                {shear_y, 1,       0, 0},
                {0,       0,       1, 0},
                {0,       0,       0, 1}
        };

        mat4x4 t = {
                {1,     0,     0, x_tilt},
                {0,     1,     0, y_tilt},
                {0,     0,     1, 0},
                {x_pos, y_pos, 0, 1}
        };

        mat4x4_rotate_Z(m, m, rotation_angle_rad);  // rotation
        mat4x4_mul(m, h, m);                        // shear
        mat4x4_mul(m, s, m);                        // scale
        mat4x4_mul(m, t, m);                        // translate
        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) m);
        glDrawArrays(GL_QUADS, 0, 4);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    free(image.pixmap);
    exit(EXIT_SUCCESS);
}
