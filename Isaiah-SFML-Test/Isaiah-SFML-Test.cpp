#include <iostream>
#include <vector>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <string>
#include "Game.h"

using namespace std;
using namespace sf;

static bool hasKey = false;
static bool displayInventory = false;
static int spawnPoint = 0; // 0 = Main Chamber, 1 = Control Room, 2 = Security Room, 3 = Analysis Lab

//-----------Handling the dialogue system------------------|

//Struct for dialogue nodes
struct DialogueNode{
	std::string id;
	std::string speaker;
	std::string text;
};

static std::vector<DialogueNode> dialogueNodes;

//Split lines in .tsv file by tabs
std::vector<std::string> splitTSVLine(const std::string& line) {
	std::vector<std::string> parts;
	std::stringstream ss(line);
	std::string item;
	while (std::getline(ss, item, '\t')) { parts.push_back(item); }
	return parts;
}

//Load dialogue from .tsv file
std::vector<DialogueNode> loadDialogue(std::string filePath) {
	std::fstream file(filePath);

	if (!file.is_open()) {
		std::cerr << "Could not open file: " << filePath << std::endl;
		return dialogueNodes;
	}

	std::string line;
	while (std::getline(file, line)) {
		auto parts = splitTSVLine(line);
		if (parts.size() >= 3) {
			DialogueNode node;
			node.id = parts[0];
			node.speaker = parts[1];
			node.text = parts[2];
			dialogueNodes.push_back(node);
		}
	}
	return dialogueNodes;
}

void displayCurrentText(sf::RenderWindow& window, sf::Font& font, const std::string& text) {
	sf::Text displayText;
	displayText.setFont(font);
	displayText.setString(text);
	displayText.setCharacterSize(20);
	displayText.setFillColor(sf::Color::White);
	displayText.setPosition(50, 50);

	window.draw(displayText);
}

//---------------------------------------------------------|

//Method implementing sprite movement
void movement(sf::Vector2f &position, float speed) {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
		position.y -= speed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
		position.y += speed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
		position.x -= speed;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
		position.x += speed;
	}
}

bool detectPlayerRectCollision(sf::Sprite& player, sf::RectangleShape rect) {
	sf::FloatRect playerBounds = player.getGlobalBounds();
	sf::FloatRect rectBounds = rect.getGlobalBounds();

	if (playerBounds.intersects(rectBounds)) {
		return true;
	}
	else {
		return false;
	}
}

//Method implementing screen boundaries
void check_boundaries(sf::Vector2f& position, sf::Sprite& sprite) {
	if (position.x < 0) position.x = 0;
	if (position.x > 800 - sprite.getGlobalBounds().width) 
		position.x = 800 - sprite.getGlobalBounds().width;
	if (position.y < 0) position.y = 0;
	if (position.y > 600 - sprite.getGlobalBounds().height) 
		position.y = 600 - sprite.getGlobalBounds().height;
}


class Inventory {
public:
	std::vector<std::string> items;

	const float HEIGHT = 50.f;
	float width = 50.f;
	sf::RectangleShape invRect;

	Inventory(){
		invRect.setSize(sf::Vector2f(width, HEIGHT));
		invRect.setFillColor(sf::Color(0,0, 100, 150));
		invRect.setOutlineColor(sf::Color::White);
		invRect.setOutlineThickness(2.f);
		invRect.setPosition(50.f, 550.f);
	}

	void addItem(const std::string& item) {
		items.push_back(item);
		width += 50.f;
		invRect.setSize(sf::Vector2f(width, HEIGHT));
		std::cout << "invRect.width incresed: " << width << std::endl;
	}

