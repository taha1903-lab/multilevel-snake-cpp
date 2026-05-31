#include <raylib.h>
#include <iostream>
#include <deque>
#include <stack>
#include <queue>
#include <vector>
#include <random>
#include <ctime>
#include <string.h>

// Constants
const int screenWidth = 800;
const int screenHeight = 600;
const int gridSize = 20;
const int gridWidth = screenWidth / gridSize;
const int gridHeight = screenHeight / gridSize;
const float initialSpeed = 0.20f; // Seconds per move
const int maxSpeedLevel = 10;
const float speedIncrement = 0.001f;

// Level constants
const int scorePerLevel = 5;  // Score needed to advance to next level
const int maxLevel = 5;       // Maximum level in the game
const int maxObstacles = 15;  // Maximum number of obstacles at highest level

// Game states
enum class GameState {
    TITLE,
    PLAYING,
    LEVEL_UP,
    GAME_OVER
};

// Direction enum
enum class Direction {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

// Node structure for snake body parts
struct SnakeNode {
    int x;
    int y;
    SnakeNode* prev;
    SnakeNode* next;
    
    SnakeNode(int posX, int posY) : x(posX), y(posY), prev(nullptr), next(nullptr) {}
};

// Food structure
struct Food {
    int x;
    int y;
    int points;  // Different foods can give different points
    Color color; // Different foods can have different colors
    
    Food(int posX, int posY, int pts = 1, Color col = RED) 
        : x(posX), y(posY), points(pts), color(col) {}
};

// Obstacle structure
struct Obstacle {
    int x;
    int y;
    
    Obstacle(int posX, int posY) : x(posX), y(posY) {}
};

// Snake class using a doubly linked list
class Snake {
private:
    SnakeNode* head;
    SnakeNode* tail;
    Direction currentDirection;
    Direction nextDirection;
    std::stack<Direction> directionHistory;
    int length;
    bool isDead;

public:
    Snake(int startX, int startY) {
        head = new SnakeNode(startX, startY);
        tail = head;
        currentDirection = Direction::RIGHT;
        nextDirection = Direction::RIGHT;
        length = 1;
        isDead = false;
        
        // Push initial direction into history
        directionHistory.push(currentDirection);
    }
    
    ~Snake() {
        SnakeNode* current = head;
        while (current != nullptr) {
            SnakeNode* next = current->next;
            delete current;
            current = next;
        }
    }
    
    void setDirection(Direction dir) {
        // Prevent 180-degree turns
        if ((currentDirection == Direction::UP && dir == Direction::DOWN) ||
            (currentDirection == Direction::DOWN && dir == Direction::UP) ||
            (currentDirection == Direction::LEFT && dir == Direction::RIGHT) ||
            (currentDirection == Direction::RIGHT && dir == Direction::LEFT)) {
            return;
        }
        
        nextDirection = dir;
    }
    
    void move() {
        int newX = head->x;
        int newY = head->y;
        
        // Update current direction from next direction
        currentDirection = nextDirection;
        
        // Store direction in history
        directionHistory.push(currentDirection);
        
        // Calculate new head position based on direction
        switch (currentDirection) {
            case Direction::UP:
                newY--;
                break;
            case Direction::DOWN:
                newY++;
                break;
            case Direction::LEFT:
                newX--;
                break;
            case Direction::RIGHT:
                newX++;
                break;
        }
        
        // Check if snake is out of bounds
        if (newX < 0 || newX >= gridWidth || newY < 0 || newY >= gridHeight) {
            isDead = true;
            return;
        }
        
        // Check if snake collides with itself
        SnakeNode* current = head->next;
        while (current != nullptr) {
            if (current->x == newX && current->y == newY) {
                isDead = true;
                return;
            }
            current = current->next;
        }
        
        // Create new head
        SnakeNode* newHead = new SnakeNode(newX, newY);
        newHead->next = head;
        head->prev = newHead;
        head = newHead;
    }
    
    void grow() {
        // Nothing needs to be removed when growing, so the tail stays
        length++;
    }
    
    void grow(int amount) {
        // Grow by specified amount
        length += amount;
    }
    
    void removeTail() {
        if (tail != nullptr) {
            SnakeNode* oldTail = tail;
            tail = tail->prev;
            
            if (tail != nullptr) {
                tail->next = nullptr;
            } else {
                head = nullptr; // Snake has only one node
            }
            
            delete oldTail;
        }
    }
    
