// Example program:
// Using SDL2 to create an application window

#include <SDL2/SDL.h> //main library
#include <SDL2/SDL_image.h> //textures
#include <SDL2/SDL_mixer.h> //sound
#include <SDL2/SDL_ttf.h> //fonts
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>


#define SQUARE_UNIT 20
#define ENEMY_COLUMN 10
#define ENEMY_ROW 5
#define ENEMY_SPEED 30

#define MAX_ENEMIES ENEMY_ROW*ENEMY_COLUMN
#define MAX_SHOTS 20
#define MAX_ENEMY_SHOTS 50
#define MAX_EXPLOSIONS MAX_ENEMY_SHOTS+MAX_SHOTS+MAX_ENEMIES+1
#define END_DELAY 200 //ms

SDL_RendererFlip flip = 0;
const int SHOT_SIZE = SQUARE_UNIT/5;
const int WINDOW_WIDTH = 640;
const int WINDOW_HEIGHT = 480;
const char* WINDOW_TITLE = "An SDL2 window";
const int SHIP_HEIGHT = SQUARE_UNIT*4/3;
//int SHIP_WIDTH = SHIP_HEIGHT*3/4;
const int SHIP_WIDTH = SQUARE_UNIT;
const int shoot_delay = 400;
const int enemy_shoot_delay = 100;

enum direction{ UP = 1, DOWN = -1, LEFT, RIGHT, NONE};

typedef struct position{
  double x;
  double y;
  SDL_Rect pos_text;
}position;

struct shot{
  int active;
  position pos; 
  enum direction dir;
  int speed;
};


struct enemy{
  int active;
  int speed;
  position pos;
};

struct explosion{
  double start_time;
  SDL_Rect pos;
};

SDL_Window *create_window(void);
int create_renderer(void);
SDL_Texture *create_ttftext(char *text, int size);
void init_all(void);
void init_positions(void);
int load_music(void);
void close_music(void);
void render_end(SDL_Texture *font_text);

void render(void);
int run_game(void);

void spawn_enemy(int x, int y, int i);
void spawn_enemies(void);

void validate_ship_pos(void);
int shoot(void);
int enemy_shoot(int index);
int get_enemy_shot(void);
int all_enemies_alive(void);
void move_enemies(double delta);

void shot_collisions(void);
int ship_collisions(void);
int check_collision(const SDL_Rect rect1, const SDL_Rect rect2);

position ship_pos;

SDL_Renderer *renderer;
SDL_Window *window;                    
SDL_Event event;

Mix_Music *music;
Mix_Chunk *sound_effect;
Mix_Chunk *explosion_sound;
Mix_Chunk *shot_sound;
Mix_Chunk *enemy_shot_sound;


SDL_Texture **ship_text;
SDL_Texture *ship_forward_text;
SDL_Texture *ship_turn_text; //rotate to get other side

SDL_Texture *enemy_text;
SDL_Texture *explosion_text;

TTF_Font* font = NULL;
SDL_Color text_color = { 255, 255, 255, 255 }; // white
SDL_Texture *FPS_text;
SDL_Rect FPS_pos;

SDL_Texture *victory_texture;
SDL_Texture *gameover_texture;

struct enemy enemies[MAX_ENEMIES]; 
struct shot shots[MAX_SHOTS];
struct shot enemy_shots[MAX_ENEMY_SHOTS]; //not implemented
struct explosion explosions[MAX_EXPLOSIONS];

int enemy_direction = RIGHT;

void move_shots(struct shot *tmp_shots, int size, double delta);

