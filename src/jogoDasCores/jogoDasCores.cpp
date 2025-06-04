/* Jogo das Cores – com pontuação e reinício (tecla R)


Nomes: Leonardo Eugenio e João Victor Costa
 */

#include <iostream>
#include <string>
#include <assert.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm> 

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
using namespace glm;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);

GLuint createQuad();
int setupShader();
int eliminarSimilares(float tolerancia);

//janela e grid
const GLuint WIDTH     = 800, HEIGHT = 600;
const GLuint ROWS      = 6,   COLS   = 8;
const GLuint QUAD_W    = 100, QUAD_H = 100;
const float  dMax      = sqrt(3.0f);

const int BASE_POINTS      = 10;
const int PENALTY_PER_ATT  = 1;
const float TOLERANCE      = 0.2f;

struct Quad {
    vec3 position;
    vec3 dimensions;
    vec3 color;
    bool eliminated;
};
Quad grid[ROWS][COLS];

int iSelected = -1;   
int attempt   = 0;
int totalScore= 0;

const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
uniform mat4 projection;
uniform mat4 model;
void main(){
    gl_Position = projection * model * vec4(position, 1.0);
}
)";
const GLchar *fragmentShaderSource = R"(
#version 400
uniform vec4 inputColor;
out vec4 color;
void main(){
    color = inputColor;
}
)";

int main(){
    srand(time(nullptr));
    glfwInit();
    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Jogo das cores!#", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        cerr<<"Failed to initialize GLAD\n"; return -1;
    }
    glViewport(0,0,WIDTH,HEIGHT);

    GLuint shaderID = setupShader();
    GLuint VAO      = createQuad();
    glUseProgram(shaderID);
    GLint colorLoc = glGetUniformLocation(shaderID, "inputColor");
    mat4 projection = ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID,"projection"),1,GL_FALSE,value_ptr(projection));

    //grid de cores
    auto resetGame = [&](){
        attempt = 0;
        totalScore = 0;
        iSelected = -1;
        for(int i=0;i<ROWS;i++){
            for(int j=0;j<COLS;j++){
                float r = rand()%256/255.0f;
                float g = rand()%256/255.0f;
                float b = rand()%256/255.0f;
                grid[i][j] = {
                    vec3( QUAD_W/2 + j*QUAD_W, QUAD_H/2 + i*QUAD_H, 0.0f ),
                    vec3((float)QUAD_W,(float)QUAD_H,1.0f),
                    vec3(r,g,b),
                    false
                };
            }
        }
        cout<<"=== Novo jogo iniciado ===\n";
    };
    resetGame();

    while(!glfwWindowShouldClose(window)){
        glfwPollEvents();

        if(iSelected>-1){
            int removed = eliminarSimilares(TOLERANCE);
            attempt++;
            int raw = BASE_POINTS - (attempt-1)*PENALTY_PER_ATT;
            int ptsPer = std::max(raw, 1);
            totalScore += removed * ptsPer;
            cout<<"Tentativa "<<attempt
                <<" -> removidos "<<removed
                <<", pontos nesta tentativa: "<<(removed*ptsPer)
                <<", total: "<<totalScore<<endl;

            bool allGone=true;
            for(int i=0;i<ROWS && allGone;i++)
                for(int j=0;j<COLS;j++)
                    if(!grid[i][j].eliminated)
                        { allGone=false; break; }
            if(allGone){
                cout<<"=== FIM DE JOGO! Pontuação final: "<<totalScore
                    <<" | pressione R para reiniciar ===\n";
            }
        }

        // desenho
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        for(int i=0;i<ROWS;i++){
            for(int j=0;j<COLS;j++){
                if(!grid[i][j].eliminated){
                    mat4 model = mat4(1.0f);
                    model = translate(model, grid[i][j].position);
                    model = scale(model, grid[i][j].dimensions);
                    glUniformMatrix4fv(glGetUniformLocation(shaderID,"model"),1,GL_FALSE,value_ptr(model));
                    glUniform4f(colorLoc,
                                grid[i][j].color.r,
                                grid[i][j].color.g,
                                grid[i][j].color.b,
                                1.0f);
                    glDrawArrays(GL_TRIANGLE_STRIP, 0, 6);
                }
            }
        }
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// compila e linka shaders
int setupShader(){
    auto checkCompile = [&](GLuint id,const char* what){
        GLint ok; GLchar log[512];
        glGetShaderiv(id,GL_COMPILE_STATUS,&ok);
        if(!ok){
            glGetShaderInfoLog(id,512,nullptr,log);
            cout<<"ERROR::"<<what<<"_SHADER_COMPILATION\n"<<log<<"\n";
        }
    };
    auto checkLink = [&](GLuint prog){
        GLint ok; GLchar log[512];
        glGetProgramiv(prog,GL_LINK_STATUS,&ok);
        if(!ok){
            glGetProgramInfoLog(prog,512,nullptr,log);
            cout<<"ERROR::SHADER_PROGRAM_LINK\n"<<log<<"\n";
        }
    };

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs,1,&vertexShaderSource,nullptr);
    glCompileShader(vs); checkCompile(vs,"VERTEX");
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs,1,&fragmentShaderSource,nullptr);
    glCompileShader(fs); checkCompile(fs,"FRAGMENT");

    GLuint prog = glCreateProgram();
    glAttachShader(prog,vs);
    glAttachShader(prog,fs);
    glLinkProgram(prog); checkLink(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

GLuint createQuad(){
    GLfloat verts[] = {
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };
    GLuint VBO, VAO;
    glGenBuffers(1,&VBO);
    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER,sizeof(verts),verts,GL_STATIC_DRAW);
    glGenVertexArrays(1,&VAO);
    glBindVertexArray(VAO);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,3*sizeof(GLfloat),(void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER,0);
    glBindVertexArray(0);
    return VAO;
}


