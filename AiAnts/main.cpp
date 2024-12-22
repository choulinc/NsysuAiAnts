#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <cmath>

using namespace std;

const int SCREEN_WIDTH = 1920;
const int SCREEN_HEIGHT = 1080;
const int FPS = 120;
const int FRAME_DELAY = 1000 / FPS;

//螞蟻角色枚舉
enum AntRole{
    WORKER,
    SOLDIER,
    QUEEN
};

// 食物類別
class Food{
public:
    int x, y;
    bool active;
    
    Food(int x, int y) : x(x), y(y), active(true) {}
    
    void render(SDL_Renderer* renderer){
        if(active){
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect foodRect = {x, y, 5, 5};
            SDL_RenderFillRect(renderer, &foodRect);
        }
    }
};

// 添加旋轉點的輔助函數
void rotatePoint(double& x, double& y, double centerX, double centerY, double angle){
    double dx = x - centerX;
    double dy = y - centerY;
    x = centerX + (dx * cos(angle) - dy * sin(angle));
    y = centerY + (dx * sin(angle) + dy * cos(angle));
}

// 改進的橢圓繪製函數，支持旋轉
void drawFilledRotatedEllipse(SDL_Renderer* renderer, int centerX, int centerY, int width, int height, double angle){
    const int resolution = 36;
    const double increment = 2.0 * M_PI / resolution;
    vector<SDL_Point> points;
    points.reserve(resolution + 1);
    
    // 收集橢圓邊緣的點
    for(double t = 0; t < 2.0 * M_PI; t += increment){
        double x = centerX + width * cos(t);
        double y = centerY + height * sin(t);
        rotatePoint(x, y, centerX, centerY, angle);
        points.push_back({(int)x, (int)y});
    }
    // 閉合多邊形
    points.push_back(points[0]);
    
    // 填充多邊形
    for(int y = centerY - height; y <= centerY + height; y++){
        for(int x = centerX - width; x <= centerX + width; x++){
            double dx = x - centerX;
            double dy = y - centerY;
            // 旋轉點回原始空間進行檢查
            double rx = dx * cos(-angle) - dy * sin(-angle);
            double ry = dx * sin(-angle) + dy * cos(-angle);
            if ((rx*rx)/(width*width) + (ry*ry)/(height*height) <= 1.0) {
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
}

class Ant{
public:
    int x, y;
    int speed;
    AntRole role;
    bool hasFood;
    double angle;
    
    Ant(int x, int y, AntRole role) : 
        x(x), y(y), 
        role(role), 
        hasFood(false),
        speed(2),
        angle((rand() % 360) * M_PI / 180.0) {}
    
    void move(){
        x += cos(angle) * speed;
        y += sin(angle) * speed;
        
        if (x < 0 || x > SCREEN_WIDTH) {
            angle = M_PI - angle;
            x = max(0, min(x, SCREEN_WIDTH));
        }
        if (y < 0 || y > SCREEN_HEIGHT) {
            angle = -angle;
            y = max(0, min(y, SCREEN_HEIGHT));
        }
        
        if (rand() % 100 < 5) {
            angle += (rand() % 60 - 30) * M_PI / 180.0;
        }
    }
    
    void render(SDL_Renderer* renderer){
        // 設置螞蟻顏色
        SDL_SetRenderDrawColor(renderer, 
            role == WORKER ? 0 : (role == SOLDIER ? 255 : 255),
            0,
            role == QUEEN ? 255 : 0,
            255);
        
        // 調整螞蟻的大小
        int baseWidth = role == QUEEN ? 4 : 3;
        int baseHeight = role == QUEEN ? 3 : 2;
        
        // 減小頭部和身體的間距，使其更連貫
        double spacing = baseWidth * 1.2; // 減小間距
        
        // 繪製身體（較大的橢圓）
        double bodyX = x - cos(angle) * spacing * 0.5;
        double bodyY = y - sin(angle) * spacing * 0.5;
        drawFilledRotatedEllipse(renderer, bodyX, bodyY, 
                               baseWidth * 1.2, baseHeight * 1.2, angle);
        
        // 繪製頭部（較小的橢圓）
        double headX = x + cos(angle) * spacing * 0.5;
        double headY = y + sin(angle) * spacing * 0.5;
        drawFilledRotatedEllipse(renderer, headX, headY, 
                               baseWidth, baseHeight, angle);
        
        // 如果攜帶食物，顯示一個小綠點
        if(hasFood){
            SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            SDL_Rect foodRect = {(int)bodyX - 1, (int)bodyY - 1, 2, 2};
            SDL_RenderFillRect(renderer, &foodRect);
        }
    }
};

// 蟻巢類別
class AntNest{
public:
    int x, y;
    int foodCount;
    vector<Ant> ants;
    
    AntNest(int x, int y) : x(x), y(y), foodCount(0){
        // 初始化螞蟻群
        initializeAnts(20);  // 預設20隻螞蟻
    }
    
    void initializeAnts(int count){
        ants.push_back(Ant(x, y, QUEEN));  // 一隻蟻后
        
        for(int i = 0; i < count - 1; i++){
            if (i < count * 0.3) {  // 30%是士兵
                ants.push_back(Ant(x, y, SOLDIER));
            }else{  // 其餘是工蟻
                ants.push_back(Ant(x, y, WORKER));
            }
        }
    }
    
    void addNewAnt(){
        if (foodCount >= 5) {  // 每5個食物可以生產一隻新螞蟻
            ants.push_back(Ant(x, y, WORKER));
            foodCount -= 5;
        }
    }
    
    void render(SDL_Renderer* renderer){
        SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255);
        SDL_Rect nestRect = {x - 15, y - 15, 30, 30};
        SDL_RenderFillRect(renderer, &nestRect);
    }
};

// 遊戲管理類別
class Game{
private:
    SDL_Window* window;
    SDL_Renderer* renderer;
    bool running;
    AntNest nest;
    vector<Food> foods;
    Uint32 frameStart;
    int frameTime;
    
public:
    Game() : 
        window(nullptr), 
        renderer(nullptr), 
        running(false),
        nest(SCREEN_WIDTH/2, SCREEN_HEIGHT/2)  // 蟻巢在畫面中央
    {}
    
    bool init(){
        if(SDL_Init(SDL_INIT_VIDEO) < 0){
            cout << "SDL初始化失敗 錯誤: " << SDL_GetError() << endl;
            return false;
        }
        
        window = SDL_CreateWindow("螞蟻生態模擬",
            SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
            SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
            
        if(!window){
            cout << "無法創建窗口 錯誤: " << SDL_GetError() << endl;
            return false;
        }
        
        renderer = SDL_CreateRenderer(window, -1, 
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
            
        if(!renderer){
            cout << "無法創建渲染器 錯誤: " << SDL_GetError() << endl;
            return false;
        }
        
        running = true;
        return true;
    }
    
    void handleEvents(){
        SDL_Event event;
        while (SDL_PollEvent(&event)){
            if (event.type == SDL_QUIT){
                running = false;
            }
        }
    }
    
    void update(){
        // 更新所有螞蟻
        for(auto& ant : nest.ants){
            ant.move();
            
            // 檢查是否找到食物
            if(!ant.hasFood){
                for(auto& food : foods){
                    if(food.active && 
                        abs(ant.x - food.x) < 5 && 
                        abs(ant.y - food.y) < 5){
                        ant.hasFood = true;
                        food.active = false;
                        break;
                    }
                }
            }
            // 檢查是否回到蟻巢
            else if(abs(ant.x - nest.x) < 15 && abs(ant.y - nest.y) < 15){
                ant.hasFood = false;
                nest.foodCount++;
                nest.addNewAnt();
            }
        }
        
        // 隨機生成食物
        if(rand() % 100 < 10 && foods.size() < 50){  // 10%機率，最多50個食物
            foods.push_back(Food(
                rand() % SCREEN_WIDTH,
                rand() % SCREEN_HEIGHT
            ));
        }
    }
    
    void render(){
        // 清除畫面
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);
        
        // 繪製蟻巢
        nest.render(renderer);
        
        // 繪製食物
        for(auto& food : foods){
            food.render(renderer);
        }
        
        // 繪製螞蟻
        for(auto& ant : nest.ants){
            ant.render(renderer);
        }
        
        SDL_RenderPresent(renderer);
    }
    
    void run(){
        while(running){
            frameStart = SDL_GetTicks();
            
            handleEvents();
            update();
            render();
            
            frameTime = SDL_GetTicks() - frameStart;
            if(FRAME_DELAY > frameTime){
                SDL_Delay(FRAME_DELAY - frameTime);
            }
        }
    }
    
    void clean(){
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
};

int main(int argc, char* args[]){
    srand(time(nullptr));
    
    Game game;
    if(game.init()){
        game.run();
    }
    game.clean();
    
    return 0;
}