	void displayItems(sf::RenderWindow& window, sf::Font& font) {
		sf::Text itemText;
		itemText.setString("Inventory:");
		itemText.setFont(font);
		itemText.setCharacterSize(20);
		itemText.setFillColor(sf::Color::White);
		itemText.setPosition(50, 550);
		
		window.draw(invRect);  //Draw inventory rectangle
		window.draw(itemText); //Draw the inventory label
		itemText.setCharacterSize(15); //Scale down text size for items
		int slot = 1;
		for (const auto& item : items) {
			itemText.setString(std::to_string(slot) + ") " + item);
			itemText.setPosition(50 + 85 * (&item - &items[0]), 580); // Adjust position for each item
			window.draw(itemText);
			slot += 1;
		}
	}
};
static Inventory inventory;

//Class for all buttons
class Button {
public:
	sf::Text textObj;
	sf::RectangleShape rect;

	Button(const std::string text, sf::Font& font, float l, float w, 
					int posX, int posY, int offsetX, int offsetY) {

		//Initializing text for button
		this->textObj.setString(text);
		this->textObj.setFont(font);
		this->textObj.setFillColor(sf::Color::Black);
		this->textObj.setCharacterSize(30);
		this->textObj.setPosition(posX + offsetX, posY + offsetY);
		//Initializing rect for button
		this->rect.setSize(sf::Vector2f(l, w));
		this->rect.setFillColor(sf::Color::White);
		this->rect.setPosition(posX, posY);
	}

	//Render text on top of button
	void draw(sf::RenderWindow& window) {
		window.draw(this->rect);
		window.draw(this->textObj);
	}

};

//Class for all doors
class Door {
public:
	sf::RectangleShape rect;

	sf::RectangleShape textDisplayRect;
	
	std::string location;

	bool canDisplayText = false;

	bool isLocked;

	sf::Text doorText;
	
	Door(float l, float w, int posX, int posY, std::string location, int textOffsetX, int textOffsetY, sf::Font& font, bool isLocked) {
		this->rect.setSize(sf::Vector2f(l, w));
		this->rect.setFillColor(sf::Color::White);
		this->rect.setPosition(posX, posY);

		this->textDisplayRect.setSize(sf::Vector2f(l+60, w+60));
		this->textDisplayRect.setFillColor(sf::Color(0, 0, 0, 0)); // transparent
		this->textDisplayRect.setPosition(posX-20, posY-20);

		this->location = location;

		this->doorText.setFont(font);
		this->doorText.setString(location);
		this->doorText.setCharacterSize(15);
		this->doorText.setFillColor(sf::Color::White);
		this->doorText.setPosition(posX + textOffsetX, posY-17+textOffsetY);

		this->isLocked = isLocked;
	
	}

	void render(sf::RenderWindow& window) {
		window.draw(this->rect);
		window.draw(this->textDisplayRect);
		
		if (canDisplayText) {
			window.draw(this->doorText);
			if (!isLocked){this->rect.setFillColor(sf::Color::Green);} // If unlockable turn green
			else { this->rect.setFillColor(sf::Color::Red); } // locked turn red 
		}
		
		else { this->rect.setFillColor(sf::Color::White); } // default color
	}
};

//Method to check if mouse is over a rect
bool mouseOverRect(const sf::RectangleShape& rect, sf::RenderWindow& window) {
	
	//Get the mouse position (x,y)
	sf::Vector2i mouseCoords = sf::Mouse::getPosition(window);

	//Get the boundary of the rectangle
	sf::FloatRect rectBounds = rect.getGlobalBounds();
	
	//Check if the mouse position is within the rectangle bounds.
	return rectBounds.contains(static_cast<sf::Vector2f>(mouseCoords));
}

//Main menu loop
void mainMenu(sf::RenderWindow& window, sf::Font& font);

void controlRoom(sf::RenderWindow& window, sf::Font& font);

void mainChamber(sf::RenderWindow& window, sf::Font& font);

void securityRoom(sf::RenderWindow& window, sf::Font& font);

void analysisLab(sf::RenderWindow& window, sf::Font& font, Door& controlRoomRef);

