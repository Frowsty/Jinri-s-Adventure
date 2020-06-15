#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"
#include <stdint.h>
#include <random>

#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 1024
#define M_PI 3.14159
#define TILE_SIZE 32
#define SPEED 150
class JinrisGame : public olc::PixelGameEngine
{
private:
    enum GameState
    {
        MENU = 0,
        GAME,
        TUTORIAL,
        CREDITS,
        END
    };
    int mGameState = 0;
    std::vector<std::string> mMenuItems;
    std::map<std::string, std::vector<std::string>> mMenuTutorial;
    std::vector<std::string> mControlsText;
    std::vector<std::string> mStoryLine;
    std::string mMenuTitle;
    int mSelectedItem = 1;
    int mMenuPadding = 2;
    std::string mAppName = "Jinri's Adventure - Built using PixelGameEngine";

    // Random number variables
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> PosDistr;
    std::uniform_int_distribution<> SpdDistr;

    bool bGameRunning;
    // Main menu background
    olc::Sprite* sBackground = nullptr;
    olc::Decal* dBackground = nullptr;

    struct mPlayer
    {
        float x;
        float y;
        bool  walkingX;
        bool  walkingY;
        float nX;
        float nY;
    };

    // Create a player instance
    mPlayer player;

    // FPS Counter
    bool mShowFPS = false;

    // Particles
    struct mParticle
    {
        float x;
        float y;
        float vx;
        float vy;
    };

    std::list<mParticle> mParticles;

    // Layers
    int mLayerIndex;
public:
    JinrisGame() = default;

public:
    bool OnUserCreate() override
    {
        // Set window title
        SetWindowTitle(mAppName);

        bGameRunning = true;

        // Initialize Random generator
        gen = std::mt19937(rd());
        PosDistr = std::uniform_int_distribution<>(0, WINDOW_WIDTH);
        SpdDistr = std::uniform_int_distribution<>(20, 50);

        // Setup menu variables
        mMenuTitle = "JINRI'S ADVENTURE";
        mMenuItems.emplace_back("START GAME");
        mMenuItems.emplace_back("HOW TO PLAY");
        mMenuItems.emplace_back("CREDITS");
        mMenuItems.emplace_back("QUIT");

        // Initialize the control texts
        mControlsText.emplace_back("WALK UP:    ARROW_UP");
        mControlsText.emplace_back("WALK DOWN:  ARROW_DOWN");
        mControlsText.emplace_back("WALK LEFT:  ARROW_LEFT");
        mControlsText.emplace_back("WALK RIGHT: ARROW_RIGHT");

        // Initialize the story line text
        mStoryLine.emplace_back("THE OBJECTIVE IS TO....");
        mMenuTutorial.insert(std::make_pair("KEYBINDINGS", mControlsText));
        mMenuTutorial.insert(std::make_pair("STORYLINE", mStoryLine));

        // Create All Particles
        for (int i = 0; i < 150; i++)
        {
            mParticle p = { static_cast<float>(PosDistr(gen)), static_cast<float>(PosDistr(gen)), 0.f, static_cast<float>(SpdDistr(gen)) };
            mParticles.emplace_back(p);
        }

        // Create the background image for menu
        sBackground = new olc::Sprite("./sprites/logo.png");
        dBackground = new olc::Decal(sBackground);

        // Initialize our main player
        player = { 0, 0, false, false, 0, 0 };

        // Create all of our layers
        mLayerIndex = CreateLayer();

        return true;
    }

    void ExitGame()
    {
        delete sBackground;
        delete dBackground;
        bGameRunning = false;
    }

    void ClampMenu()
    {
        if (mSelectedItem >= mMenuItems.size())
            mSelectedItem = mMenuItems.size();
        else if (mSelectedItem <= 1)
            mSelectedItem = 1;
    }

    void MenuInput()
    {
        // Check if user is scrolling the menu
        if (GetKey(olc::DOWN).bPressed)
            mSelectedItem++;
        if (GetKey(olc::UP).bPressed)
            mSelectedItem--;

        if (GetKey(olc::ENTER).bPressed)
            mGameState = mSelectedItem;
    }