    bool checkFoodCollision(const Food& food) const {
        return (head->x == food.x && head->y == food.y);
    }
    
    bool checkObstacleCollision(const std::vector<Obstacle>& obstacles) const {
        for (const auto& obstacle : obstacles) {
            if (head->x == obstacle.x && head->y == obstacle.y) {
                return true;
            }
        }
        return false;
    }
    
    bool isDying() const {
        return isDead;
    }
    
    void kill() {
        isDead = true;
    }
    
    int getLength() const {
        return length;
    }
    
    SnakeNode* getHead() const {
        return head;
    }
    
    Direction getCurrentDirection() const {
        return currentDirection;
    }
    
    // Get the most recent direction from history
    Direction getLastDirection() {
        if (!directionHistory.empty()) {
            return directionHistory.top();
        }
        return Direction::RIGHT; // Default direction
    }
    
    // Get all snake body positions for rendering
    std::deque<Vector2> getBodyPositions() const {
        std::deque<Vector2> positions;
        SnakeNode* current = head;
        
        while (current != nullptr) {
            positions.push_back({static_cast<float>(current->x * gridSize), 
                                static_cast<float>(current->y * gridSize)});
            current = current->next;
        }
        
        return positions;
    }
    
    void reset(int startX, int startY) {
        // Clear the old snake
        SnakeNode* current = head;
        while (current != nullptr) {
            SnakeNode* next = current->next;
            delete current;
            current = next;
        }
        
        // Reset to initial state
        head = new SnakeNode(startX, startY);
        tail = head;
        currentDirection = Direction::RIGHT;
        nextDirection = Direction::RIGHT;
        length = 1;
        isDead = false;
        
        // Clear direction history and add initial direction
        while (!directionHistory.empty()) {
            directionHistory.pop();
        }
        directionHistory.push(currentDirection);
    }
};

// Food manager using a queue
class FoodManager {
private:
    std::queue<Food> foodQueue;
    std::mt19937 rng;
    int currentLevel;

public:
    FoodManager() : currentLevel(1) {
        // Seed the random number generator
        rng.seed(static_cast<unsigned int>(time(nullptr)));
    }
    
    void setLevel(int level) {
        currentLevel = level;
    }
    
    void generateFood(const Snake& snake, const std::vector<Obstacle>& obstacles) {
        int x, y;
        bool validPosition;
        
        do {
            validPosition = true;
            x = std::uniform_int_distribution<int>(0, gridWidth - 1)(rng);
            y = std::uniform_int_distribution<int>(0, gridHeight - 1)(rng);
            
            // Check if position overlaps with snake body
         //   auto positions = snake.getBodyPositions();
            //for (const auto& pos : positions) {
               // if (x == static_cast<int>(pos.x / gridSize) && 
                 //   y == static_cast<int>(pos.y / gridSize)) {
                   // validPosition = false;
                   // break;
                //}
            //}
            
            // Check if position overlaps with obstacles
            for (const auto& obstacle : obstacles) {
                if (x == obstacle.x && y == obstacle.y) {
                    validPosition = false;
                    break;
                }
            }
        } while (!validPosition);
        
        // Determine food type based on level and randomness
        int pointValue = 1;
        Color foodColor = RED;
        
        // 15% chance of special food at levels 2+
        if (currentLevel >= 2 && std::uniform_int_distribution<int>(1, 100)(rng) <= 15) {
            pointValue = 2;
            foodColor = GOLD;
        }
        
        // 10% chance of super food at levels 4+
        if (currentLevel >= 4 && std::uniform_int_distribution<int>(1, 100)(rng) <= 10) {
            pointValue = 3;
            foodColor = PURPLE;
        }
        
        // Add food to queue
        foodQueue.push(Food(x, y, pointValue, foodColor));
        
        // Keep only one food at a time
        if (foodQueue.size() > 1) {
            foodQueue.pop();
        }
    }
    
    Food getCurrentFood() const {
        if (foodQueue.empty()) {
            // Default food position if queue is empty (shouldn't happen)
            return Food(5, 5);
        }
        return foodQueue.front();
    }
};

// Obstacle manager
class ObstacleManager {
private:
    std::vector<Obstacle> obstacles;
    std::mt19937 rng;
    
public:
    ObstacleManager() {
        // Seed the random number generator
        rng.seed(static_cast<unsigned int>(time(nullptr)));
    }
    
