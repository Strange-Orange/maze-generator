#include "maze.h"
#include "globals.h"

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <utility>
#include <map>
#include <stdlib.h>
#include <time.h>

Maze::Maze()
    :m_S(nullptr), m_T(nullptr), m_StartX(g_CR + 1), m_StartY(g_CR + 1), m_EndX((g_WH - (g_CR * 2)) + 1), m_EndY((g_WH - (g_CR * 2)) + 1)
{
    // Initialize random seed
    srand(time(nullptr));
    m_S = SDL_CreateRGBSurfaceWithFormat(0, g_WH, g_WH, 24, SDL_PIXELFORMAT_RGB24);
    if (m_S == nullptr)
    {
        std::cerr << "Failed to create surface: " << SDL_GetError() << std::endl;
    }
    else
    {
        srand(time(nullptr));
        createGrid();
        createCells();

        int randomPoint = rand() % m_Cells.size();
        const int colLength = (g_WH - (g_CR * 2)) / g_CR;
        int col = randomPoint / colLength;
        // The column and the randomPoint must both be even to land on a path space
        if (randomPoint % 2 != 0)
        {
            randomPoint--;
            col = randomPoint / colLength;
        }
        // If the column is not even move to the same row but the previous column
        if (col % 2 != 0)
            randomPoint -= (colLength - 1);

        // There must be a better way to jump to a random point in the map
        CellsIter it = m_Cells.begin();
        for (int i = 0; i < randomPoint; i++, it++) {}

        int spx = it->first.first;
        int spy = it->first.second;
        
        dfsMazeGen(spx, spy);

        updateTexture();
    }
}

Maze Maze::operator=(const Maze& m)
{
    Maze newMaze;
    return newMaze;
}

Maze::~Maze()
{
    if (m_T != nullptr)
    {
        SDL_DestroyTexture(m_T);
    }
    if (m_S != nullptr)
    {
        SDL_FreeSurface(m_S);
    }
}

SDL_Texture* Maze::getTexture() const 
{
    return m_T;
}

// Creates a texture from the current member surface
void Maze::updateTexture()
{
    if (m_T != nullptr)
    {    
        SDL_DestroyTexture(m_T);
        m_T = nullptr;
    }
    m_T = SDL_CreateTextureFromSurface(g_Renderer, m_S);
    if (m_T == nullptr)
    {
        std::cerr << "Failed to create texture: " << SDL_GetError() << std::endl;
    }
}

void Maze::createGrid()
{
    std::vector<SDL_Rect> walls;
    // Boarders
    // Top and bottom
    walls.push_back({g_CR * 2, 0, g_WH, g_CR});
    walls.push_back({0, g_WH - g_CR, g_WH - g_CR * 2, g_CR});
    // Sides
    walls.push_back({0, 0, g_CR, g_WH});
    walls.push_back({g_WH - g_CR, 0, g_CR, g_WH});
    // Columns
    for (int x = g_CR * 2, y = 0; x < g_WH - g_CR; x+= g_CR * 2)
        walls.push_back({x, y, g_CR, g_WH});
    // Rows
    for (int x = 0, y = g_CR * 2; y < g_WH - g_CR; y+= g_CR * 2)
        walls.push_back({x, y, g_WH, g_CR});

    SDL_FillRects(m_S, walls.data(), walls.size(), SDL_MapRGB(m_S->format, 255, 255, 255));
}

// Create map for all grid squares, each is represented by an x, y coordinate being in the top left corner of a cell
void Maze::createCells()
{
    const int sp = g_CR + 1;
    for (int x = sp, y = sp; y < g_WH - g_CR; y+=g_CR)
    {
        for (; x < g_WH - g_CR; x+=g_CR)
        {
            m_Cells[{x, y}] = false;
        }
        x = sp;
    }
}