int main(int argc, char* argv[]) {
  init_all();
    window = create_window();
    
    init_positions();
    
    //    spawn_enemy(WINDOW_WIDTH/2, 0, 0);
    spawn_enemies();
      //Create renderer
    if(!create_renderer()){
      return 0;
    }
    
    if(!load_music()){
      return 0;
    }
    ship_forward_text = IMG_LoadTexture(renderer, "spaceship_small2.png");
    if(!ship_forward_text){
      printf("Could not load texture: %s\n", IMG_GetError());
    }
    ship_text = &ship_forward_text;
 
    ship_turn_text = IMG_LoadTexture(renderer, "spaceship_small2_turn.png");
    if(!ship_turn_text){
      printf("Could not load texture: %s\n", IMG_GetError());
    } 

    enemy_text = IMG_LoadTexture(renderer, "enemy_1.png");
    if(!enemy_text){
      printf("Could not load texture: %s\n", IMG_GetError());
    }
    
    explosion_text = IMG_LoadTexture(renderer, "explosion.png");
    if(!explosion_text){
      printf("Could not load texture: %s\n", IMG_GetError());
    } 
    
    //loads font and creates texture
    gameover_texture = create_ttftext("Game Over", 90);
    victory_texture = create_ttftext("Victory", 90);
    
    render();
    
    if(run_game()){ //game loop
      //victory
      render_end(victory_texture);
    }else{
      //lose
      render_end(gameover_texture);      
    }
    
    // Close and destroy the window
    SDL_DestroyWindow(window);
    SDL_DestroyTexture(ship_forward_text);
    SDL_DestroyTexture(ship_turn_text);
    SDL_DestroyTexture(enemy_text);
    SDL_DestroyTexture(gameover_texture);
    SDL_DestroyTexture(victory_texture);
    // Clean up
    close_music();
    IMG_Quit();
    Mix_CloseAudio();
    Mix_Quit();
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();
    return 0;
}


SDL_Window *create_window(void){
  // Create an application window with the following settings:
    SDL_Window *window = SDL_CreateWindow(
        WINDOW_TITLE,                  // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        WINDOW_WIDTH,                               // width, in pixels
        WINDOW_HEIGHT,                               // height, in pixels
        0                  // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL) {
      // In the case that the window could not be made...
      printf("Could not create window: %s\n", SDL_GetError());
      return NULL;
    }
    return window;
} 

int create_renderer(void){
  renderer = SDL_CreateRenderer(window, -1, 0);
  if(!renderer){
    printf("create renderer: %s\n", SDL_GetError());
    return 0;
  }
  return 1;
}

SDL_Texture *create_ttftext(char *text, int size){
  if(!font){
    font = TTF_OpenFont ("fonts/Roboto-Regular.ttf", size);
  }
  if(!font) {
    printf("TTF_OpenFont: %s\n", TTF_GetError());
    return NULL;
  }
    
  SDL_Surface *font_surface = TTF_RenderText_Blended(font, text, text_color);
  if(!font_surface){
    printf("TTF_Rendertext_blended: %s\n", TTF_GetError());
    return NULL;
  }
    
  SDL_Texture *font_texture = SDL_CreateTextureFromSurface(renderer, font_surface);
    
  SDL_FreeSurface(font_surface);
  return font_texture;
}

void init_all(void){
  SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);              // Initialize SDL2
    
  int img_flags = IMG_INIT_PNG;
  int initted=IMG_Init(img_flags);
  if((initted&img_flags) != img_flags) {
    printf("IMG_Init: Failed to init required png support!\n");
    printf("IMG_Init: %s\n", IMG_GetError());
    // handle error
  }
  
  // open 44.1KHz, signed 16bit, system byte order,
  //      stereo audio, using 1024 byte chunks
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024)==-1) {
    printf("Mix_OpenAudio: %s\n", Mix_GetError());
    exit(2);
  }
  
  if(TTF_Init()==-1) {
    printf("TTF_Init: %s\n", TTF_GetError());
    exit(2);
  }
}