int main() {

	std::vector<DialogueNode> dialogueNodes = loadDialogue("dialogue.tsv");


	inventory.addItem("Letter");

	sf::RenderWindow window(sf::VideoMode(800, 600), "Please Work", sf::Style::Titlebar | sf::Style::Close);

	sf::Texture texture;
	texture.loadFromFile("C:\\Users\\ikbro\\Desktop\\Sprites\\entity_idle.png");
	
	//"C:\Users\ikbro\Desktop\Fonts\AvilockBold.ttf"
	sf::Font font;
	if(!font.loadFromFile("C:\\Users\\ikbro\\Desktop\\Fonts\\AvilockBold.ttf")) {
		std::cerr << "Error loading font" << std::endl;
		return -1;
	}

	mainMenu(window, font);
	mainChamber(window, font);

	return 0;
}

void mainMenu(sf::RenderWindow& window, sf::Font& font) {

	//Title text
	sf::Text titleText;
	titleText.setFont(font);
	titleText.setString("D A T A   H U N T E R");
	titleText.setFillColor(sf::Color::White);
	titleText.setCharacterSize(60);
	titleText.setPosition(200, 200);

	//Start Button
	Button startButton("Start", font, 100.f, 25.f, 330, 300, 14, -7);

	while (window.isOpen())
	{
		sf::Vector2i mouseCoords = sf::Mouse::getPosition(window);
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::MouseButtonPressed && mouseOverRect(startButton.rect, window)) {
				if (event.mouseButton.button == sf::Mouse::Left) {
					return; // Exit menu and return to main.
				}
			}

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.scancode == sf::Keyboard::Scan::Escape)
					window.close();

				else if (event.key.code == sf::Keyboard::Enter)
					return; // Exit menu and return to main.
			}
		}

		if (mouseOverRect(startButton.rect, window)) {
			startButton.rect.setFillColor(sf::Color::Blue);
		}
		else {
			startButton.rect.setFillColor(sf::Color::White);
		}

		window.draw(titleText);
		startButton.draw(window);

		//End Current Frame
		window.display();

	}
}

void mainChamber(sf::RenderWindow& window, sf::Font& font) {
	sf::Texture texture;
	texture.loadFromFile("C:\\Users\\ikbro\\Desktop\\Sprites\\entity_idle.png");
	int xPos = 0;
	int yPos = 0;
	const float speed_temp = 30.0f;//Define speed
	const float speed = speed_temp * 0.001; //Scale speed down
	sf::Sprite sprite(texture);
	sprite.setScale(3.f, 3.f);
	//Handling spawn point
	std::cout << spawnPoint << std::endl;
	if (spawnPoint == 0) { xPos = 200; yPos = 300; } // Main Chamber
	if (spawnPoint == 1) { xPos = 680; yPos = 300; } // Control Room
	if (spawnPoint == 3) { xPos = 300, yPos = 52; }  // Analysis Lab
	sf::Vector2f position(xPos, yPos);//Current sprite position
	sprite.setPosition(position);

	sf::Vector2f velocity(1, 0);//Initialize Velocity

	sf::Text locationText;
	locationText.setFont(font);
	locationText.setString("Main Chamber");
	locationText.setCharacterSize(25);
	locationText.setFillColor(sf::Color::White);
	locationText.setPosition(10, 10);

	std::vector<Door> doors;
	Door doorEast(50.f, 100.f, 750.f, 300.f, "Control Room", -29, 0, font, true);
	Door doorNorth(100.f, 50.f, 300.f, 0.f, "Analysis Lab ", 0, 70, font, false);
	doors.push_back(doorEast);
	doors.push_back(doorNorth);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.scancode == sf::Keyboard::Scan::Escape)
					window.close();

				if (event.key.scancode == sf::Keyboard::Scan::I) {
					displayInventory = !displayInventory;
				}
			}
		}

		//Handlig Sprite movement
		movement(position, speed);
		sprite.setPosition(position);

		if (hasKey) { doors[0].isLocked = false; } // Unlock door if key is obtained

		for (auto& door : doors) {

			if (detectPlayerRectCollision(sprite, door.textDisplayRect)) {
				door.canDisplayText = true;
			}
			else {
				door.canDisplayText = false;
			}

			if (detectPlayerRectCollision(sprite, door.rect) && !door.isLocked) {//Check for collision with door
				//Check where the door leads
				if (door.location == "Control Room") { controlRoom(window, font); } 
				if (door.location == "Analysis Lab ") { analysisLab(window, font, doorEast); }
			}
		}
		check_boundaries(position, sprite);

		velocity *= 0.9f; //Adjust for friction effect

		//Update position based on velocity
		position += velocity;

		//Clear window with black color
		window.clear(sf::Color::Black);

		// Drawing various objects
		window.draw(locationText);
		//displayCurrentText(window, font, dialogueNodes[1].text);
		for (auto& door : doors) {
			door.render(window);
		}
		window.draw(sprite);
		if (displayInventory) { inventory.displayItems(window, font); }

		//End Current Frame
		window.display();

	}
}

