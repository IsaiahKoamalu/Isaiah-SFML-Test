#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#include <SFML/Network.hpp>

/*
	Class that acts as the game engine
	Wrapper class.
*/
class Game
{
private:
	//Variables
	//Window
	sf::RenderWindow* window;
	sf::VideoMode videoMode;
	sf:: Event ev;

	//Private Function
	void initVariables();
	void initWindow();

public:
	//Constructors / Destructors
	Game();
	virtual ~Game();

	//Functions
	void update();
	void render();
};