void init_positions(void){
  for(int i = 0; i < MAX_SHOTS; i++){
    shots[i].pos.pos_text.h = SHOT_SIZE;
    shots[i].pos.pos_text.w = SHOT_SIZE;
    shots[i].active = 0;
  }
    
  for(int i = 0; i < MAX_ENEMY_SHOTS; i++){
    enemy_shots[i].pos.pos_text.h = SHOT_SIZE;
    enemy_shots[i].pos.pos_text.w = SHOT_SIZE;
    enemy_shots[i].active = 0;
  }

  ship_pos.x = WINDOW_WIDTH/2;
  ship_pos.y = WINDOW_HEIGHT-SHIP_HEIGHT;
  ship_pos.pos_text.x = WINDOW_WIDTH/2;
  ship_pos.pos_text.y = WINDOW_HEIGHT-SHIP_HEIGHT;
  ship_pos.pos_text.h = SHIP_HEIGHT;
  ship_pos.pos_text.w = SHIP_WIDTH;

  FPS_pos.x = 0;
  FPS_pos.y = 0;
  FPS_pos.h = SQUARE_UNIT;
  FPS_pos.w = SQUARE_UNIT*2;
}

int load_music(void){

  music = Mix_LoadMUS("dk.ogg");
  if(!music){
    printf("Mix_LoadMUS: %s\n", Mix_GetError());
    return 0;
  }


  sound_effect = Mix_LoadWAV("arrow_x.wav");
  if(!sound_effect){
    printf("Mix_LoadWAV(\"music.mp3\"): %s\n", Mix_GetError());
    return 0;
  }

  shot_sound = Mix_LoadWAV("laser.wav");
  if(!shot_sound){
    printf("Mix_LoadWAV(\"music.mp3\"): %s\n", Mix_GetError());
    return 0;
  }

  enemy_shot_sound = Mix_LoadWAV("enemy_laser.wav");
  if(!enemy_shot_sound){
    printf("Mix_LoadWAV(\"music.mp3\"): %s\n", Mix_GetError());
    return 0;
  }

  explosion_sound = Mix_LoadWAV("explosion.wav");
  if(!explosion_sound){
    printf("Mix_LoadWAV(\"music.mp3\"): %s\n", Mix_GetError());
    return 0;
  }


  return 1;
}

void close_music(void){
  Mix_FreeChunk(shot_sound);
  Mix_FreeChunk(enemy_shot_sound);
  Mix_FreeChunk(explosion_sound);
  Mix_FreeChunk(sound_effect);
  Mix_FreeMusic(music);
}


void render(void){
  //Set color to black
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  //Paint everythign balck
  SDL_RenderClear(renderer);

  //paint ship
  SDL_RenderCopyEx(renderer, *ship_text, NULL, &(ship_pos.pos_text), \
                   0, NULL, flip);
  //change color to white
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  //make a white rect at test_pos
  //SDL_RenderFillRect(renderer, &test_pos);
  //Draw shots
  for(int i = 0; i < MAX_SHOTS; i++){
    if(shots[i].active){
      SDL_RenderFillRect(renderer, &(shots[i].pos.pos_text));
    }
  }

  //draw enemies
  for(int i = 0; i < MAX_ENEMIES; i++){
    if(enemies[i].active){
      SDL_RenderCopy(renderer, enemy_text, NULL, &enemies[i].pos.pos_text);
    }
  }
  
  //draw enemy shots
  SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
  for(int i = 0; i < MAX_ENEMY_SHOTS; i++){
    if(enemy_shots[i].active){
      SDL_RenderFillRect(renderer, &(enemy_shots[i].pos.pos_text));
    }
  }
  
  SDL_RenderCopy(renderer, FPS_text, NULL, &FPS_pos);

  //draw everything
  SDL_RenderPresent(renderer);
}

void render_end(SDL_Texture *font_text){
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  //Paint everythign balck
  SDL_RenderClear(renderer);
  SDL_Rect font_pos = { 
    .x = WINDOW_WIDTH/2-400/2, 
    .y = WINDOW_HEIGHT/2-150/2, 
    .w = 400, 
    .h = 150
  };      
  SDL_RenderCopy(renderer, font_text, NULL, &font_pos);
  SDL_RenderPresent(renderer);
  SDL_Delay(END_DELAY);
}

