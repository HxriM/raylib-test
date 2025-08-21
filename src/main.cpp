#include <iostream>
#include <raylib.h>
#include <expected>
#include <map>
#include <string>
#include <string_view>
#include <stdexcept>

class Window
{
public:
  Window(float width, float height, const char *title)
  {
    InitWindow(width, height, title);
  }

  ~Window()
  {
    CloseWindow();
  }

  bool shouldClose() const
  {
    return WindowShouldClose(); // Check if application should close
  }

  bool stayOpen() const
  {
    return !shouldClose(); // Return true if the window should stay open
  }

  bool isReady() const
  {
    return IsWindowReady(); // Check if window is ready
  }
};

enum class ScreenState
{
  Config,
  Playing
};

class AudioDevice
{
public:
  AudioDevice()
  {
    InitAudioDevice(); // Initialize audio device
  }

  ~AudioDevice()
  {
    CloseAudioDevice(); // Close audio device
  }

  bool isReady() const
  {
    return IsAudioDeviceReady(); // Check if audio device is ready
  }
};

class SoundContainer
{
public:
  SoundContainer()
  {
  }

  void loadSound(const char *filePath, std::string_view name)
  {
    Sound sound = LoadSound(filePath);
    if (sound.stream.buffer == nullptr)
    {
      throw std::runtime_error("Failed to load sound: " + std::string(filePath));
    }
    sounds.emplace(std::string(name), std::move(sound));
  }

  void playSound(const std::string &name)
  {
    auto it = sounds.find(name);
    if (it != sounds.end())
    {
      PlaySound(it->second); // Play the sound if found
    }
    else
    {
      throw std::runtime_error("Sound not found: " + name);
    }
  }

private:
  std::map<std::string, Sound> sounds;
};

struct GameState
{
  size_t cpuScore = 0;
  size_t playerScore = 0;
  ScreenState screenState = ScreenState::Config; // Initial screen state
};

class Paddle
{
public:
  Paddle(float x, float y, float width, float height, Color color)
      : x_(x), y_(y), width_(width), height_(height), color_(color) {}

  void draw() const
  {
    ::DrawRectangle(x_, y_, width_, height_, color_);
  }

  bool canMoveUp(float yMin) const
  {
    return y_ > yMin;
  }

  bool canMoveDown(float yMax) const
  {
    return y_ + height_ < yMax;
  }

  float getX() const { return x_; }
  float getY() const { return y_; }
  float getWidth() const { return width_; }
  float getHeight() const { return height_; }
  Color getColor() const { return color_; }
  Rectangle getRect() const { return {x_, y_, width_, height_}; }

  void setSpeed(float speed)
  {
    speed_ = speed;
  }

protected:
  float x_;
  float y_;
  float width_;
  float height_;
  Color color_;
  float speed_ = 10;
};

class Ball
{
public:
  Ball(float x, float y, float radius, Color color)
      : x_(x), y_(y), radius_(radius), color_(color) {}

  void draw() const
  {
    ::DrawCircle(x_, y_, radius_, color_);
  }

  enum class CollisionType
  {
    None,
    Left,
    Right,
    TopOrBottom
  };

  CollisionType update(Rectangle rect, float lineThick)
  {
    // Update ball position based on speed
    auto newX = x_ + speed_.x;
    auto newY = y_ + speed_.y;

    if (newX - radius_ < rect.x + lineThick)
    {
      return CollisionType::Left; // Ball hit left wall
    }

    if (newX + radius_ > rect.x + rect.width - lineThick)
    {
      return CollisionType::Right; // Ball hit right wall
    }

    if (newY - radius_ < rect.y + lineThick || newY + radius_ > rect.y + rect.height - lineThick)
    {
      reverseY();                        // Reverse direction on Y-axis if hitting top or bottom wall
      return CollisionType::TopOrBottom; // No collision with left or right wall
    }

    x_ = newX;
    y_ = newY;
    return CollisionType::None; // No collision
  }

  float getX() const { return x_; }
  float getY() const { return y_; }
  float getRadius() const { return radius_; }
  Color getColor() const { return color_; }
  Vector2 getPos() const { return {x_, y_}; }
  Vector2 getSpeed() const { return speed_; }

