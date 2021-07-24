#include <iostream>
#include "Flock.h"
#include "Boid.h"
#include "Pvector.h"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"

#ifndef GAME_H
#define GAME_H

// Game handles the instantiation of a flock of boids, game input, asks the
// model to compute the next step in the stimulation, and handles all of the
// program's interaction with SFML. 

class Game {
private:
    sf::RenderWindow window;
    int window_width;
    int window_height;

    Flock flock;
    float boidsSize;
    vector<sf::CircleShape> shapes;

	void Update(sf::Clock &clock, sf::Text &fpsText);
	void DrawBoid();
    void HandleInput();

public:
    Game();
    void Run();
	void DrawUI(sf::Clock &fpsClock, sf::Text &fpsText, float drawBoidTime, float flockTime);
};

#endif
