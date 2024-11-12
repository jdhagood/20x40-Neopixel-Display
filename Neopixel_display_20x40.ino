#include <Adafruit_NeoPixel.h>

#define LED_PIN 6      // GPIO pin for the NeoPixel data line
#define LED_COUNT 800  // Total number of LEDs (20x40 matrix)
#define ROWS 40        // Number of rows
#define COLS 20        // Number of columns
#define BRIGHTNESS 30  // Adjust brightness (0-255)

// Button GPIO pins
#define BUTTON_LEFT 1
#define BUTTON_RIGHT 2
#define BUTTON_ROTATE_LEFT 3
#define BUTTON_ROTATE_RIGHT 4
#define BUTTON_FAST_FALL 5

//Game Parameters
#define GAME_TICK_SPEED 500
#define DEBOUNCE_TIME 10

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Color array for each Tetris piece
const uint32_t colors[] = {
  strip.Color(255, 255, 0),    // O piece: yellow
  strip.Color(0, 255, 255),    // I piece: cyan
  strip.Color(255, 0, 0),      // S piece: red
  strip.Color(0, 255, 0),      // Z piece: green
  strip.Color(255, 165, 0),    // L piece: orange
  strip.Color(255, 192, 203),  // J piece: pink
  strip.Color(255, 0, 255)     // T piece: purple
};