// Return a vector with the values of DIRECTIONS enum of valid neighbours, visitedStatus is set to false to check if the cell
// has not been visited and to check if the cell has been viisted it is set to true
inline std::vector<int> Maze::checkNeighbours(const int x, const int y, bool visitedStatus)
{
    int distance = g_CR;
    if (!visitedStatus)
        distance = g_CR * 2;
    
    std::vector<int> n;
    if (m_Cells.find({x - distance, y}) != m_Cells.end() && m_Cells.at({x - distance, y}) == visitedStatus)
        n.push_back(LEFT);
    if (m_Cells.find({x + distance, y}) != m_Cells.end() && m_Cells.at({x + distance, y}) == visitedStatus)
        n.push_back(RIGHT);
    if (m_Cells.find({x, y - distance}) != m_Cells.end() && m_Cells.at({x, y - distance}) == visitedStatus)
        n.push_back(UP);
    if (m_Cells.find({x, y + distance}) != m_Cells.end() && m_Cells.at({x, y + distance}) == visitedStatus)
        n.push_back(DOWN);
    return n;
}

// Fill neighbour in the direction of d and update x and y to the new cell
inline void Maze::removeWall(int d, int& x, int& y)
{
    SDL_Rect r;
    switch (d)
    {
        case (LEFT):
            m_Cells.at({x - g_CR, y}) = true;
            r = {(x - g_CR) - 1, y - 1, g_CR, g_CR};
            x -= g_CR * 2; 
            break;
        case (RIGHT):
            m_Cells.at({x + g_CR, y}) = true;
            r = {(x + g_CR) - 1, y - 1, g_CR, g_CR};
            x += g_CR * 2; 
            break;
        case (UP):
            m_Cells.at({x, y - g_CR}) = true;
            r = {x - 1, (y - g_CR) - 1, g_CR, g_CR};
            y -= g_CR * 2;
            break;
        default:
            m_Cells.at({x, y + g_CR}) = true;
            r = {x - 1, (y + g_CR) - 1, g_CR, g_CR};
            y+= g_CR * 2; 
    }

    SDL_FillRect(m_S, &r, SDL_MapRGB(m_S->format, 0, 0, 0));
}

void Maze::dfsMazeGen(int x, int y)
{
    // Store the current cell 
    m_Cells.at({x, y}) = true;
    std::vector<int> n = checkNeighbours(x, y, false);
    // While there are still unvisited neighbours keep going
    while (!n.empty())
    {
        int rNeighbour = rand() % n.size();
        removeWall(n.at(rNeighbour), x, y);
        dfsMazeGen(x, y);
        n = checkNeighbours(x, y, false);
    }
}

// Move to the neighbour in the direction of d
inline void Maze::moveToNeighbour(int d, int& x, int& y)
{
    switch (d)
    {
        case (LEFT):
           x -= g_CR;
           break;
        case (RIGHT):
            x += g_CR;
            break;
        case (UP):
            y -= g_CR;
            break;
        default:
            y += g_CR;
    }
}

void Maze::dfsMazeExplore(int x, int y)
{
    m_Cells.at({x, y}) = false;
    SDL_Rect r = {x, y, 8, 8};
    SDL_FillRect(m_S, &r, SDL_MapRGB(m_S->format, 0, 0, 255));
    std::vector<int> n = checkNeighbours(x, y, true);

    // Keep searching if the exit has not been found
    if (m_Parent.find({m_EndX, m_EndY}) == m_Parent.end() && !n.empty())
    {
        // Parent x and parent y
        const int px = x, py = y;
        for (int& neighbour: n)
        {
            moveToNeighbour(neighbour, x, y);
            if (m_Parent.find({x, y}) == m_Parent.end())
            {
                m_Parent[{px, py}] = {x, y};
                dfsMazeExplore(x, y);
            }
            // Reset position to the parent before moving to new neighbour
            x = px, y = py;   
        }
    }
}

void Maze::explore()
{
    m_Parent[{m_StartX, m_StartY}] = {-1, -1};
    dfsMazeExplore(m_StartX, m_StartY);
}