void controlRoom(sf::RenderWindow& window, sf::Font& font) {
	sf::Texture texture;
	texture.loadFromFile("C:\\Users\\ikbro\\Desktop\\Sprites\\entity_idle.png");
	int x_pos = 100;
	int y_pos = 300;
	const float speed_temp = 30.0f;//Define speed
	const float speed = speed_temp * 0.001; //Scale speed down
	sf::Sprite sprite(texture);
	sprite.setScale(3.f, 3.f);
	sf::Vector2f position(x_pos, y_pos);//Current sprite position
	sprite.setPosition(position);

	sf::Vector2f velocity(1, 0);//Initialize Velocity

	sf::Text locationText;
	locationText.setFont(font);
	locationText.setString("Control Room");
	locationText.setCharacterSize(20);
	locationText.setFillColor(sf::Color::White);

	//Initialize doors for the control room
	std::vector<Door> doors;
	Door doorEast(50.f, 100.f, 700.f, 300.f, "Security Room", 0, 0, font, false);
	doors.push_back(doorEast);
	Door doorWest(50.f, 100.f, 50.f, 300.f, "Main Chamber", 0, 0, font, false);
	doors.push_back(doorWest);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.scancode == sf::Keyboard::Scan::Escape)
					window.close();

			}
		}

		//Handlig Sprite movement
		movement(position, speed);
		sprite.setPosition(position);
		for (auto& door : doors) {

			if (detectPlayerRectCollision(sprite, door.textDisplayRect)) {
				door.canDisplayText = true;
			}
			else {
				door.canDisplayText = false;
			}

			if (detectPlayerRectCollision(sprite, door.rect)) {//Check for collision with door
				//Check where the door leads
				if (door.location == "Main Chamber") { spawnPoint = 1; mainChamber(window, font); }
				if (door.location == "Security Room") { securityRoom(window, font); }
			}
		}

		check_boundaries(position, sprite);

		velocity *= 0.9f; //Adjust for friction effect

		//Update position based on velocity
		position += velocity;

		//Clear window with black color
		window.clear(sf::Color::Black);

		window.draw(sprite);
		window.draw(locationText);
		for (auto& door : doors) {
			door.render(window);
		}

		//End Current Frame
		window.display();

	}
}

