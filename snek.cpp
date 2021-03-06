#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <time.h>
#include "crow/amalgamate/crow_all.h"
#include "gameinfo.h"

using namespace std;
using namespace crow;


int findFallbackMove(GameInfo game) {
	profile prof(__FUNCTION__, __LINE__);
	cout << "FALL BACK MOVE" << endl;
	Point head = game.snake.getHead();

	vector<int> posmoves = vector<int>();
	vector<int> freevec = vector<int>();
	for (auto m : moveslist) {
		Point p = head.addMove(m);
		//if we can move into tail
		if (p.compare(game.snake.getTail()) && game.snake.coords.size() > 3) {
			return m;
		}


		int free = game.getFreeSquares(head, 7);


		if (game.isValid(p)) {
			posmoves.push_back(m);
			freevec.push_back(free);
		}
	}

	//dead end go into snake BUFFER
	if (!posmoves.size()) {
		cout << "BUFFER" << endl;
		for (auto m : moveslist) {
			Point p = head.addMove(m);
			if (game.board.getCoord(p) == BUFFER) {
				posmoves.push_back(m);
			}
		}
	}

	//try to go into wall
	if (!posmoves.size()) {
		cout << "WALL" << endl;
		for (auto m : moveslist) {
			Point p = head.addMove(m);
			if (game.board.getCoord(p) == WALL) {
				posmoves.push_back(m);
			}
		}
	}

	//its over
	if (!posmoves.size()) {
		return 0;
	}



	return posmoves[distance(freevec.begin(), max_element(freevec.begin(),  freevec.end()) )];
}

int eat(GameInfo game, Path path) {
	if (path.path.size() > 1) {
		return path.getStepDir(0);
	}
	return findFallbackMove(game);
}

int orbit(GameInfo game) {
	Point head = game.snake.getHead();
	Point target = game.getOrbitTarget();
	Path path = game.astarGraphSearch(head, target);
	if (path.path.size() > 1 && game.snake.coords.size() > 3) {
		return path.getStepDir(0);
	}
	return findFallbackMove(game);
}

Path findPathToNearestFood(GameInfo game) {
	Point head = game.snake.getHead();
	Path path = game.breadthFirstSearch(head, {FOOD}, false);
	return path;
}



string moveResponse(int dir) {
	JSON move;
	switch (dir) {
	case NORTH:
		move["move"] = "up";
		move["taunt"] = "THE NORTH REMEMBERS";
		break;
	case EAST:
		move["move"] = "right";
		move["taunt"] = "TO THE EAST";
		break;
	case SOUTH:
		move["move"] = "down";
		move["taunt"] = "SOUTH WHERE ITS WARM";
		break;
	case WEST:
		move["move"] = "left";
		move["taunt"] = "WEST IS BEST";
		break;
	}
	return move.dump();
}

void checkFreeSquares(GameInfo game) {
	Point head = game.snake.getHead();
	cout << "Free Moves " << endl;
	for (auto m : moveslist) {
		Point p = head.addMove(m);
		int free = 0;

		if (game.isValid(p)) {
			free = game.getFreeSquares(p, 10);
		}

		cout << free << " ";
	}

	cout << endl;
}

/*
Path findGoodFood(GameInfo game, int radius){
	for(auto food: game.food){
		for(auto snake: game.snakes){

		}
	}

}*/

bool isClose(GameInfo game, Point point, int psize, int radius){
	for(auto snake: game.snakes){
		if(snake.id.compare(game.snake.id)){
			if(snake.getHead().manDist(point) <= radius){
				return true;
			}
		}
	}
	return false;
}


int decideExcecute(GameInfo game) {
	profile prof(__FUNCTION__, __LINE__);

	checkFreeSquares(game);

	Path foodpath = findPathToNearestFood(game);

	int fsize = foodpath.path.size();
	int ssize = game.snake.coords.size();

	int buffer = 10;
	if (ssize > 12) {
		buffer = 35;
	}

	
	if (isClose(game, foodpath.getLast(), fsize, 5)){
		cout << "IS CLOSE EAT" << endl;
		return eat(game, foodpath);
	}
	

	if(game.snake.coords.size() < 10){
		return eat(game, foodpath);
	}

	if (game.snake.health < (fsize + buffer)) {
		return eat(game, foodpath);
	}
	if (fsize > ssize) {
		return eat(game, foodpath);
	}

	return orbit(game);
}



string SnakeInfo() {
	JSON info;
	info["color"] = "#000F00";
	info["head_url"] = "http://pets.wilco.org/Portals/7/Containers/Pets2011/images/star.png";
	info["taunt"] = "C++ is a superior language";
	info["name"] = "leks";
	return info.dump();
}

SimpleApp initSnakeApp() {
	SimpleApp app;

	//INFO
	CROW_ROUTE(app, "/")([]() {
		return SnakeInfo();
	});


	//START
	CROW_ROUTE(app, "/start")
	.methods("POST"_method)
	([](const crow::request & req) {
		return SnakeInfo();
	});

	//MOVE
	CROW_ROUTE(app, "/move")
	.methods("POST"_method)
	([](const crow::request & req) {
		GameInfo game = GameInfo(req.body);
		int move =  decideExcecute(game);
		return moveResponse(move);
	});

	return app;
}

int main(int argc, char **argv)
{
	int port = 7000;
	if (argc == 2) {
		port = atoi(argv[1]);
	}
	SimpleApp app = initSnakeApp();
	app.port(port).multithreaded().run();
}
