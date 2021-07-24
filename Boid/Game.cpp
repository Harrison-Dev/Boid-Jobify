#include <iostream>
#include "Flock.h"
#include "Boid.h"
#include "Pvector.h"
#include "Game.h"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"

#include "ftl/task_counter.h"
#include "ftl/task_scheduler.h"

const int kBoidNum = 2000;
const bool isJob = true;

struct FlockBirdSet {
	FlockBirdSet() {}
	int32_t index;
	Flock* flock;
};

// Construct window using SFML
Game::Game()
{
    this->boidsSize = 3.0;
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    this->window_height = desktop.height;
    this->window_width  = desktop.width;
    this->window.create(sf::VideoMode(window_width, window_height, desktop.bitsPerPixel), "Boids", sf::Style::Default);
}

// Run the simulation. Run creates the boids that we'll display, checks for user
// input, and updates the view
void Game::Run()
{
    for (int i = 0; i < kBoidNum; i++) {
        Boid b(window_width / 3, window_height / 3); // Starts all boids in the center of the screen
        sf::CircleShape shape(8, 3);

        // Changing the Visual Properties of the shape
        // shape.setPosition(b.location.x, b.location.y); // Sets position of shape to random location that boid was set to.
        shape.setPosition(window_width, window_height); // Testing purposes, starts all shapes in the center of screen.
        shape.setOutlineColor(sf::Color(0,255,0));
        shape.setFillColor(sf::Color::Green);
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(1);
        shape.setRadius(boidsSize);

        // Adding the boid to the flock and adding the shapes to the vector<sf::CircleShape>
        flock.addBoid(b);
        shapes.push_back(shape);
    }
	
	// Load Font
	sf::Font font;
	auto s0 = "consola.ttf";
	if (!font.loadFromFile(s0))
	{
		std::cout << "Fail to load " << s0 << std::endl;
		return;
	}

	// Setup Text
	sf::Text fpsText("Frames per Second: ", font);
	fpsText.setFillColor(sf::Color::Red);
	fpsText.setCharacterSize(16);
	//fpsText.setPosition(window_width - 230, 0);

	// Clock for fps
	sf::Clock fpsClock;

	ftl::TaskScheduler taskScheduler;
	taskScheduler.Init();

    while (window.isOpen()) {
        Update(&taskScheduler,fpsClock, fpsText);
    }
}

void Game::DrawUI(sf::Clock &fpsClock, sf::Text &text, float drawBoidTime, float flockTime)
{
	float currentTime = fpsClock.restart().asSeconds();
	float fps = 1 / currentTime;
	text.setString("Frames per Second: " + to_string(int(fps + 0.5)));
	text.setPosition(window_width - 230, 0);
	window.draw(text);
	text.setString("Draw Boid Time: " + to_string(drawBoidTime));
	text.setPosition(window_width - 230, 20);
	window.draw(text);
	text.setString("Flocking Cal Time: " + to_string(flockTime));
	text.setPosition(window_width - 230, 40);
	window.draw(text);
}

void Game::HandleInput()
{
    sf::Event event;
    while (window.pollEvent(event)) {
        // "close requested" event: we close the window
        // Implemented alternate ways to close the window. (Pressing the escape, X, and BackSpace key also close the program.)
        if ((event.type == sf::Event::Closed) ||
            (event.type == sf::Event::KeyPressed &&
             event.key.code == sf::Keyboard::Escape) ||
            (event.type == sf::Event::KeyPressed &&
             event.key.code == sf::Keyboard::BackSpace) ||
            (event.type == sf::Event::KeyPressed &&
             event.key.code == sf::Keyboard::X))
             {
            window.close();
        }
    }

    // Check for mouse click, draws and adds boid to flock if so.
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
        // Gets mouse coordinates, sets that as the location of the boid and the shape
        sf::Vector2i mouseCoords = sf::Mouse::getPosition(window);
        Boid b(mouseCoords.x, mouseCoords.y, false);
        sf::CircleShape shape(10,3);

        // Changing visual properties of newly created boid
        shape.setPosition(mouseCoords.x, mouseCoords.y);
        shape.setOutlineColor(sf::Color(255, 0, 0));
        shape.setFillColor(sf::Color(255, 0, 0));
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(1);
        shape.setRadius(boidsSize);

        // Adds newly created boid and shape to their respective data structure
        flock.addBoid(b);
        shapes.push_back(shape);

        // New Shape is drawn
        window.draw(shapes[shapes.size()-1]);
    }
}

void Game::FlyBird(ftl::TaskScheduler * taskScheduler, void * arg)
{
	(void)taskScheduler;
	FlockBirdSet *flockSet = reinterpret_cast<FlockBirdSet *>(arg);
	flockSet->flock->flocking(flockSet->index);
}

void Game::Update(ftl::TaskScheduler * scheduler, sf::Clock &fpsClock, sf::Text &fpsText)
{
    window.clear();
	HandleInput();
    
	sf::Clock inFrameClock;

	// Draws all of the Boids out, and applies functions that are needed to update.
	DrawBoid();
	float drawBoidTime = inFrameClock.restart().asSeconds();

    // Applies the three rules to each boid in the flock and changes them accordingly.

	//MainThreadFlocking();
	JobFlocking(scheduler);

	float flockTime = inFrameClock.restart().asSeconds();

	DrawUI(fpsClock, fpsText, drawBoidTime, flockTime);

	window.display();
}

void Game::MainThreadFlocking()
{
	for (int i = 0; i < kBoidNum; i++) {
		flock.flocking(i);
	}
}

void Game::JobFlocking(ftl::TaskScheduler * scheduler)
{
	window.setActive(false);

	ftl::Task tasks[kBoidNum];
	FlockBirdSet flockSet[kBoidNum];

	for (int i = 0; i < kBoidNum; ++i)
	{
		FlockBirdSet *iFlockSet = &flockSet[i];

		iFlockSet->flock = &flock;
		iFlockSet->index = i;

		tasks[i] = { FlyBird, iFlockSet };
	}

	ftl::TaskCounter counter(scheduler);
	scheduler->AddTasks(kBoidNum, tasks, ftl::TaskPriority::Normal, &counter);
	scheduler->WaitForCounter(&counter);

	window.setActive(true);
}

void Game::DrawBoid()
{
	for (int i = 0; i < shapes.size(); i++) 
	{

		// Matches up the location of the shape to the boid
		shapes[i].setPosition(flock.getBoid(i).location.x, flock.getBoid(i).location.y);

		// Calculates the angle where the velocity is pointing so that the triangle turns towards it.
		float theta = flock.getBoid(i).angle(flock.getBoid(i).velocity);
		shapes[i].setRotation(theta);

		// Prevent boids from moving off the screen through wrapping
		// If boid exits right boundary
		if (shapes[i].getPosition().x > window_width)
			shapes[i].setPosition(shapes[i].getPosition().x - window_width, shapes[i].getPosition().y);
		// If boid exits bottom boundary
		if (shapes[i].getPosition().y > window_height)
			shapes[i].setPosition(shapes[i].getPosition().x, shapes[i].getPosition().y - window_height);
		// If boid exits left boundary
		if (shapes[i].getPosition().x < 0)
			shapes[i].setPosition(shapes[i].getPosition().x + window_width, shapes[i].getPosition().y);
		// If boid exits top boundary
		if (shapes[i].getPosition().y < 0)
			shapes[i].setPosition(shapes[i].getPosition().x, shapes[i].getPosition().y + window_height);
	}

	for (int i = 0; i < shapes.size(); i++) 
	{
		window.draw(shapes[i]);
	}
}
