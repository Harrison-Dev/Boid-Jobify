#include <iostream>
#include "Flock.h"
#include "Boid.h"
#include "Pvector.h"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"

#include "ftl/task_counter.h"
#include "ftl/task_scheduler.h"

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

	static inline void Render(Game* current, sf::RenderWindow* window,sf::Clock &clock, sf::Text &fpsText);
	void MainThreadFlocking();
	inline void JobFlocking(ftl::TaskScheduler * scheduler);
	void DrawBoid();
    void HandleInput();
	static void SetupRenderThread(Game* currentInstance);

public:
    Game();
    void Run();
	inline void Update(ftl::TaskScheduler &taskScheduler);
	void DrawUI(sf::Clock &fpsClock, sf::Text &fpsText, float drawBoidTime, float flockTime);
	static void FlyBirdTask(ftl::TaskScheduler *taskScheduler, void *arg);
};

#endif