int run_game(void){
  double current_time;
  double previous_time;
  double delta;
  double last_shot = 0;
  double last_enemy_shot = 0;
  
  int quit = 0;
  int victory = 0;
  
  current_time = SDL_GetTicks();
  
  while(!quit){
    
    while(SDL_PollEvent(&event)){
      if(event.type == SDL_QUIT){
        puts("quitting");
        quit = 1;
      }else if(event.type == SDL_KEYDOWN) { 
        switch(event.key.keysym.sym){ 
        case SDLK_1: 
          Mix_PlayChannel( -1, sound_effect, 0 ); 
          break;
        
        case SDLK_9: //If there is no music playing 
          if( Mix_PlayingMusic() == 0 ) { //Play the music 
            Mix_PlayMusic( music, -1 ); 
          } //If music is being played 
          else 
            { //If the music is paused 
              if( Mix_PausedMusic() == 1 ) { //Resume the music 
                Mix_ResumeMusic(); 
              } //If the music is playing 
              else 
                { //Pause the music 
                  Mix_PauseMusic(); 
                } 
            } 
          break; 
        case SDLK_0: //Stop the music 
          Mix_HaltMusic(); 
          break; 
        }
      }
    }
    const Uint8* currentKeyStates = SDL_GetKeyboardState( NULL );
    int oldX = ship_pos.pos_text.x;
    char FPS_str[10];
    
    previous_time = current_time;
    current_time = SDL_GetTicks();
    delta = (current_time-previous_time)/1000;

    sprintf(FPS_str, "%d\n",(int)(1/delta));
    FPS_text = create_ttftext(FPS_str, 90);

    if(currentKeyStates[SDL_SCANCODE_LEFT]){
      ship_pos.x -= 125*delta;
    }
    if(currentKeyStates[SDL_SCANCODE_RIGHT]){
      ship_pos.x += 125*delta;
    }
    ship_pos.pos_text.x = ship_pos.x;
    validate_ship_pos();
    
    if(ship_pos.pos_text.x < oldX){
      ship_text = &ship_turn_text;
      flip = SDL_FLIP_HORIZONTAL;
    }else if(ship_pos.pos_text.x > oldX){
      ship_text = &ship_turn_text;
      flip = 0;
    }else{
      ship_text = &ship_forward_text;
      flip = 0;
    }
    
    if(currentKeyStates[SDL_SCANCODE_UP]){
      if(current_time > last_shot+shoot_delay){
        if(shoot()){
          Mix_PlayChannel( -1, shot_sound, 0 );
          last_shot = current_time;
        }
      }
    }
    
    int rand_delay = rand() % enemy_shoot_delay;
    rand_delay -= enemy_shoot_delay/2;
    if(current_time > last_enemy_shot+enemy_shoot_delay+rand_delay){
      int shot_id = get_enemy_shot();
      if(!enemy_shoot(shot_id)){
        puts("out of enemy shots");
      }else{
        Mix_PlayChannel( -1, enemy_shot_sound, 0 );
      }
      last_enemy_shot = current_time;
    }
    
    move_enemies(delta);
    move_shots(shots, MAX_SHOTS, delta);
    move_shots(enemy_shots, MAX_ENEMY_SHOTS, delta);
    
    shot_collisions();
    if(ship_collisions()){ //lost
      quit = 1;
      victory = 0;
    }

 
    if(!all_enemies_alive()){ //won
      quit = 1;
      victory = 1;
    }
    
    render();
    SDL_DestroyTexture(FPS_text);
    SDL_Delay(15);
  }
  return victory;
}

void shot_collisions(void){
  for(int i = 0; i < MAX_SHOTS; i++){
    if(shots[i].active){
      //shots hitting enemy shots
      for(int j = 0; j < MAX_ENEMY_SHOTS; j++){
        if(enemy_shots[j].active){
          if(check_collision(enemy_shots[j].pos.pos_text, shots[i].pos.pos_text)){
            //enemy shot is hit
            enemy_shots[j].active = 0;
            shots[i].active = 0;
          }
        }
      }
      //shots hitting enemies
      for(int j = 0; j < MAX_ENEMIES; j++){
        if(enemies[j].active){
          if(check_collision(enemies[j].pos.pos_text, shots[i].pos.pos_text)){
            //enemy is hit
            enemies[j].active = 0;
            shots[i].active = 0;
            Mix_PlayChannel( -1, explosion_sound, 0 );
          }
        }
      }
    }
  }
}