    void DrawParticles()
    {
        SetDrawTarget(mLayerIndex);
        Clear(olc::BLACK);

        // Draw all the particles
        for (auto& p : mParticles)
        {
            if (p.y >= WINDOW_HEIGHT)
                p.y = 0;
            FillCircle(p.x, p.y, 4, olc::WHITE);
            p.y += p.vy * GetElapsedTime();
        }

        EnableLayer(mLayerIndex, true);
        SetDrawTarget(nullptr);
    }

    void DrawGlobalMenu()
    {
        Clear(olc::BLANK);

        // Draw all the snowlike particles
        DrawParticles();

        // Draw title and menu background
        int iXPos = (WINDOW_WIDTH / 2) - (GetTextSize(mMenuTitle).x / 2) * 4;
        int iYPos = 10;
        DrawStringDecal(olc::vi2d(iXPos, iYPos), mMenuTitle, olc::Pixel(172, 83, 194), olc::vf2d(4.0f, 4.0f));
        iXPos = (WINDOW_WIDTH / 2) - (sBackground->width / 2) * 4;
        iYPos += (GetTextSize(mMenuTitle).y * 1.5) * 4;
        DrawDecal(olc::vi2d(iXPos, iYPos), dBackground, olc::vf2d(4.0f, 4.0f));
    }

    void DrawMainMenu()
    {
        // Make sure we draw particles in all submenus / mainmenu as well
        DrawParticles();

        // Call the global menu that contains the title and image
        DrawGlobalMenu();

        int i = 1;
        int iYPos;
        int iXPos;
        for (auto& Item : mMenuItems)
        {
            iXPos = (WINDOW_WIDTH / 2) - (GetTextSize(Item).x / 2) * 4;
            if (i == 1)
                iYPos = (WINDOW_HEIGHT / 2) - (GetTextSize(Item).y / 2) * 4;
            else
                iYPos = (WINDOW_HEIGHT / 2) - ((GetTextSize(Item).y * 1.5) * 4) + ((GetTextSize(Item).y * i) * 4) + (mMenuPadding * (i - 1));
            if (mSelectedItem == i)
                DrawStringDecal(olc::vi2d(iXPos, iYPos), Item, olc::Pixel(172, 83, 194), olc::vf2d(4.0f, 4.0f));
            else
                DrawStringDecal(olc::vi2d(iXPos, iYPos), Item, olc::Pixel(175, 175, 175), olc::vf2d(4.0f, 4.0f));
            i++;
        }
        MenuInput();
        ClampMenu(); // Make sure we can't go out of bounds in menu
    }

    void DrawTutorialMenu()
    {
        // Make sure we draw particles in all submenus
        DrawParticles();

        // Call the global menu that contains the title and image
        DrawGlobalMenu();

        int iYPos;
        int iXPos;
        for (auto& title : mMenuTutorial)
        {
            if (title.first == "KEYBINDINGS")
                iXPos = (WINDOW_WIDTH / 2) / 2 - (GetTextSize(title.first).x / 2) * 4;
            else
                iXPos = (WINDOW_WIDTH / 2) + (WINDOW_WIDTH / 2) / 2 - (GetTextSize(title.first).x / 2) * 4;
            iYPos = (WINDOW_HEIGHT / 2) - (GetTextSize(title.first).y * 4);
            DrawStringDecal(olc::vi2d(iXPos, iYPos), title.first, olc::Pixel(175, 175, 175), olc::vf2d(3.0f, 3.0f));
            int i = 2;
            for (auto& text : title.second)
            {
                if (title.first == "KEYBINDINGS")
                    iXPos = (WINDOW_WIDTH / 2) / 2 - (GetTextSize(title.first).x / 2) * 4;
                else
                    iXPos = (WINDOW_WIDTH / 2) + (WINDOW_WIDTH / 2) / 2 - (GetTextSize(title.first).x / 2) * 4;
                iYPos = (WINDOW_HEIGHT / 2) - ((GetTextSize(text).y * 1.5) * 4) + ((GetTextSize(text).y * i) * 4) + (mMenuPadding * (i - 1));
                DrawStringDecal(olc::vi2d(iXPos, iYPos), text, olc::Pixel(175, 175, 175), olc::vf2d(2.0f, 2.0f));
                i++;
            }
        }
        iXPos = (WINDOW_WIDTH / 2) - (GetTextSize("BACK").x / 2) * 4;
        iYPos = WINDOW_HEIGHT - (GetTextSize("BACK").y * 2) * 4;
        DrawStringDecal(olc::vi2d(iXPos, iYPos), "BACK", olc::Pixel(172, 83, 194), olc::vf2d(4.0f, 4.0f));

        if (GetKey(olc::ENTER).bPressed)
            mGameState = 0;
    }