class TetrisGame {
private:
  uint32_t board[ROWS / 2][COLS / 2];  // The game board stores color values
  const int pieces[7][4][4] = {
    // Tetris pieces defined as 4x4 grids
    { { 1, 1, 0, 0 },
      { 1, 1, 0, 0 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 } },  // O piece

    { { 0, 0, 0, 0 },
      { 1, 1, 1, 1 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 } },  // I piece

    { { 0, 1, 1, 0 },
      { 1, 1, 0, 0 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 } },  // S piece

    { { 1, 1, 0, 0 },
      { 0, 1, 1, 0 },
      { 0, 0, 0, 0 },
      { 0, 0, 0, 0 } },  // Z piece

    { { 1, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // L piece

    { { 0, 0, 1, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // J piece

    { { 0, 1, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }  // T piece
  };

  // Current piece properties
  int currentPiece;
  int currentPieceShape[4][4];
  int posX, posY;
  unsigned long last_game_update = 0;
  unsigned long last_button_press = 0;
  bool button_pressed = false;

public:
  TetrisGame() {
    // Initialize the board to 0 (empty)
    memset(board, 0, sizeof(board));
    currentPiece = random(0, 7);  // Select a random piece
    posX = (COLS / 2 - 2) / 2;    // Center the piece horizontally, scaled down
    posY = 0;                     // Start at the top
  }

  void setup() {
    strip.begin();
    strip.setBrightness(BRIGHTNESS);
    strip.show();  // Clear the display
    pinMode(BUTTON_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
    pinMode(BUTTON_ROTATE_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_ROTATE_RIGHT, INPUT_PULLUP);
    pinMode(BUTTON_FAST_FALL, INPUT_PULLUP);
    spawnNewPiece();
  }

  void play() {
    // Main game loop logic
    handleInput();
    if (millis() - last_game_update >= GAME_TICK_SPEED) {
      last_game_update = millis();
      movePieceDown();
    }
    drawBoard();
  }

private:
  void handleInput() {
    if (digitalRead(BUTTON_LEFT) == LOW && !button_pressed) {
      movePieceLeft();
    }
    if (digitalRead(BUTTON_RIGHT) == LOW && !button_pressed) {
      movePieceRight();
    }
    if (digitalRead(BUTTON_ROTATE_LEFT) == LOW && !button_pressed) {
      rotateLeft();
    }
    if (digitalRead(BUTTON_ROTATE_RIGHT) == LOW && !button_pressed) {
      rotateRight();
    }
    // if (digitalRead(BUTTON_FAST_FALL) == LOW) {
    //   fastFall();
    // }
    if (digitalRead(BUTTON_LEFT) == HIGH && digitalRead(BUTTON_RIGHT) == HIGH && digitalRead(BUTTON_ROTATE_LEFT) == HIGH && digitalRead(BUTTON_ROTATE_RIGHT) == HIGH && millis() - last_button_press > DEBOUNCE_TIME) {
      button_pressed = false;
    }
  }

  void movePieceLeft() {
    last_button_press = millis();
    button_pressed = true;
    posX--;
    if (collisionDetected(currentPieceShape)) {
      posX++;
    }
  }

  void movePieceRight() {
    last_button_press = millis();
    button_pressed = true;
    posX++;
    if (collisionDetected(currentPieceShape)) {
      posX--;
    }
  }

  // Rotate the piece 90 degrees clockwise
  void rotateRight() {
    last_button_press = millis();
    button_pressed = true;
    int rotatedPiece[4][4] = { 0 };  // Temporary array to store the rotated piece

    // Rotate the piece 90 degrees clockwise
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        rotatedPiece[x][3 - y] = currentPieceShape[y][x];
      }
    }

    // Check for collisions with the board edges or existing blocks
    if (!collisionDetected(rotatedPiece)) {
      // If no collision, copy the rotated piece back to currentPieceShape
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          currentPieceShape[y][x] = rotatedPiece[y][x];
        }
      }
    }
  }

  // Rotate the piece 90 degrees counterclockwise
  void rotateLeft() {
    last_button_press = millis();
    button_pressed = true;
    int rotatedPiece[4][4] = { 0 };  // Temporary array to store the rotated piece

    // Rotate the piece 90 degrees counterclockwise
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        rotatedPiece[3 - x][y] = currentPieceShape[y][x];
      }
    }

    // Check for collisions with the board edges or existing blocks
    if (!collisionDetected(rotatedPiece)) {
      // If no collision, copy the rotated piece back to currentPieceShape
      for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
          currentPieceShape[y][x] = rotatedPiece[y][x];
        }
      }
    }
  }


  void movePieceDown() {
    // Logic to move the current piece down
    posY++;
    if (collisionDetected(currentPieceShape)) {
      posY--;  // Revert if there is a collision
      placePiece();
      clearLines();
      spawnNewPiece();
    }
  }

  bool collisionDetected(int p[4][4]) {
    // Check if the current piece collides with the board or the floor
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (p[x][y] != 0) {
          int boardX = posX + x;
          int boardY = posY + y;
          if (boardX < 0 || boardX >= (COLS / 2) || boardY >= (ROWS / 2) || board[boardY][boardX] != 0) {
            return true;
          }
        }
      }
    }
    return false;
  }

  void placePiece() {
    // Place the current piece on the board with its color
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentPieceShape[x][y] != 0) {
          board[posY + y][posX + x] = colors[currentPiece];  // Store the color
        }
      }
    }
  }

  void clearLines() {
    // Check and clear completed lines
    for (int y = 0; y < (ROWS / 2); y++) {
      bool fullLine = true;
      for (int x = 0; x < (COLS / 2); x++) {
        if (board[y][x] == 0) {
          fullLine = false;
          break;
        }
      }
      if (fullLine) {
        // Shift all rows above down
        for (int row = y; row > 0; row--) {
          memcpy(board[row], board[row - 1], sizeof(board[row]));
        }
        memset(board[0], 0, sizeof(board[0]));  // Clear the top row
      }
    }
  }

  void spawnNewPiece() {
    // Spawn a new piece
    currentPiece = random(0, 7);
    posX = (COLS / 2 - 2) / 2;
    posY = 0;
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        currentPieceShape[x][y] = pieces[currentPiece][x][y];
      }
    }
  }

  void drawBoard() {
    // Draw the board and current piece on the LED matrix
    for (int y = 0; y < (ROWS / 2); y++) {
      for (int x = 0; x < (COLS / 2); x++) {
        if (board[y][x] == 0) {
          set2x2PixelBlock(x, y, 0);  // Turn off LED
        } else {
          set2x2PixelBlock(x, y, board[y][x]);  // Use the stored color
        }
      }
    }
    // Draw the current piece
    for (int y = 0; y < 4; y++) {
      for (int x = 0; x < 4; x++) {
        if (currentPieceShape[x][y] != 0) {
          set2x2PixelBlock(posX + x, posY + y, colors[currentPiece]);  // Use the current piece color
        }
      }
    }
    strip.show();
  }

  void set2x2PixelBlock(int x, int y, uint32_t color) {
    // Draw a 2x2 block of pixels for each game piece or board cell
    int baseX = x * 2;
    int baseY = y * 2;
    strip.setPixelColor(getPixelIndex(baseX, baseY), color);
    strip.setPixelColor(getPixelIndex(baseX + 1, baseY), color);
    strip.setPixelColor(getPixelIndex(baseX, baseY + 1), color);
    strip.setPixelColor(getPixelIndex(baseX + 1, baseY + 1), color);
  }

  int getPixelIndex(int x, int y) {
    // Map (x, y) coordinates to the NeoPixel index
    if (x % 2 == 0) {
      return x * ROWS + y;
    } else {
      return (x + 1) * ROWS - y - 1;
    }
  }
};

// Create an instance of the TetrisGame class
TetrisGame game;

void setup() {
  game.setup();
}

void loop() {
  game.play();
}