void securityRoom(sf::RenderWindow& window, sf::Font& font) {
	sf::Texture texture;
	texture.loadFromFile("C:\\Users\\ikbro\\Desktop\\Sprites\\entity_idle.png");
	int x_pos = 200;
	int y_pos = 300;
	const float speed_temp = 30.0f;//Define speed
	const float speed = speed_temp * 0.001; //Scale speed down
	sf::Sprite sprite(texture);
	sprite.setScale(3.f, 3.f);
	sf::Vector2f position(x_pos, y_pos);//Current sprite position
	sprite.setPosition(position);

	sf::Vector2f velocity(1, 0);//Initialize Velocity

	sf::Text locationText;
	locationText.setFont(font);
	locationText.setString("Security Room");
	locationText.setCharacterSize(20);
	locationText.setFillColor(sf::Color::White);

	std::vector<Door> doors;

	Door doorWest(50.f, 100.f, 50.f, 300.f, "Control Room", 0, 0, font, false);
	doors.push_back(doorWest);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.scancode == sf::Keyboard::Scan::Escape)
					window.close();
			}
		}

		//Handlig Sprite movement
		movement(position, speed);
		sprite.setPosition(position);

		for (auto& door : doors) {

			if (detectPlayerRectCollision(sprite, door.textDisplayRect)) {
				door.canDisplayText = true;
			}
			else {
				door.canDisplayText = false;
			}

			if (detectPlayerRectCollision(sprite, door.rect)) {//Check for collision with door
				if (door.location == "Control Room") {
					controlRoom(window, font);
				}
			}
			check_boundaries(position, sprite);

			velocity *= 0.9f; //Adjust for friction effect

			//Update position based on velocity
			position += velocity;

			//Clear window with black color
			window.clear(sf::Color::Black);

			window.draw(sprite);
			window.draw(locationText);
			for (auto& door : doors) {
				door.render(window);
			}
			//End Current Frame
			window.display();

		}

	}
}

void analysisLab(sf::RenderWindow& window, sf::Font& font, Door& controlRoomRef) {
	
	sf::Texture texture;
	texture.loadFromFile("C:\\Users\\ikbro\\Desktop\\Sprites\\entity_idle.png");
	int x_pos = 325;
	int y_pos = 500;
	const float speed_temp = 30.0f;//Define speed
	const float speed = speed_temp * 0.001; //Scale speed down
	sf::Sprite sprite(texture);
	sprite.setScale(3.f, 3.f);
	sf::Vector2f position(x_pos, y_pos);//Current sprite position
	sprite.setPosition(position);

	sf::Vector2f velocity(1, 0);//Initialize Velocity

	sf::Text locationText;
	locationText.setFont(font);
	locationText.setString("Analysis Lab");
	locationText.setCharacterSize(20);
	locationText.setFillColor(sf::Color::White);

	//Inialize key to test door unlock
	sf::RectangleShape key;
	key.setSize(sf::Vector2f(25.f, 25.f));
	key.setFillColor(sf::Color::Yellow);
	key.setPosition(100.f, 300.f);

	std::vector<Door> doors;

	Door doorSouth(100.f, 50.f, 300.f, 550.f, "Main Chamber", 0, 0, font, false);
	doors.push_back(doorSouth);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();

			if (event.type == sf::Event::KeyPressed)
			{
				if (event.key.scancode == sf::Keyboard::Scan::Escape)
					window.close();
				if (event.key.scancode == sf::Keyboard::Scan::I) {
					displayInventory = !displayInventory;
				}
			}
		}

		//Handlig Sprite movement
		movement(position, speed);
		sprite.setPosition(position);

		for (auto& door : doors) {

			if (detectPlayerRectCollision(sprite, door.textDisplayRect)) {
				door.canDisplayText = true;
			}
			else {
				door.canDisplayText = false;
			}

			if (detectPlayerRectCollision(sprite, door.rect)) {//Check for collision with door
				if (door.location == "Main Chamber") {
					spawnPoint = 3;
					mainChamber(window, font);
				}
			}
		}

		if (detectPlayerRectCollision(sprite, key) && !hasKey) {
			hasKey = true; // Unlock door
			key.setFillColor(sf::Color::Green); // Change key color to indicate it has been picked up
			inventory.addItem("Keycard");
		}

		check_boundaries(position, sprite);

		velocity *= 0.9f; //Adjust for friction effect

		//Update position based on velocity
		position += velocity;

		//Clear window with black color
		window.clear(sf::Color::Black);

		// Drawing various objects
		window.draw(sprite);
		window.draw(locationText);
		window.draw(key);
		for (auto& door : doors) {
			door.render(window);
		}
		if(displayInventory){ inventory.displayItems(window, font); }

		//End Current Frame
		window.display();
	}
}