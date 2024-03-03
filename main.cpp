/**
* Author: Andy Ng
* Assignment: Pong Clone
* Date due: 2024-03-02, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/


#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course
#include "stb_image.h"

//window dimensions
const int WINDOW_WIDTH  = 640,
          WINDOW_HEIGHT = 480;

//our viewport or our camera's position and dimensions
const int VIEWPORT_X      = 0,
          VIEWPORT_Y      = 0,
          VIEWPORT_WIDTH  = WINDOW_WIDTH,
          VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
           F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const char PIKACHU_SPRITE[] = "assets/pikachu.png",
            CHARIZARD_SPRITE[] = "assets/charizard.png";


SDL_Window* displayWindow;
bool gameIsRunning = true;


bool moveUp = true;


ShaderProgram program;
GLuint p1TextureID;
GLuint p2TextureID;
GLuint ballTextureID;


glm::mat4 viewMatrix, p1Matrix, p2Matrix, ballMatrix, projectionMatrix;


// ball
glm::vec3 ballSize = glm::vec3(0.2f, 0.2f, 1.0f);
glm::vec3 ballPosition = glm::vec3(0, 0, 0);
glm::vec3 ballMovement = glm::vec3(0, 0, 0);
float ballWidth = ballSize.x;
float ballHeight = ballSize.y;
float ballSpeed = 2.5f;

// player 1
glm::vec3 p1Position = glm::vec3(-4.5, 0, 0);
glm::vec3 p1Movement = glm::vec3(0, 0, 0);

// player 2
glm::vec3 p2Position = glm::vec3(4.5, 0, 0);
glm::vec3 p2Movement = glm::vec3(0, 0, 0);
float yMove;

// player size and speed
glm::vec3 playerSize = glm::vec3(0.7f, 2.0f, 1.0f);
float playerHeight = playerSize.y;
float playerWidth = playerSize.x;
float playerSpeed = 3.5f;


float previousTicks = 0.0f;


GLuint LoadTexture(const char* filePath) {
    //Step 1//
    int width, height, num_of_component;
    unsigned char* image = stbi_load(filePath, &width, &height, &num_of_component, STBI_rgb_alpha);

    if (image == NULL) {
        std::cout << "Unable to load image. Make sure the path is correct\n";
        assert(false);
    }
    //Step 1//

    
    //Step2//
    GLuint textureID; // instatitation of
    glGenTextures(1, &textureID); // number of textures = 1
    glBindTexture(GL_TEXTURE_2D, textureID); //GL_texture is binding textureID to a 2D texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    //sends the image to the graphics card by binding the texture ID with our raw image data


    //2 lines that set your desired mode in the case of magnfication and minification
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    
    stbi_image_free(image);
    return textureID;
}

void Initialize() {
    SDL_Init(SDL_INIT_VIDEO);
    displayWindow = SDL_CreateWindow("Pong Clone!", 
                                     SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                     WINDOW_WIDTH, WINDOW_HEIGHT,
                                     SDL_WINDOW_OPENGL);
    
    SDL_GLContext context = SDL_GL_CreateContext(displayWindow);
    SDL_GL_MakeCurrent(displayWindow, context);

#ifdef _WINDOWS
    glewInit();
#endif
    //adding color to our window
    glViewport(VIEWPORT_X, VIEWPORT_Y, WINDOW_WIDTH, WINDOW_HEIGHT);
    program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    //specify color of the background
    glClearColor(0.17f, 0.13f, 0.07f, 1.0f);

   

    viewMatrix = glm::mat4(1.0f);
    p1Matrix = glm::mat4(1.0f);
    p2Matrix = glm::mat4(1.0f);
    ballMatrix = glm::mat4(1.0f);
    projectionMatrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);


    program.set_projection_matrix(projectionMatrix);
    program.set_view_matrix(viewMatrix);

    glUseProgram(program.get_program_id());

    p1TextureID = LoadTexture(PIKACHU_SPRITE);
    p2TextureID = LoadTexture(CHARIZARD_SPRITE);


    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   

}

bool bounceOffTop(glm::vec3 position, float heightDifference) { // ball position,ballheight
    float heightDiff = heightDifference / 2.0f;
    if (position.y + heightDiff >= 3.75f) {
        return true;
    }
    return false;
}

bool bounceOffBot(glm::vec3 position, float heightDifference) {
    float heightDiff = heightDifference / 2.0f;
    if (position.y - heightDiff <= -3.75f) {
        return true;
    }
    return false;
}

// box to box detection
bool collision() {
  // player1 collision
    float xDist1 = fabs(p1Position.x - ballPosition.x) - ((ballWidth + playerWidth) / 2.0f);
    float yDist1 = fabs(p1Position.y - ballPosition.y) - ((ballHeight  + playerHeight) / 2.0f);

// player2 collision
    float xDist2 = fabs(p2Position.x - ballPosition.x) - ((ballWidth + playerWidth) / 2.0f);
    float yDist2 = fabs(p2Position.y - ballPosition.y) - ((ballHeight + playerHeight) / 2.0f);
    
    // colliding
    if ((xDist1 < 0.0f && yDist1 < 0.0f ) || (xDist2 < 0.0f && yDist2 < 0.0f)) {
        return true;
    }
    else {
        return false;
    }
}

void onePlayerMode() {
    if (moveUp) {
        p2Movement.y = 1.0f;
    }
    else {
        p2Movement.y = -1.0f;
    }
    if (p2Position.y >= 3.75f) {
        moveUp = false;
    }
    else if (p2Position.y < -3.75f) {
        moveUp = true;
    }
    else {
        p2Movement.y = 0.0f;
    }
    
}

void ProcessInput() {

    p1Movement = glm::vec3(0.0f, 0.0f, 0.0f);
    p2Movement = glm::vec3(0.0f, 0.0f, 0.0f);

       
    //reset movement
//    if (!onePlayer){
//        p2Movement = glm::vec3(0.0f, 0.0f, 0.0f);
//    }
//    else{
//        p2Movement = glm::vec3(0.0f, yMove, 0.0f);
//    
//    }
//    p1Movement = glm::vec3(0.0f, 0.0f, 0.0f);
    
    
//
    
   

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                
            case SDL_WINDOWEVENT_CLOSE:
                gameIsRunning = false;
                break;
                
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym){
                    case SDLK_t:
                        onePlayerMode();
                        
                }
                        
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    // press space to start game
    if (key_state[SDL_SCANCODE_SPACE]) {
        int slope = rand();
        ballMovement.x = slope;
        ballMovement.y = slope;
    }
  

    if (key_state[SDL_SCANCODE_W] && !bounceOffTop(p1Position, playerHeight)) {
        p1Movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_S] && !bounceOffBot(p1Position, playerHeight)) {
        p1Movement.y = -1.0f;
    }

    if (key_state[SDL_SCANCODE_UP] && !bounceOffTop(p2Position, playerHeight)) {
        p2Movement.y = 1.0f;
    }
    else if (key_state[SDL_SCANCODE_DOWN] && !bounceOffBot(p2Position, playerHeight)) {
        p2Movement.y = -1.0f;
    }
    
    if (key_state[SDL_SCANCODE_T] && !bounceOffBot(p2Position, playerHeight)) {
        onePlayerMode();
    }
    

  
//    while (key_state[SDL_SCANCODE_T] && !bounceOffBot(p2Position, playerHeight) && !bounceOffTop(p2Position, playerHeight)) {
//           onePlayerMode();
//       }
    
    if (glm::length(ballMovement) > 1.0f) {
        ballMovement = glm::normalize(ballMovement);
    }
}

void Update() {
    //ticks is amt of time(in ms) for one frame
    //basically all the deltaTime added together
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float deltaTime = ticks - previousTicks;
    previousTicks = ticks;

    if (ballPosition.x > 5.0f || ballPosition.x < -5.0f) {
        gameIsRunning = false;
    }
    
    if (bounceOffTop(ballPosition, ballHeight) || bounceOffBot(ballPosition, ballHeight)) {
        ballMovement.y *= -1.0f;
    }
    
    if (bounceOffTop(p2Position, playerHeight) || bounceOffBot(p2Position, playerHeight)) {
        yMove *= -1.0f;
    }
    
    if (collision()){
        ballMovement.x *= -1.0f;
    }

    p1Matrix = glm::mat4(1.0f);
    p1Position += p1Movement * playerSpeed * deltaTime;
    p1Matrix = glm::translate(p1Matrix, p1Position);
    p1Matrix = glm::scale(p1Matrix, playerSize);

    p2Matrix = glm::mat4(1.0f);
    p2Position += p2Movement * playerSpeed * deltaTime;
    p2Matrix = glm::translate(p2Matrix, p2Position);
    p2Matrix = glm::scale(p2Matrix, playerSize);

    ballMatrix = glm::mat4(1.0f);
    ballPosition += ballMovement * ballSpeed * deltaTime;
    ballMatrix = glm::translate(ballMatrix, ballPosition);
    ballMatrix = glm::scale(ballMatrix, ballSize);
}


void Render() {
    //clean back buffer and assign new color to it
    glClear(GL_COLOR_BUFFER_BIT);
    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float texCoords[] = { 0.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 0.0 };
    glVertexAttribPointer(program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.get_position_attribute());
    glVertexAttribPointer(program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(program.get_tex_coordinate_attribute());

    program.set_model_matrix(p1Matrix);
    glBindTexture(GL_TEXTURE_2D, p1TextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    program.set_model_matrix(p2Matrix);
    glBindTexture(GL_TEXTURE_2D, p2TextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    program.set_model_matrix(ballMatrix);
    glBindTexture(GL_TEXTURE_2D, ballTextureID);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program.get_position_attribute());
    glDisableVertexAttribArray(program.get_tex_coordinate_attribute());

    //swap windoes to display all the colors since only front window is being displayed
    SDL_GL_SwapWindow(displayWindow);
}

void Shutdown() {
    SDL_Quit();
}

int main(int argc, char* argv[]) {
    Initialize();
    
    while (gameIsRunning) {
        ProcessInput();
        Update();
        Render();
    }

    Shutdown();
    return 0;
}
