#include "Game.h"

//Private Functions
void Game::initVariables()
{
	this->window = nullptr;
}

void Game::initWindow()
{
	this->videoMode.height = 600;
	this->videoMode.width = 800;
	this->window = new sf::RenderWindow(sf::VideoMode(800, 600), "Please Work", sf::Style::Titlebar | sf::Style::Close);
}

//Constructors / Destructors
Game::Game()
{
	this->initVariables();
	this->initWindow();
}

Game::~Game()
{
	delete this->window;
}

//Functions
void Game::update()
{

}

void Game::render()
{

}