    void generateObstacles(int level, const Snake& snake) {
        // Clear existing obstacles
        obstacles.clear();
        
        // Calculate number of obstacles based on level
        int numObstacles = (level - 1) * 3; // 0 at level 1, 3 at level 2, etc.
        if (numObstacles > maxObstacles) {
            numObstacles = maxObstacles;
        }
        
        // Generate obstacles
        for (int i = 0; i < numObstacles; i++) {
            addObstacle(snake);
        }
        
        // Add border obstacles at higher levels (level 3+)
        if (level >= 3) {
            addBorderObstacles(level);
        }
    }
    
    void addObstacle(const Snake& snake) {
        int x, y;
        bool validPosition;
        
        do {
            validPosition = true;
            x = std::uniform_int_distribution<int>(0, gridWidth - 1)(rng);
            y = std::uniform_int_distribution<int>(0, gridHeight - 1)(rng);
            
            // Don't place obstacles too close to the snake's head
            SnakeNode* head = snake.getHead();
            int safeDistance = 5;
            if (std::abs(x - head->x) < safeDistance && std::abs(y - head->y) < safeDistance) {
                validPosition = false;
                continue;
            }
            
            // Check if position overlaps with snake body
            auto positions = snake.getBodyPositions();
            for (const auto& pos : positions) {
                if (x == static_cast<int>(pos.x / gridSize) && 
                    y == static_cast<int>(pos.y / gridSize)) {
                    validPosition = false;
                    break;
                }
            }
            
            // Check if position overlaps with existing obstacles
            for (const auto& obstacle : obstacles) {
                if (x == obstacle.x && y == obstacle.y) {
                    validPosition = false;
                    break;
                }
            }
        } while (!validPosition);
        
        obstacles.push_back(Obstacle(x, y));
    }
    
    void addBorderObstacles(int level) {
        // Different border patterns for different levels
        switch (level) {
            case 3: {
                // Level 3: Add simple corners
                int cornerSize = 3;
                
                // Top-left corner
                for (int i = 0; i < cornerSize; i++) {
                    for (int j = 0; j < cornerSize - i; j++) {
                        obstacles.push_back(Obstacle(i, j));
                    }
                }
                
                // Top-right corner
                for (int i = 0; i < cornerSize; i++) {
                    for (int j = 0; j < cornerSize - i; j++) {
                        obstacles.push_back(Obstacle(gridWidth - 1 - i, j));
                    }
                }
                
                // Bottom-left corner
                for (int i = 0; i < cornerSize; i++) {
                    for (int j = 0; j < cornerSize - i; j++) {
                        obstacles.push_back(Obstacle(i, gridHeight - 1 - j));
                    }
                }
                
                // Bottom-right corner
                for (int i = 0; i < cornerSize; i++) {
                    for (int j = 0; j < cornerSize - i; j++) {
                        obstacles.push_back(Obstacle(gridWidth - 1 - i, gridHeight - 1 - j));
                    }
                }
                break;
            }
            case 4: {
                // Level 4: Add center obstacle
                int centerX = gridWidth / 2;
                int centerY = gridHeight / 2;
                int size = 3;
                
                for (int i = -size/2; i <= size/2; i++) {
                    for (int j = -size/2; j <= size/2; j++) {
                        obstacles.push_back(Obstacle(centerX + i, centerY + j));
                    }
                }
                break;
            }
            case 5: {
                // Level 5: Add maze-like pattern
                // Horizontal bars
                int barY1 = gridHeight / 3;
                int barY2 = gridHeight * 2 / 3;
                int gap = 5;
                
                for (int x = 0; x < gridWidth; x++) {
                    if (x % (gap * 2) < gap) {
                        obstacles.push_back(Obstacle(x, barY1));
                    } else {
                        obstacles.push_back(Obstacle(x, barY2));
                    }
                }
                break;
            }
        }
    }
    
    const std::vector<Obstacle>& getObstacles() const {
        return obstacles;
    }
    