  void reverseX() { speed_.x *= -1; }
  void reverseY() { speed_.y *= -1; }
  void setSpeed(Vector2 speed)
  {
    speed_ = speed;
  }

private:
  float x_;
  float y_;
  float radius_;
  Color color_;
  Vector2 speed_ = {10, 10};
};

class PlayerPaddle : public Paddle
{
public:
  using Paddle::Paddle;

  void updatePosition(float yMin, float yMax)
  {
    if (IsKeyDown(KEY_DOWN) && canMoveDown(yMax))
    {
      y_ += speed_; // Move down
    }

    if (IsKeyDown(KEY_UP) && canMoveUp(yMin))
    {
      y_ -= speed_; // Move up
    }
  }
};

class CpuPaddle : public Paddle
{
public:
  using Paddle::Paddle;

  void updatePosition(float ballY, float yMin, float yMax)
  {
    // Simple AI to follow the ball's Y position
    if (y_ + height_ / 2 < ballY && canMoveDown(yMax))
    {
      y_ += speed_; // Move down
    }
    else if (y_ + height_ / 2 > ballY && canMoveUp(yMin))
    {
      y_ -= speed_; // Move up
    }
  }
};
struct ConfigOptions
{
  float screenWidth = 800;
  float screenHeight = 600;
  float containerMarginX = 10;
  float containerMarginY = 10;
  float containerThick = 10;
  float paddleSpeed = 7; // Speed of the paddles
  float ballSpeed = 5;   // Speed of the ball
};

void resetBall(Ball &ball, const ConfigOptions &config)
{
  auto randX = GetRandomValue(-100, 150);
  auto randY = GetRandomValue(-100, 150);
  ball = Ball(config.screenWidth / 2 + randX, config.screenHeight / 2 + randY, 10, WHITE);
}

void drawScore(const GameState &gameState, const ConfigOptions &config)
{
  DrawText(TextFormat("%i", gameState.playerScore), config.screenWidth / 4 - 20, 20, 80, WHITE);
  DrawText(TextFormat("%i", gameState.cpuScore), 3 * config.screenWidth / 4 - 20, 20, 80, WHITE);
}

void drawConfigScreen(ConfigOptions &config)
{
  ::ClearBackground(DARKGRAY);
  ::DrawText("CONFIG SCREEN", config.screenWidth / 2 - 100, 100, 30, WHITE);
  ::DrawText("Press UP/DOWN to change Paddle Speed", config.screenWidth / 2 - 180, 200, 20, WHITE);
  ::DrawText(TextFormat("Paddle Speed: %i", (int)config.paddleSpeed), config.screenWidth / 2 - 80, 250, 20, YELLOW);
  ::DrawText("Press LEFT/RIGHT to change Ball Speed", config.screenWidth / 2 - 180, 280, 20, WHITE);
  ::DrawText(TextFormat("Ball Speed: %i", (int)config.ballSpeed), config.screenWidth / 2 - 80, 300, 20, YELLOW);
  ::DrawText("Press ENTER to start", config.screenWidth / 2 - 100, 350, 20, GREEN);
}

