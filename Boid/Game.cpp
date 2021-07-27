#include <iostream>
#include "Flock.h"
#include "Boid.h"
#include "Pvector.h"
#include "Game.h"
#include "SFML/Window.hpp"
#include "SFML/Graphics.hpp"

#include "ftl/task_counter.h"
#include "ftl/task_scheduler.h"

enum DemoType
{
	MainThreadOnly = 0,
	MainThreadJobify = 1,
	KickNowAndGatherLater = 2,
	DoubleBuffering = 3, // Fix kick now and gather later race condition
	TimeSlicing = 4,
};

int m_demoType = KickNowAndGatherLater;
const int kBoidNum = 1500;

enum InframeState
{
	ReadyToNext = 0,
	LogicFin = 1,
	RenderFin = 2,
};

float m_flockingTime = 0;
atomic<int> m_inFrameState = 0;

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
	
	switch (m_demoType)
	{
		case KickNowAndGatherLater:
		case DoubleBuffering:
		{
			// deactivate its OpenGL context
			window.setActive(false);

			// launch the rendering thread
			sf::Thread thread(
				&SetupRenderThread,
				this);
			thread.launch();

			ftl::TaskScheduler taskScheduler;
			taskScheduler.Init();
			while (window.isOpen()) {
				Update(taskScheduler);
			}
		}
			break;
		case MainThreadOnly:
		case MainThreadJobify:
		{
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

			// Clock for fps
			sf::Clock fpsClock;

			ftl::TaskScheduler taskScheduler;
			taskScheduler.Init();
			while (window.isOpen()) {
				Render(this, &window, fpsClock, fpsText);
				Update(taskScheduler);
			}
		}
		break;
	}
}

void Game::SetupRenderThread(Game* currentInstance)
{
	sf::RenderWindow* window = &(currentInstance->window);

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

	// Clock for fps
	sf::Clock fpsClock;

	// the rendering loop
	while (window->isOpen())
	{
		// draw...
		Render(currentInstance, window, fpsClock, fpsText);
	}
}

void Game::Update(ftl::TaskScheduler &taskScheduler)
{
	if(m_demoType == KickNowAndGatherLater || m_demoType == DoubleBuffering)
		m_inFrameState = ReadyToNext;
	
	HandleInput();

	sf::Clock gameLogicClock;

	switch (m_demoType)
	{
		case MainThreadOnly:
			MainThreadFlocking();
			break;
		case MainThreadJobify:
			window.setActive(false);
			JobFlocking(&taskScheduler);
			window.setActive(true);
			break;
		case KickNowAndGatherLater:
		case DoubleBuffering:
			JobFlocking(&taskScheduler);
			break;
	}

	m_flockingTime = gameLogicClock.restart().asSeconds();

	if (m_demoType == KickNowAndGatherLater || m_demoType == DoubleBuffering)
	{
		if (m_demoType == DoubleBuffering)
		{
			flockClone = flock; // Deep copy flock for render
		}
		m_inFrameState = LogicFin;
		while (m_inFrameState != RenderFin); // wait next state
	}
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

	return;

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

void Game::MainThreadFlocking()
{
	for (int i = 0; i < kBoidNum; i++) {
		flock.flocking(i);
	}
}

void Game::JobFlocking(ftl::TaskScheduler * scheduler)
{
	ftl::Task tasks[kBoidNum];
	FlockBirdSet flockSet[kBoidNum];

	for (int i = 0; i < kBoidNum; ++i)
	{
		FlockBirdSet *iFlockSet = &flockSet[i];

		iFlockSet->flock = &flock;
		iFlockSet->index = i;

		tasks[i] = { FlyBirdTask, iFlockSet };
	}

	ftl::TaskCounter counter(scheduler);
	scheduler->AddTasks(kBoidNum, tasks, ftl::TaskPriority::Normal, &counter);
	scheduler->WaitForCounter(&counter);
}

void Game::FlyBirdTask(ftl::TaskScheduler * taskScheduler, void * arg)
{
	(void)taskScheduler;
	FlockBirdSet *flockSet = reinterpret_cast<FlockBirdSet *>(arg);
	flockSet->flock->flocking(flockSet->index);
}

void Game::Render(Game* current, sf::RenderWindow* window, sf::Clock &fpsClock, sf::Text &fpsText)
{
    window->clear();
    
	sf::Clock renderClock;

	// Draws all of the Boids out, and applies functions that are needed to update.
	current->DrawBoid();
	float drawBoidTime = renderClock.restart().asSeconds();

	if (m_demoType == KickNowAndGatherLater || m_demoType == DoubleBuffering)
	{
		while (m_inFrameState != LogicFin);// Wait logic frame
		m_inFrameState = RenderFin;
	}

	current->DrawUI(fpsClock, fpsText, drawBoidTime, m_flockingTime);
	window->display();
}

void Game::DrawBoid()
{
	Flock renderFlock = (m_demoType == DoubleBuffering) ? flockClone : flock;

	if (renderFlock.getSize() == 0)
		return;

	for (int i = 0; i < shapes.size(); i++) 
	{
		// Matches up the location of the shape to the boid
		shapes[i].setPosition(renderFlock.getBoid(i).location.x, renderFlock.getBoid(i).location.y);

		// Calculates the angle where the velocity is pointing so that the triangle turns towards it.
		float theta = renderFlock.getBoid(i).angle(renderFlock.getBoid(i).velocity);
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

void Game::DrawUI(sf::Clock &fpsClock, sf::Text &text, float drawBoidTime, float flockTime)
{
	float currentTime = fpsClock.restart().asSeconds();
	float fps = 1 / currentTime;
	float textPosX = window_width - 250;
	text.setString("Frames per Second: " + to_string(int(fps + 0.5)));
	text.setPosition(textPosX, 0);
	window.draw(text);
	text.setString("Frames Time: " + to_string(currentTime));
	text.setPosition(textPosX, 20);
	window.draw(text);
	text.setString("Draw Boid Time: " + to_string(drawBoidTime));
	text.setPosition(textPosX, 40);
	window.draw(text);
	text.setString("Flocking Cal Time: " + to_string(flockTime));
	text.setPosition(textPosX, 60);
	window.draw(text);
}
