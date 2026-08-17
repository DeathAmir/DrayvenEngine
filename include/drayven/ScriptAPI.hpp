#pragma once
#include <cmath>
#include <iostream>
#include <random>
#include <string>
#include <string_view>

namespace drayven::script {
struct Value {
    double number{0};
    Value()=default; Value(double v):number(v){} Value(int v):number(v){} Value(bool v):number(v?1.0:0.0){}
    operator double() const { return number; }
};
inline void log(std::string_view text){ std::cout << "[DRYS] " << text << '\n'; }
inline bool keyDown(std::string_view){ return false; }
inline bool keyPressed(std::string_view){ return false; }
inline void moveX(double){}
inline void moveY(double){}
inline void setPosition(double,double,double=0){}
inline void playSound(std::string_view){}
inline void spawn(std::string_view){}
inline void destroy(std::string_view){}
inline double deltaTime(){ return 1.0/60.0; }
inline double random(double min,double max){ static std::mt19937_64 rng{std::random_device{}()}; std::uniform_real_distribution<double>d(min,max); return d(rng); }
}
