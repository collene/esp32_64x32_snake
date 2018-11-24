#include <PxMatrix.h>

// ESP32 Pins for JoyStick
#define JOY_X 35
#define JOY_Y 34
#define KEY_1 27
#define KEY_2 32
#define KEY_3 33

// ESP32 Pins for LED MATRIX
#define P_LAT 22
#define P_A 19
#define P_B 23
#define P_C 18
#define P_D 5
#define P_E 15  // NOT USED for 1/16 scan
#define P_OE 2
hw_timer_t * timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

uint8_t display_draw_time = 0;

PxMATRIX display(64,32,P_LAT, P_OE,P_A,P_B,P_C,P_D);

// Some standard colors
uint16_t RED = display.color565(255, 0, 0);
uint16_t GREEN = display.color565(0, 255, 0);
uint16_t BLUE = display.color565(0, 0, 255);
uint16_t WHITE = display.color565(255, 255, 255);
uint16_t YELLOW = display.color565(255, 255, 0);
uint16_t CYAN = display.color565(0, 255, 255);
uint16_t PURPLE = display.color565(255, 0, 255);
uint16_t BLACK = display.color565(0, 0, 0);
uint16_t playerColor = BLUE;
unsigned long lastColorChange;

#define ROWS 64
#define COLS 32

#define UP 1
#define LEFT 2
#define RIGHT 3
#define DOWN 4

struct point {
  int x;
  int y;
};

point player[ROWS * COLS];

int playerDirection;
int playerLength;
point playerHead;

point apple;

int board[ROWS][COLS];

unsigned long lastClockTick;
int gameRate;
int numApplesEaten = 0;

#define INITIALIZING 1
#define PLAYING 2
#define PAUSED 3
#define GAME_OVER 4

int playState = INITIALIZING;

// boilerplate code for the display -- copied from another library
void IRAM_ATTR display_updater(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);
  display.display(display_draw_time);
  portEXIT_CRITICAL_ISR(&timerMux);
}
void display_update_enable(bool is_enable)
{
if (is_enable)
  {
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &display_updater, true);
    timerAlarmWrite(timer, 2000, true);
    timerAlarmEnable(timer);
  }
else
  {
    timerDetachInterrupt(timer);
    timerAlarmDisable(timer);
  }
}

void setup() {  
  pinMode(KEY_1, INPUT_PULLUP);
  pinMode(KEY_2, INPUT_PULLUP);
  pinMode(KEY_3, INPUT_PULLUP);
  
  display.begin(16); // 1/16 scan
  display.setFastUpdate(true);
  display_update_enable(true);

  randomSeed(analogRead(0));
  defineBoard();
  startGame();
}

void defineBoard() {
  for(int i = 0; i < COLS; i++) {
    board[0][i] = 1;
    board[ROWS - 1][i] = 1;
  }
}
void generateRandomBoard() {
  // clear the existing board first
  for(int i = 1; i < ROWS - 1; i++) {
    for(int j = 0; j < COLS; j++) {
      if(j == 0 || j == (COLS - 1)) {
        board[i][j] = 1;        
      } else {
        board[i][j] = 0;
      }
    }
  }
  // 40 random pixels set
  for(int i = 0; i < 40; i++) {
    bool found = false;
    int x = random(1, ROWS - 1);
    int y = random(1, COLS - 1);
    while(!found) {          
      if(!boardContainsCoordinates(x, y)
         && !boardContainsCoordinates(x - 1, y - 1)         
         && !boardContainsCoordinates(x + 1, y - 1)
         && !boardContainsCoordinates(x - 1, y + 1) 
         && !boardContainsCoordinates(x + 1, y + 1)){
         found = true;
      } else {
        x = random(1, ROWS - 1);
        y = random(1, COLS - 1);
      }
    }
    board[x][y] = 1;
  }
}

void startGame() {
  display.clearDisplay();
  resetGameVariables();
  drawBoard();
  drawPlayer();  
  playState = PLAYING;
}