int eliminarSimilares(float tolerancia){
    int x = iSelected % COLS;
    int y = iSelected / COLS;
    vec3 C = grid[y][x].color;
    int count = 0;
    for(int i=0;i<ROWS;i++){
        for(int j=0;j<COLS;j++){
            if(!grid[i][j].eliminated){
                vec3 O = grid[i][j].color;
                float d = sqrt(pow(C.r-O.r,2) + pow(C.g-O.g,2) + pow(C.b-O.b,2));
                float norm = d / dMax;
                if(norm <= tolerancia){
                    grid[i][j].eliminated = true;
                    count++;
                }
            }
        }
    }
    iSelected = -1;
    return count;
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods){
    if(button==GLFW_MOUSE_BUTTON_LEFT && action==GLFW_PRESS){
        double mx,my;
        glfwGetCursorPos(window,&mx,&my);
        int col = mx / QUAD_W;
        int row = my / QUAD_H;
        if(row>=0 && row<ROWS && col>=0 && col<COLS
           && !grid[row][col].eliminated){
            iSelected = col + row*COLS;
        }
    }
}

// tecla ESC fecha, tecla R reinicia
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode){
    if(key==GLFW_KEY_ESCAPE && action==GLFW_PRESS)
        glfwSetWindowShouldClose(window,GL_TRUE);
    if(key==GLFW_KEY_R && action==GLFW_PRESS){
        // reinicia o jogo
        srand(time(nullptr));
        attempt = totalScore = 0;
        iSelected = -1;
        for(int i=0;i<ROWS;i++)
            for(int j=0;j<COLS;j++){
                float r = rand()%256/255.0f;
                float g = rand()%256/255.0f;
                float b = rand()%256/255.0f;
                grid[i][j].color = vec3(r,g,b);
                grid[i][j].eliminated = false;
            }
        cout<<"=== Jogo reiniciado (tecla R) ===\n";
    }
}
