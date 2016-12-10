#define GLFW_DLL 1

#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2.h>
#include <GLFW/glfw3.h>


#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <fcntl.h>

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

typedef struct Pixel{
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} Pixel;

int width, height, depth;

Vertex vertexes[] = {
  {{1, -1}, {0.99999, 0.99999}},
  {{1, 1},  {0.99999, 0}},
  {{-1, 1}, {0, 0}},
  {{-1, 1}, {0, 0}},
  {{-1, -1}, {0, 0.99999}},
  {{1, -1}, {0.99999, 0.99999}}
};

static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec2 TexCoordIn;\n"
"attribute vec2 vPos;\n"
"varying lowp vec2 TexCoordOut;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}\n";

static const char* fragment_shader_text =
"varying lowp vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}\n";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

float rotate = 0;
int pan_left = 0;
int pan_right = 0;
int scale = 1;
int shear_x = 0;
int shear_y = 0;

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        // SCALE IN
        scale += 1;
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        // SCALE OUT
        scale -= 1;
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        // ROTATE LEFT
        rotate -= 90*3.1415/180;
    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        // ROTATE RIGHT
        rotate += 90*3.1415/180;
    if (key == GLFW_KEY_X && action == GLFW_PRESS)
        // SHEAR X
        shear_x += 1;
    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
        // SHEAR Y
        shear_y += 1;
    if (key == GLFW_KEY_A && action == GLFW_PRESS)
        // PAN LEFT
        pan_left += 1;
    if (key == GLFW_KEY_D && action == GLFW_PRESS)
        // PAN RIGHT
        pan_right += 1;
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

FILE* escape_comments(FILE *input, int c){
	if(c == '#'){
		printf("Traversing comments...\n");
		while(c != '\n'){
			c = fgetc(input);
		}
		printf("Escaped comments\n");
		c = fgetc(input);
		input = escape_comments(input, c);
	}
	else{
		fseek(input, -1, SEEK_CUR);
	}
	return input;
}

// reads in p3 file type
void read_p3(FILE* input){
	printf("Reading P3 file\n");
  int c;
	c = fgetc(input);
	if(c != '\n'){
		fprintf(stderr, "Incorrect .PPM file type\n");
		exit(1);
	}
	c = fgetc(input);
  input = escape_comments(input, c);
	fscanf(input, "%d %d\n%d\n", &width, &height, &depth);
	if(depth > 255){
		printf("Error: Image not an 8 bit channel\n");
		exit(1);
	}
  Pixel new;
	Pixel *image = malloc(sizeof(Pixel)*width*height);
	int count = 0;
	while(!feof(input)){
		fscanf(input, "%hhu ", &new.red);
		fscanf(input, "%hhu ", &new.green);
		fscanf(input, "%hhu ", &new.blue);
		image[count] = new;
		count++;
	}
	shading(image);
}

// reads in p6 file type
void read_p6(FILE* input){
	printf("Reading p6\n");
	int c;
		c = fgetc(input);
		if(c != '\n'){
			fprintf(stderr, "Incorrect .PPM file type\n");
			exit(1);
		}
		c = fgetc(input);
		input = escape_comments(input, c);
		fscanf(input, "\n%d %d\n%d\n", &width, &height, &depth);
		if(depth > 255){
			printf("Error: Image not an 8 bit channel\n");
			exit(1);
		}
		Pixel new;
		Pixel *image = malloc(sizeof(Pixel)*width*height);
		int count = 0;
		while(!feof(input)){
			fread(&new.red, 1, 1, input);
			fread(&new.green, 1, 1, input);
			fread(&new.blue, 1, 1, input);
			image[count] = new;
			count++;
		}
		shading(image);
}

int main(int argc, char **argv)
{
    // reading file into image
    FILE* input_fp;
    int width, height, depth;
    char c;

    if (argc != 2){
      printf("Incorrect number of arguments\n");
      return 1;
    }

    input_fp = fopen(argv[1], "rb");
    if(input_fp == NULL){
		  printf("Error: File was not found\n");
		  return 1;
	  }

  	// checking what the current file type is
  	c = fgetc(input_fp);
  	if(c != 80){
  		printf("Incorrect file type: Expected .PPM file\n");
  		return 1;
  	}

  	//getting magic number
  	c = fgetc(input_fp);
  	if(c != 51 && c != 54){
  		printf("Incorrect file type: Expected P3 or P6 file\n");
  		return 1;
  	}

    if(c == 51){
  		read_p3(input_fp);
  	}
  	if(c == 54){
  		read_p6(input_fp);
  	}

    fclose(input_fp);

}

int shading(Pixel *image){

		GLFWwindow* window;
		GLuint vertex_buffer, vertex_shader, fragment_shader, program;
		GLint mvp_location, vpos_location, vcol_location;

		glfwSetErrorCallback(error_callback);


    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);


    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
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

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
		 GL_UNSIGNED_BYTE, image);

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
        // rotation, taking in global variable rotate
        mat4x4_rotate_Z(m, m, rotate);
        mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