    void clear() {
        obstacles.clear();
    }
};

// Game class
class Game {
private:
    GameState state;
    Snake snake;
    FoodManager foodManager;
    ObstacleManager obstacleManager;
    float moveTimer;
    float currentSpeed;
    int score;
    int highScore;
    int currentLevel;
    float levelUpTimer;
    
public:
    Game() : snake(gridWidth / 2, gridHeight / 2), 
             state(GameState::TITLE),
             moveTimer(0.0f),
             currentSpeed(initialSpeed),
             score(0),
             highScore(0),
             currentLevel(1),
             levelUpTimer(0.0f) {
        // Initialize Raylib
        InitWindow(screenWidth, screenHeight, "Snake Game with Levels & Obstacles");
        SetTargetFPS(60);
        
        // Generate initial food
        foodManager.generateFood(snake, obstacleManager.getObstacles());
    }
    
    ~Game() {
        CloseWindow();
    }
    
    void run() {
        while (!WindowShouldClose()) {
            update();
            draw();
        }
    }
    
    void update() {
        switch (state) {
            case GameState::TITLE:
                updateTitleScreen();
                break;
            case GameState::PLAYING:
                updateGameplay();
                break;
            case GameState::LEVEL_UP:
                updateLevelUp();
                break;
            case GameState::GAME_OVER:
                updateGameOver();
                break;
        }
    }
    
    void updateTitleScreen() {
        if (IsKeyPressed(KEY_SPACE)) {
            state = GameState::PLAYING;
            resetGame();
        }
    }
    
    void updateLevelUp() {
        levelUpTimer += GetFrameTime();
        
        // Show level up screen for 2 seconds
        if (levelUpTimer >= 2.0f || IsKeyPressed(KEY_SPACE)) {
            state = GameState::PLAYING;
            levelUpTimer = 0.0f;
            
            // Generate new obstacles for the level
            obstacleManager.generateObstacles(currentLevel, snake);
            
            // Update food manager with new level
            foodManager.setLevel(currentLevel);
            
            // Generate new food
            foodManager.generateFood(snake, obstacleManager.getObstacles());
        }
    }
    
    void updateGameplay() {
        // Handle input
        if (IsKeyPressed(KEY_UP)) {
            snake.setDirection(Direction::UP);
        } else if (IsKeyPressed(KEY_DOWN)) {
            snake.setDirection(Direction::DOWN);
        } else if (IsKeyPressed(KEY_LEFT)) {
            snake.setDirection(Direction::LEFT);
        } else if (IsKeyPressed(KEY_RIGHT)) {
            snake.setDirection(Direction::RIGHT);
        }
        
        // Update movement timer
        moveTimer += GetFrameTime();
        
        // Move snake if enough time has passed
        if (moveTimer >= currentSpeed) {
            moveTimer = 0;
            snake.move();
            
            // Check for obstacle collision
            if (snake.checkObstacleCollision(obstacleManager.getObstacles())) {
                snake.kill();
                state = GameState::GAME_OVER;
                
                // Update high score
                if (score > highScore) {
                    highScore = score;
                }
                return;
            }
            
            // Check if food was eaten
            if (snake.checkFoodCollision(foodManager.getCurrentFood())) {
                // Get food properties
                Food currentFood = foodManager.getCurrentFood();
                
                // Grow snake based on food value
                snake.grow(currentFood.points);
                
                // Increase score based on food value
                score += currentFood.points;
                
                // Generate new food
                foodManager.generateFood(snake, obstacleManager.getObstacles());
                
                // Check for level up
                int calculatedLevel = (score / scorePerLevel) + 1;
                if (calculatedLevel > currentLevel && calculatedLevel <= maxLevel) {
                    currentLevel = calculatedLevel;
                    state = GameState::LEVEL_UP;
                }
                
                // Speed up the game slightly
                if (currentSpeed > initialSpeed - (speedIncrement * maxSpeedLevel)) {
                    currentSpeed -= speedIncrement;
                }
            } else {
                // Remove tail if no food was eaten
                snake.removeTail();
            }
            
            // Check if snake died
            if (snake.isDying()) {
                state = GameState::GAME_OVER;
                
                // Update high score
                if (score > highScore) {
                    highScore = score;
                }
            }
        }
    }
    
    void updateGameOver() {
        if (IsKeyPressed(KEY_R)) {
            state = GameState::PLAYING;
            resetGame();
        } else if (IsKeyPressed(KEY_ESCAPE)) {
            state = GameState::TITLE;
        }
    }
    