int main()
{
  ConfigOptions config;

  Window window(config.screenWidth, config.screenHeight, "Pongongongong");

  AudioDevice audioDevice;

  SoundContainer soundContainer;
  soundContainer.loadSound("resources/bounce.mp3", "bounce");
  soundContainer.loadSound("resources/gameover.mp3", "gameover");
  soundContainer.loadSound("resources/paddleclick.mp3", "paddleclick");
  soundContainer.loadSound("resources/yay.mp3", "yay");

  ::SetTargetFPS(60); // Set our game to run at 60 frames-per-second

  // Draw Container Rectangle
  Rectangle container = {
      config.containerMarginX,
      config.containerMarginY,
      config.screenWidth - 2 * config.containerMarginX,
      config.screenHeight - 2 * config.containerMarginY};

  Color colors[21] = {
      YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN,
      SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN,
      LIGHTGRAY, GRAY, DARKGRAY};

  Color bgColor = BLACK;
  size_t colorIndex = 0;

  PlayerPaddle playerPaddle(50, config.screenHeight / 2 - 50, 10, 100, WHITE);
  CpuPaddle cpuPaddle(config.screenWidth - 60, config.screenHeight / 2 - 50, 10, 100, WHITE);
  Ball ball(config.screenWidth / 2, config.screenHeight / 2, 10, WHITE);
  ::ClearBackground(PURPLE);

  GameState gameState;
  // Main game loop
  while (window.stayOpen()) // Detect window close button or ESC key
  {
    Color highlightColor = colors[colorIndex];
    Color nonHighlightColor = {static_cast<unsigned char>(highlightColor.r / 2), static_cast<unsigned char>(highlightColor.g / 2), static_cast<unsigned char>(highlightColor.b / 2), highlightColor.a};

    ::BeginDrawing();

    if (gameState.screenState == ScreenState::Config)
    {
      drawConfigScreen(config);

      // Example: Adjust paddle speed with up/down
      if (IsKeyPressed(KEY_UP))
        config.paddleSpeed += 1;
      if (IsKeyPressed(KEY_DOWN))
        config.paddleSpeed = std::max(1.0f, config.paddleSpeed - 1); // Ensure paddle speed doesn't go below 1

      // Example: Adjust ball speed with left/right
      if (IsKeyPressed(KEY_LEFT))
        config.ballSpeed = std::max(1.0f, config.ballSpeed - 1); // Ensure ball speed doesn't go below 1
      if (IsKeyPressed(KEY_RIGHT))
        config.ballSpeed += 1;

      if (IsKeyPressed(KEY_ENTER))
      {
        ball.setSpeed({config.ballSpeed, config.ballSpeed}); // Set ball speed
        playerPaddle.setSpeed(config.paddleSpeed);           // Set player paddle speed
        cpuPaddle.setSpeed(config.paddleSpeed);              // Set CPU paddle speed
        gameState.screenState = ScreenState::Playing;
      }

      ::EndDrawing();
      continue;
    }

    if (gameState.screenState == ScreenState::Playing)
    {
      ::ClearBackground(bgColor);
      ::DrawRectangleLinesEx(container, 10, DARKGRAY);

      auto ballCollision = ball.update(container, config.containerThick);
      switch (ballCollision)
      {
      case Ball::CollisionType::Left:                // Cpu Point
        gameState.screenState = ScreenState::Config; // Change state to Paused
        gameState.cpuScore++;
        resetBall(ball, config);
        soundContainer.playSound("gameover");
        break;
      case Ball::CollisionType::Right:               // Player Point
        gameState.screenState = ScreenState::Config; // Change state to Paused
        gameState.playerScore++;
        resetBall(ball, config);
        soundContainer.playSound("yay");
        break;
      case Ball::CollisionType::TopOrBottom:
        colorIndex = (colorIndex + 1) % 21; // Change color on top/bottom collision
        soundContainer.playSound("bounce");
        break;
      case Ball::CollisionType::None:
        break;
      }

      playerPaddle.updatePosition(container.y + config.containerThick, container.y + container.height - config.containerThick);
      cpuPaddle.updatePosition(ball.getY(), container.y + config.containerThick, container.y + container.height - config.containerThick);

      auto playerCollision = CheckCollisionCircleRec(ball.getPos(), ball.getRadius(), playerPaddle.getRect());
      auto cpuCollision = CheckCollisionCircleRec(ball.getPos(), ball.getRadius(), cpuPaddle.getRect());
      if (playerCollision || cpuCollision)
      {
        ball.reverseX();
        soundContainer.playSound("paddleclick"); // Play sound on paddle hit
      }

      // Draw two half rectangles
      Rectangle leftHalf = {container.x, container.y, container.width / 2, container.height};
      Rectangle rightHalf = {container.x + container.width / 2, container.y, container.width / 2, container.height};
      bool ballInLeftHalf = CheckCollisionPointRec(ball.getPos(), leftHalf);
      bool ballInRightHalf = CheckCollisionPointRec(ball.getPos(), rightHalf);

      ::DrawRectangleRec(leftHalf, ballInLeftHalf ? highlightColor : nonHighlightColor);
      ::DrawRectangleRec(rightHalf, ballInRightHalf ? highlightColor : nonHighlightColor);
      playerPaddle.draw();
      cpuPaddle.draw();
      ball.draw();
      drawScore(gameState, config);
    }

    ::EndDrawing();
  }

  return 0;
}