    void DrawCreditsMenu()
    {
        // Make sure we draw particles in all submenus
        DrawParticles();

        // Call the global menu that contains the title and image
        DrawGlobalMenu();

        if (GetKey(olc::ENTER).bPressed)
            mGameState = 0;
    }

    void PlayerInput()
    {
        if ((GetKey(olc::LEFT).bPressed || GetKey(olc::LEFT).bHeld) && (!player.walkingX && !player.walkingY))
        {
            player.walkingX = true;
            player.nX = player.x - TILE_SIZE;
        }
        if ((GetKey(olc::DOWN).bPressed || GetKey(olc::DOWN).bHeld) && (!player.walkingY && !player.walkingX))
        {
            player.walkingY = true;
            player.nY = player.y + TILE_SIZE;
        }
        if ((GetKey(olc::UP).bPressed || GetKey(olc::UP).bHeld) && (!player.walkingY && !player.walkingX))
        {
            player.walkingY = true;
            player.nY = player.y - TILE_SIZE;
        }
        if ((GetKey(olc::RIGHT).bPressed || GetKey(olc::RIGHT).bHeld) && (!player.walkingX && !player.walkingY))
        {
            player.walkingX = true;
            player.nX = player.x + TILE_SIZE;
        }
    }

    void UpdatePlayer()
    {
        if (player.walkingX)
        {
            if (player.x != player.nX && player.nX > player.x)
                player.x += std::roundf((SPEED * GetElapsedTime()) * 100) / 100;
            if (player.x != player.nX && player.nX < player.x)
                player.x -= std::roundf((SPEED * GetElapsedTime()) * 100) / 100;
            if (std::abs(player.x - player.nX) < 1)
            {
                player.walkingX = false;
                player.x = player.nX;
            }
        }
        if (player.walkingY)
        {
            if (player.y != player.nY && player.nY > player.y)
                player.y += std::roundf((SPEED * GetElapsedTime()) * 100) / 100;
            if (player.y != player.nY && player.nY < player.y)
                player.y -= std::roundf((SPEED * GetElapsedTime()) * 100) / 100;

            if (std::abs(player.y - player.nY) < 1)
            {
                player.walkingY = false;
                player.y = player.nY;
            }
        }
        FillRect(player.x, player.y, 32, 32, olc::WHITE);
    }

    void DrawMap()
    {
        int y = WINDOW_HEIGHT / 32;
        int x = WINDOW_WIDTH / 32;
        for (int i = 0; i < y; i++)
            for (int j = 0; j < x; j++)
                DrawRect(32 * j, 32 * i, 32, 32, olc::BLUE);
    }

    void RunGame()
    {
        // Call player input function (includes movement and other
        DrawMap();
        PlayerInput();
        UpdatePlayer();
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        // "Panic Key" will exit game if pressed
        if (GetKey(olc::ESCAPE).bPressed)
            return false;
        if (GetKey(olc::BACK).bPressed && mGameState != GameState::GAME)
            mGameState = 0;
        if (GetKey(olc::F12).bPressed)
            mShowFPS = !mShowFPS;
        switch (mGameState)
        {
        case GameState::MENU:
            // Draw menu here
            DrawMainMenu();
            break;
        case GameState::GAME:
            // Run game here
            Clear(olc::BLACK);
            RunGame();
            break;
        case GameState::TUTORIAL:
            // Settings menu here
            DrawTutorialMenu();
            break;
        case GameState::CREDITS:
            // Display credits here
            DrawCreditsMenu();
            break;
        case GameState::END:
            // Clear memory and exit game
            ExitGame();
            break;
        }

        if (mShowFPS)
            DrawStringDecal(olc::vi2d(1, 1), "FPS: " + std::to_string(GetFPS()), olc::GREEN, olc::vf2d(2.5f, 2.5f));
        return bGameRunning;
    }
};


int main()
{
    JinrisGame game;
    if (game.Construct(WINDOW_WIDTH, WINDOW_HEIGHT, 1, 1))
        game.Start();

    return 0;
}