    void draw() {
        BeginDrawing();
        ClearBackground(BLACK);
        
        switch (state) {
            case GameState::TITLE:
                drawTitleScreen();
                break;
            case GameState::PLAYING:
                drawGameplay();
                break;
            case GameState::LEVEL_UP:
                drawLevelUp();
                break;
            case GameState::GAME_OVER:
                drawGameOver();
                break;
        }
        
        EndDrawing();
    }
    
    void drawTitleScreen() {
        const char* title = "SNAKE GAME";
        const char* subtitle = "With Levels and Obstacles";
        const char* instructions = "Press SPACE to Start";
        
        DrawText(title, screenWidth / 2 - MeasureText(title, 40) / 2, 
                 screenHeight / 4, 40, GREEN);
        DrawText(subtitle, screenWidth / 2 - MeasureText(subtitle, 20) / 2, 
                 screenHeight / 4 + 50, 20, WHITE);
        DrawText(instructions, screenWidth / 2 - MeasureText(instructions, 30) / 2, 
                 screenHeight / 2 + 50, 30, YELLOW);
        
        // Draw feature list
        const char* features[] = {
            "- Multiple Difficulty Levels",
            "- Increasing Obstacles",
            "- Special Food Items",
            "- Unique Level Layouts"
        };
        
        for (int i = 0; i < 4; i++) {
            DrawText(features[i], screenWidth / 2 - MeasureText(features[i], 18) / 2, 
                     screenHeight / 2 + 100 + i * 30, 18, LIGHTGRAY);
        }
    }
    
    void drawLevelUp() {
        char levelText[50];
        sprintf(levelText, "LEVEL %d", currentLevel);
        
        DrawText(levelText, screenWidth / 2 - MeasureText(levelText, 60) / 2, 
                 screenHeight / 2 - 30, 60, GREEN);
        
        char descText[100];
        switch (currentLevel) {
            case 2:
                strcpy(descText, "Obstacles appear!");
                break;
            case 3:
                strcpy(descText, "Corner obstacles!");
                break;
            case 4:
                strcpy(descText, "Center obstacles!");
                break;
            case 5:
                strcpy(descText, "Maze pattern - Good luck!");
                break;
            default:
                strcpy(descText, "Get ready for the next challenge!");
        }
        
        DrawText(descText, screenWidth / 2 - MeasureText(descText, 24) / 2, 
                 screenHeight / 2 + 40, 24, YELLOW);
        
        const char* continueText = "Press SPACE to continue";
        DrawText(continueText, screenWidth / 2 - MeasureText(continueText, 20) / 2, 
                 screenHeight * 3 / 4, 20, WHITE);
    }
    