int ship_collisions(void){
  //enemy shots
  for(int i = 0; i < MAX_ENEMY_SHOTS; i++){
    if(enemy_shots[i].active && check_collision(enemy_shots[i].pos.pos_text, ship_pos.pos_text)){
      //rip ship
      return 1;
    }
  }

  for(int i = 0; i < MAX_ENEMIES; i++){
    if(enemies[i].active && check_collision(enemies[i].pos.pos_text, ship_pos.pos_text)){
      return 1;
    }
  }

  //enemies

  return 0;
}

void validate_ship_pos(void){
  if(ship_pos.x+SHIP_WIDTH > WINDOW_WIDTH){
    ship_pos.x = WINDOW_WIDTH-SHIP_WIDTH;
  }else if(ship_pos.x < 0){
    ship_pos.x = 0;
  }
}

int check_collision( const SDL_Rect rect1, const SDL_Rect rect2 )
{
  // Find edges of rect1
  int left1 = rect1.x;
  int right1 = rect1.x + rect1.w;
  int top1 = rect1.y;
  int bottom1 = rect1.y + rect1.h;

  // Find edges of rect2
  int left2 = rect2.x;
  int right2 = rect2.x + rect2.w;
  int top2 = rect2.y;
  int bottom2 = rect2.y + rect2.h;

  // Check edges
  if ( left1 > right2 )// Left 1 is right of right 2
    return 0; // No collision

  if ( right1 < left2 ) // Right 1 is left of left 2
    return 0; // No collision

  if ( top1 > bottom2 ) // Top 1 is below bottom 2
    return 0; // No collision

  if ( bottom1 < top2 ) // Bottom 1 is above top 2 
    return 0; // No collision

  return 1;
}

int shoot(void){
  for(int i = 0; i < MAX_SHOTS; i++){
    if(!shots[i].active){
      shots[i].active = 1;
      shots[i].pos.x = ship_pos.x+SHIP_WIDTH/2-SHOT_SIZE/2;
      shots[i].pos.pos_text.x = shots[i].pos.x;
      shots[i].pos.y = ship_pos.y-SHOT_SIZE;
      shots[i].pos.pos_text.y = shots[i].pos.y;
      shots[i].dir = UP;
      shots[i].speed = 500;//400;
      return 1;
    }
  }
  return 0;
}

int enemy_shoot(int index){
  if(enemies[index].active){ //This should be checked outside function also
    for(int i = 0; i < MAX_ENEMY_SHOTS; i++){
      if(!enemy_shots[i].active){
        enemy_shots[i].active = 1;
        enemy_shots[i].pos.x = enemies[index].pos.x+SQUARE_UNIT/2-SHOT_SIZE/2;
        enemy_shots[i].pos.pos_text.x = enemy_shots[i].pos.x;
        enemy_shots[i].pos.y = enemies[index].pos.y+SQUARE_UNIT;
        enemy_shots[i].pos.pos_text.y = enemy_shots[i].pos.y;
        enemy_shots[i].dir = DOWN;
        enemy_shots[i].speed = 200;
        return 1;
      }
    }
  }
  return 0;
}