void resetGameVariables() {  
  playState = INITIALIZING;
  generateRandomBoard();
  bool found = false;  
  // generate a random spot for the user to start, and a random direction
  while(!found) {
    // start the player in a random spot
    playerHead.x = random(1, ROWS - 1);
    playerHead.y = random(1, COLS - 1);
    int startDirection = random(0, 2);
    
    if(startDirection == 0) {
      if(playerHead.x < ROWS / 2) {
        playerDirection = DOWN;
      } else {
        playerDirection = UP;
      }
    } else {
      if(playerHead.y < COLS / 2) {
        playerDirection = LEFT;
      } else {
        playerDirection = RIGHT;
      }  
    }

    if(playerHas5Moves()) {
      found = true;
    }
  }

  generateApple();

  playerLength = 1;
  player[0].x = playerHead.x;
  player[0].y = playerHead.y;

  lastClockTick = millis();
  lastColorChange = millis();
  gameRate = 200;  
  numApplesEaten = 0;
}
// make sure with the random start that the player has a few moves to react
bool playerHas5Moves() {
  for(int i = 0; i < 5; i++) {
    switch(playerDirection) {
      case RIGHT:
        if(board[playerHead.x][playerHead.y + i] == 1) {
          return false;
        }
        break;
      case LEFT:
        if(board[playerHead.x][playerHead.y - i] == 1) {
          return false;
        }
        break;
      case UP:
        if(board[playerHead.x - i][playerHead.y] == 1) {
          return false;
        }
        break;
      case DOWN:
        if(board[playerHead.x + i][playerHead.y] == 1) {
          return false;
        }
        break;
    }
  }
  return true;
}
void generateApple() {
  bool found = false;
  // make sure that the apple doesn't end up on a board coordinate or on top of the player
  while(!found) {    
    apple.x = random(1, ROWS - 1);
    apple.y = random(1, COLS - 1);
    if(!playerContainsCoordinates(apple.x, apple.y) && !boardContainsCoordinates(apple.x, apple.y)) {       
      found = true;
    }
  }  
  display.drawPixel(apple.x, apple.y, RED);
}
bool playerContainsCoordinates(int x, int y) {  
  for(int i = 0; i < playerLength; i++) {
    if(player[i].x == x && player[i].y == y) {      
      return true;
    }
  }
  return false;
}
bool boardContainsCoordinates(int x, int y) {
  return board[x][y] == 1;
}
void drawPlayer() {
  for(int i = 0; i < playerLength; i++) {        
    display.drawPixel(player[i].x, player[i].y, playerColor);    
  }  
}
void drawBoard() {
  for(int i = 0; i < ROWS; i++) {
    for(int j = 0; j < COLS; j++) {
      if(i == 0 && j == 0) {
        display.drawPixel(i, j, playerColor);
      } else if(board[i][j] == 1) {   
        display.drawPixel(i, j, GREEN);             
      } else if(i == apple.x && j == apple.y) {
        display.drawPixel(i, j, RED);
      } else {
        display.drawPixel(i, j, BLACK);        
      }
    }
  }  
}
void setPlayerDirection() {
  int x = (63 * analogRead(JOY_X)) / 4096;
  int y = 30 - ((31 * analogRead(JOY_Y)) / 4096);

  float deltax = abs(ROWS / 2 - x);
  float deltay = abs(COLS / 2 - y);

  if(deltax > deltay * 2) {
    if(x <= ROWS / 2 - 10) {
      playerDirection = UP;
    } else if(x > ROWS / 2 + 10) {
      playerDirection = DOWN;
    }
  } else {
    if(y <= COLS / 2 - 10) {
      playerDirection = LEFT;
    } else if(y > COLS / 2 + 10) {
      playerDirection = RIGHT;
    }
  }
}

void loop() {  
  readButtons();
  if(playState == PLAYING) {
    setPlayerDirection();
    if(millis() - lastClockTick > gameRate) {
      advancePlayer();
      detectCollision();
      lastClockTick = millis();
    }
  }
}

void readButtons() {
  if(digitalRead(KEY_1) == LOW) {
    playState = PAUSED;
  } else if(digitalRead(KEY_2) == LOW) {
    playState = PLAYING;
  } else if(digitalRead(KEY_3) == LOW && millis() - lastColorChange > 500) {   
    changePlayerColor();
  }
}

void advancePlayer() {  
  if(playerDirection == LEFT) {
    playerHead.y -= 1;
  } else if(playerDirection == RIGHT) {    
    playerHead.y += 1;    
  } else if(playerDirection == UP) {    
    playerHead.x -= 1;    
  } else if(playerDirection == DOWN) {        
    playerHead.x += 1;
  }  
  // see if this point already exists in the player's matrix
  for(int i = 0; i < playerLength; i++) {
    if(player[i].x == playerHead.x && player[i].y == playerHead.y) {
      gameOver();
    }
  }
  if(!detectAppleEaten()) {
    display.drawPixel(player[playerLength - 1].x, player[playerLength - 1].y, BLACK);
  }
  display.drawPixel(playerHead.x, playerHead.y, playerColor);
  for(int i = playerLength - 1; i > 0; i--) {
    player[i] = player[i - 1];
  }
  player[0].x = playerHead.x;
  player[0].y = playerHead.y;
}

void changePlayerColor() {  
  if(playerColor == BLUE) {
    playerColor = WHITE;
  } else if(playerColor == WHITE) {
    playerColor = YELLOW;    
  } else if(playerColor == YELLOW) {
    playerColor = CYAN;
  } else if(playerColor == CYAN) {
    playerColor = PURPLE;
  } else {
    playerColor = BLUE;
  }  
  for(int i = 0; i < playerLength - 1; i++) {
    display.drawPixel(player[i].x, player[i].y, playerColor);
  }
  lastColorChange = millis();
}

void detectCollision() {
  if(board[playerHead.x][playerHead.y] == 1) {
    gameOver();
  }
}

boolean detectAppleEaten() {
  if(playerHead.x == apple.x && playerHead.y == apple.y) {    
    numApplesEaten++;
    playerLength += 1;
    player[playerLength - 1].x = playerHead.x;
    player[playerLength - 1].y = playerHead.y;
    display.drawPixel(playerHead.x, playerHead.y, playerColor);
    if(numApplesEaten % 4 == 0 && gameRate > 40) {
      gameRate -= 20;
    }
    generateApple();
    return true;
  }
  return false;
}

void gameOver() {
  playState = GAME_OVER;
  display.clearDisplay();
  for(int i = 0; i < ROWS; i++) {
    for(int j = 0; j < COLS; j++) {
      display.drawPixel(i, j, GREEN);      
    }    
  }  
  delay(3000);
  startGame();
}