    void drawGameplay() {
        // Draw grid lines (optional)
        for (int i = 0; i < gridWidth; i++) {
            DrawLine(i * gridSize, 0, i * gridSize, screenHeight, Color{20, 20, 20, 255});
        }
        for (int i = 0; i < gridHeight; i++) {
            DrawLine(0, i * gridSize, screenWidth, i * gridSize, Color{20, 20, 20, 255});
        }
        
        // Draw obstacles
        const auto& obstacles = obstacleManager.getObstacles();
        for (const auto& obstacle : obstacles) {
            // Different colors for different obstacles based on level
            Color obstacleColor;
            switch (currentLevel) {
                case 2:
                    obstacleColor = DARKGRAY;
                    break;
                case 3:
                    obstacleColor = BROWN;
                    break;
                case 4:
                    obstacleColor = DARKBLUE;
                    break;
                case 5:
                    obstacleColor = DARKPURPLE;
                    break;
                default:
                    obstacleColor = DARKGRAY;
            }
            
            DrawRectangle(obstacle.x * gridSize, obstacle.y * gridSize, 
                         gridSize, gridSize, obstacleColor);
        }
        
        // Draw food
        Food food = foodManager.getCurrentFood();
        DrawRectangle(food.x * gridSize, food.y * gridSize, gridSize, gridSize, food.color);
        
        // Add a small indicator for special food
        if (food.points > 1) {
            int indicatorSize = gridSize / 4;
            DrawCircle(food.x * gridSize + gridSize / 2, food.y * gridSize + gridSize / 2, 
                      indicatorSize, WHITE);
        }
        
        // Draw snake
        auto bodyPositions = snake.getBodyPositions();
        for (size_t i = 0; i < bodyPositions.size(); i++) {
            Color snakeColor = (i == 0) ? GREEN : Color{0, 200, 0, 255};
            DrawRectangle(bodyPositions[i].x, bodyPositions[i].y, gridSize, gridSize, snakeColor);
            
            // Draw eyes on the head
            if (i == 0) {
                // Calculate eye positions based on direction
                int eyeSize = gridSize / 5;
                int eyeOffset = gridSize / 3;
                
                Vector2 leftEye, rightEye;
                Direction dir = snake.getCurrentDirection();
                
                switch (dir) {
                    case Direction::UP:
                        leftEye = {bodyPositions[i].x + eyeOffset, bodyPositions[i].y + eyeOffset};
                        rightEye = {bodyPositions[i].x + gridSize - eyeOffset - eyeSize, bodyPositions[i].y + eyeOffset};
                        break;
                    case Direction::DOWN:
                        leftEye = {bodyPositions[i].x + eyeOffset, bodyPositions[i].y + gridSize - eyeOffset - eyeSize};
                        rightEye = {bodyPositions[i].x + gridSize - eyeOffset - eyeSize, bodyPositions[i].y + gridSize - eyeOffset - eyeSize};
                        break;
                    case Direction::LEFT:
                        leftEye = {bodyPositions[i].x + eyeOffset, bodyPositions[i].y + eyeOffset};
                        rightEye = {bodyPositions[i].x + eyeOffset, bodyPositions[i].y + gridSize - eyeOffset - eyeSize};
                        break;
                    case Direction::RIGHT:
                        leftEye = {bodyPositions[i].x + gridSize - eyeOffset - eyeSize, bodyPositions[i].y + eyeOffset};
                        rightEye = {bodyPositions[i].x + gridSize - eyeOffset - eyeSize, bodyPositions[i].y + gridSize - eyeOffset - eyeSize};
                        break;
                }
                
                DrawRectangle(leftEye.x, leftEye.y, eyeSize, eyeSize, BLACK);
                DrawRectangle(rightEye.x, rightEye.y, eyeSize, eyeSize, BLACK);
            }
        }
        
        // Draw score and level information
        char scoreText[50];
        sprintf(scoreText, "Score: %d", score);
        DrawText(scoreText, 10, 10, 20, WHITE);
        
        char levelText[50];
        sprintf(levelText, "Level: %d", currentLevel);
        DrawText(levelText, 10, 40, 20, YELLOW);
        
        char nextLevelText[50];
        int pointsToNextLevel = ((currentLevel) * scorePerLevel) - score;
        if (currentLevel < maxLevel) {
            sprintf(nextLevelText, "Next level: %d points", pointsToNextLevel);
            DrawText(nextLevelText, 10, 70, 16, LIGHTGRAY);
        } else {
            sprintf(nextLevelText, "MAX LEVEL");
            DrawText(nextLevelText, 10, 70, 16, GOLD);
        }
    }
    
    void drawGameOver() {
        const char* gameOverText = "GAME OVER";
        char finalScoreText[50];
        char highScoreText[50];
        char levelReachedText[50];
        const char* instructionsText = "Press R to Restart or ESC for Title Screen";
        
        sprintf(finalScoreText, "Score: %d", score);
        sprintf(highScoreText, "High Score: %d", highScore);
        sprintf(levelReachedText, "Level Reached: %d", currentLevel);
        
        DrawText(gameOverText, screenWidth / 2 - MeasureText(gameOverText, 40) / 2, 
                 screenHeight / 4, 40, RED);
        DrawText(finalScoreText, screenWidth / 2 - MeasureText(finalScoreText, 30) / 2, 
                 screenHeight / 2 - 50, 30, WHITE);
        DrawText(highScoreText, screenWidth / 2 - MeasureText(highScoreText, 30) / 2, 
                 screenHeight / 2, 30, YELLOW);
        DrawText(levelReachedText, screenWidth / 2 - MeasureText(levelReachedText, 24) / 2, 
                 screenHeight / 2 + 50, 24, GREEN);
        DrawText(instructionsText, screenWidth / 2 - MeasureText(instructionsText, 20) / 2, 
                 screenHeight * 3 / 4, 20, GRAY);
    }
    
    void resetGame() {
        snake.reset(gridWidth / 2, gridHeight / 2);
        obstacleManager.clear();
        foodManager.setLevel(1);
        foodManager.generateFood(snake, obstacleManager.getObstacles());
        moveTimer = 0.0f;
        currentSpeed = initialSpeed;
        score = 0;
        currentLevel = 1;
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}