void move_shots(struct shot *tmp_shots, int size, double delta){
  for(int i = 0; i < size; i++){
    if(tmp_shots[i].active){
      switch(tmp_shots[i].dir){
      case UP:
        tmp_shots[i].pos.y -= tmp_shots[i].speed*delta;
        break;
      case DOWN:
        tmp_shots[i].pos.y += tmp_shots[i].speed*delta;
        break;
      default:
        puts("move_shots error");
        break;
      }
      tmp_shots[i].pos.pos_text.y = tmp_shots[i].pos.y;
      
      //checks for OOB
      if(tmp_shots[i].pos.y > WINDOW_HEIGHT){
        tmp_shots[i].active = 0;
      }else if(tmp_shots[i].pos.y+SHOT_SIZE < 0){
        tmp_shots[i].active = 0;
      }
    }
  }
}

void spawn_enemy(int x, int y, int i){
    if(!enemies[i].active){
      enemies[i].pos.y = y;
      enemies[i].pos.pos_text.y = y;
      enemies[i].pos.x = x;
      enemies[i].pos.pos_text.x = x;
      enemies[i].pos.pos_text.h = SQUARE_UNIT;
      enemies[i].pos.pos_text.w = SQUARE_UNIT;
      enemies[i].active = 1;
      enemies[i].speed = ENEMY_SPEED;
      return;
    }
  
  puts("That enemy is already active");
}

int all_enemies_alive(void){
  int nr_alive = 0;
  for(int i = 0; i < MAX_ENEMIES; i++){
    if(enemies[i].active){
      nr_alive++;
    }
  }
  return nr_alive;
}

void spawn_enemies(void){
  int x,y;
  for(int i = 0; i < ENEMY_ROW; i++){
    for(int j = 0; j < ENEMY_COLUMN; j++){
      y = SQUARE_UNIT*i+(i+1)*SQUARE_UNIT/2;
      x = SQUARE_UNIT*j+SQUARE_UNIT/2*(j+1);
      spawn_enemy(x,y,i*ENEMY_COLUMN+j);
    }
  }
}

void move_enemies(double delta){
  int go_down = 0;
  double nr_alive = all_enemies_alive();
  double max_e = MAX_ENEMIES;
  double bonus_speed = 1/max_e*(max_e-nr_alive)*ENEMY_SPEED*5;

  //Casting one dim array to two dim. Do not overextend.
  struct enemy (*t_enemies)[ENEMY_COLUMN] = \
    (struct enemy (*)[ENEMY_COLUMN])&enemies[0];  
  
  if(t_enemies[0][0].pos.pos_text.x <= 0+SQUARE_UNIT/2){
    if(enemy_direction == LEFT){
      go_down = 1;
    }
    enemy_direction = RIGHT;
  }else if(t_enemies[0][ENEMY_COLUMN-1].pos.pos_text.x >= \
           WINDOW_WIDTH-SQUARE_UNIT){
    if(enemy_direction == RIGHT){
      go_down = 1;
    }
    enemy_direction = LEFT;
  }
  
  for(int i = 0; i < ENEMY_ROW; i++){
    for(int j = 0; j < ENEMY_COLUMN; j++){
      if(go_down){
        t_enemies[i][j].pos.y += SQUARE_UNIT*3/2; 
        t_enemies[i][j].pos.pos_text.y = t_enemies[i][j].pos.y;
      }

      double e_speed = t_enemies[i][j].speed+bonus_speed;
      if(enemy_direction == RIGHT){
        t_enemies[i][j].pos.x += e_speed*delta; 
        t_enemies[i][j].pos.pos_text.x = t_enemies[i][j].pos.x;
      }else if(enemy_direction == LEFT){
        t_enemies[i][j].pos.x -= e_speed*delta; 
        t_enemies[i][j].pos.pos_text.x = t_enemies[i][j].pos.x;
      }
    }
  }
}

int get_enemy_shot(){
  struct enemy (*t_enemies)[ENEMY_COLUMN] = \
    (struct enemy (*)[ENEMY_COLUMN])&enemies[0];  
  
  int col, row;
  for(;;){
    col = rand() % ENEMY_COLUMN;
    for(int i = 0; i < ENEMY_ROW; i++){
      row = ENEMY_ROW-i-1;
      if(t_enemies[row][col].active){
        return row*ENEMY_COLUMN+col;
      }
    }
  }
  

}
