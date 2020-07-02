#define OLC_PGE_APPLICATION
#define OLC_PGEX_ANIMSPR
#include "olcPixelGameEngine.h"
#include "olcPGEX_AnimatedSprite.h"
#include "olcPGEX_Camera2D.h"
#include "olcPGEX_SplashScreen.h"
#include <random>
#include "json.hpp"

using json = nlohmann::json;

#define WINDOW_HEIGHT 832
#define WINDOW_WIDTH 1024
#define M_PI 3.14159
#define TILE_SIZE 32
#define SPEED 150
#define FOV 15
class JinrisGame : public olc::PixelGameEngine
{
private:
    enum GameState
    {
        SPLASHSCREEN = 0,
        MENU,
        GAME,
        TUTORIAL,
        CREDITS,
        END
    };
    int mGameState = 0;
    bool initialized = false;
    std::vector<std::string> mMenuItems;
    std::map<std::string, std::vector<std::string>> mMenuTutorial;
    std::vector<std::string> mControlsText;
    std::vector<std::string> mStoryLine;
    std::string mMenuTitle;
    int mSelectedItem = 1;
    int mMenuPadding = 2;
    std::string mAppName = "Jinri's Adventure - The unknown ending";

    // Random number variables
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_int_distribution<> PosDistr;
    std::uniform_int_distribution<> SpdDistr;
    std::uniform_int_distribution<> EnmDistr;
    std::uniform_int_distribution<> mRandomPlayerPos;

    bool bGameRunning;
    // Main menu background
    olc::Renderable mBackground;

    // Player sprite
    olc::AnimatedSprite PlayerSprite;
    olc::Renderable* spritesheet;
    // Player Camera
    olcPGEX_Camera2D camera;

    // Splash screen
    olcPGEX_SplashScreen mSplashScreen;

    struct mPlayer
    {
        float x;
        float y;
        bool  walkingX;
        bool  walkingY;
        int nX;
        int nY;
        int health;
    };

    // Create a player instance
    mPlayer player;
    std::string mSpriteStateName = "idle-down";
    bool mSpawnPlayer = true;
    int mStoredPlayerX;
    int mStoredPlayerY;

    // Enable debug mode for debug information
    bool mDebugMode = false;

    // Particles
    struct mParticle
    {
        float x;
        float y;
        float vx;
        float vy;
    };

    std::list<mParticle> mParticles;

    struct mTile
    {
        olc::vi2d position;
        olc::vi2d tilesheetPos;
        bool destroyed;
    };

    struct mCollider
    {
        std::string tag;
        olc::vf2d position;
        olc::vf2d size;
        mTile* tile;
    };

    std::vector<mCollider*> mColliders;
    mCollider mPlayerCollider;

    struct mProjectile
    {
        olc::vf2d startPosition;
        olc::vf2d position;
        olc::vf2d size;
        bool destroyed;
        std::string direction;
    };

    std::vector<mProjectile*> mProjectiles;
    mCollider mProjectileCollider;
    olc::Renderable mProjectileSprite;
    float mProjectileRotation = 0.0f;

    olc::Renderable mSpriteSheet;
    std::list<mTile*> mTiles;
    int mMapSizeX;
    int mMapSizeY;

    // Particle layer
    int mLayerParticle;

    int mPossibleCollidables = 0;
    int mTilesDrawnOnMap = 0;

    bool* mObstacleMap;
    int* mFlowFieldZ;

    struct mMonster
    {
        olc::vf2d position;
        olc::vi2d nPosition;
        olc::vi2d oPosition;
        int health;
        olc::vi2d startPos;
        olc::vi2d endPos;
        std::list<std::pair<int, int>> path;
        mCollider* collider;
    };

    std::vector<mMonster*> mMonsters;
    int monsterTempX = 0;
    int monsterTempY = 0;


public:
    JinrisGame() = default;

public:

    // Pyxel json map parser
    void LoadMap()
    {
        // This is not used by LoadMap, I only load the tilesheet here to access it later
        // Figured it was best placed here since it's map related
        mSpriteSheet.Load("./sprites/tilesheet.png");

        std::ifstream i("./sprites/testing.json");
        json j = json::parse(i);

        mMapSizeY = j.at("tileshigh");
        mMapSizeX = j.at("tileswide");
        olc::vi2d tileSize = { j.at("tilewidth"), j.at("tileheight") };

        int tileSheetWidth = mSpriteSheet.Sprite()->width / tileSize.x;

        auto layers = j.at("layers");

        // Loop all the layers that we grab from the json file
        for (auto& layer : layers)
        {
            // Loop each tile in the layer
            for (auto& tile : layer.at("tiles"))
            {
                // This is the tile ID (position in the tilesheet / spritesheet) determined by Pyxel
                int tileID = tile.at("tile");
                // If tileID is -1 that means the tile has no tile aka is transparent
                if (tileID != -1)
                {
                    std::string layerName = layer.at("name");
                    int x = tile.at("x");
                    int y = tile.at("y");
                    x *= tileSize.x;
                    y *= tileSize.y;

                    int tileSheetPosY = 0;
                    int tileSheetPosX = 0;

                    // This is an easy way to know what Row in the tilesheet we want to grab the tile from
                    if (tileID > tileSheetWidth)
                    {
                        int i = tileID;
                        while (i > tileSheetWidth)
                        {
                            tileSheetPosY += 1;
                            i -= tileSheetWidth;
                        }
                        tileSheetPosX = i - 1;
                    }
                    else
                    {
                        tileSheetPosX = tileID - 1;
                    }

                    // Make sure we grab just 1 tile / sprite
                    tileSheetPosY *= tileSize.y;
                    tileSheetPosX *= tileSize.x;

                    // Here we store all tile values into a vector of tiles
                    // The data we store is basically the position on the map where it's drawn
                    // and also the position on the tileSheet where it's stored
                    mTiles.push_back(new mTile{ { x, y }, { tileSheetPosX, tileSheetPosY }, false });

                    // Used for my own collisions, ignore this (Credits to Witty bits for the collision struct from the relay race)
                    if (layerName == "Colliders")
                        mColliders.push_back(new mCollider{ "map_terrain", { static_cast<float>(x), static_cast<float>(y) }, tileSize, mTiles.back() });
                    if (layerName == "Collectables")
                        mColliders.push_back(new mCollider{ "collectable", { static_cast<float>(x), static_cast<float>(y) }, tileSize, mTiles.back() });
                }
            }
        }
    }

    void ReloadObstacleMap(mMonster* monster)
    {
        auto p = [&](int x, int y) { return y * mMapSizeX + x; };
        int minFOV = monster->position.x - FOV;
        int maxFOV = monster->position.x + FOV;
        if (minFOV < 0)
            minFOV = 0;
        if (maxFOV > mMapSizeX)
            maxFOV = mMapSizeX;
        for (int x = minFOV; x < maxFOV; x++)
        {
            for (int y = minFOV; y < maxFOV; y++)
            {
                mObstacleMap[p(x, y)] = false;
                for (auto* c : mColliders)
                {
                    if (c->position.x / TILE_SIZE == x && c->position.y / TILE_SIZE == y && c->tag != "_destroyed_")
                        mObstacleMap[p(x, y)] = true;
                }
                for (auto* m : mMonsters)
                {
                    if (!m->path.empty())
                    {
                        for (const auto& cord : m->path)
                        {
                            if (cord.first == x && cord.second == y)
                                mObstacleMap[p(cord.first, cord.second)] = true;
                        }
                    }
                }
                if (mObstacleMap[p(x, y)])
                    mFlowFieldZ[p(x, y)] = -1;
                else
                    mFlowFieldZ[p(x, y)] = 0;
            }
        }
    }

    bool OnUserCreate() override
    {
        // Set window title
        SetWindowTitle(mAppName);

        bGameRunning = true;

        LoadMap();

        // Init obstacle and flowmap
        mObstacleMap = new bool[mMapSizeX * mMapSizeY]{ false };
        mFlowFieldZ = new int[mMapSizeX * mMapSizeY]{ 0 };

        // Initialize Random generator
        gen = std::mt19937(rd());
        PosDistr = std::uniform_int_distribution<>(0, WINDOW_WIDTH);
        SpdDistr = std::uniform_int_distribution<>(20, 50);
        EnmDistr = std::uniform_int_distribution<>(-10, 10);
        mRandomPlayerPos = std::uniform_int_distribution<>(1, mMapSizeX - 1);

        // Setup menu options
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
        mControlsText.emplace_back("SHOOTING:   SPACE");

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
        mBackground.Load("./sprites/logo.png");

        // Initialize our main player along side all sprites for main player
        player = { 0, 0, false, false, 0, 0, 100 };
        mPlayerCollider = { "player", { player.x, player.y }, { static_cast<float>(TILE_SIZE), static_cast<float>(TILE_SIZE) } };
        PlayerSprite.type = olc::AnimatedSprite::SPRITE_TYPE::DECAL;
        PlayerSprite.mode = olc::AnimatedSprite::SPRITE_MODE::SINGLE;
        spritesheet = new olc::Renderable();

        spritesheet->Load("./sprites/character.png");
        PlayerSprite.spriteSheet = spritesheet;
        PlayerSprite.SetSpriteSize({ 32, 32 });

        // Add animated player states
        PlayerSprite.AddState("idle-down", { olc::vi2d(32, 0) });
        PlayerSprite.AddState("walking-down", { olc::vi2d(0, 0), olc::vi2d(64, 0) });
        PlayerSprite.AddState("idle-left", { olc::vi2d(32, 32) });
        PlayerSprite.AddState("walking-left", { olc::vi2d(0, 32), olc::vi2d(64, 32) });
        PlayerSprite.AddState("idle-right", { olc::vi2d(32, 64) });
        PlayerSprite.AddState("walking-right", { olc::vi2d(0, 64), olc::vi2d(64, 64) });
        PlayerSprite.AddState("idle-up", { olc::vi2d(32, 96) });
        PlayerSprite.AddState("walking-up", { olc::vi2d(0, 96), olc::vi2d(64, 96) });

        // Set players default state
        PlayerSprite.SetState("idle-down");

        // Load sprite for the projectile
        mProjectileSprite.Load("./sprites/banana.png");

        // Create Monsters
        for (int i = 0; i < 10; i++)
        {
            float x = SpdDistr(gen);
            float y = SpdDistr(gen);
            mColliders.push_back(new mCollider{ "monster", { x, y }, { TILE_SIZE, TILE_SIZE } });
            mMonsters.push_back(new mMonster{ { x, y }, { static_cast<int>(x), static_cast<int>(y) }, { static_cast<int>(x), static_cast<int>(y) },
                                                      100, { 1, 1 }, { 5, 5 }, {}, mColliders.back() });
        }

        // Set Camera position
        camera.InitialiseCamera(olc::vf2d(player.x, player.y) - (camera.vecCamViewSize * 0.5), { WINDOW_WIDTH, WINDOW_HEIGHT });
        camera.vecCamPos = { player.x, player.y };

        // Create all of our layers
        mLayerParticle = CreateLayer();

        return true;
    }

    void ExitGame()
    {
        delete spritesheet;
        delete[] mObstacleMap;
        delete[] mFlowFieldZ;
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
            mGameState = mSelectedItem + 1;
    }

    void DrawParticles()
    {
        SetDrawTarget(mLayerParticle);
        Clear(olc::BLACK);

        // Draw all the particles
        for (auto& p : mParticles)
        {
            if (p.y >= WINDOW_HEIGHT)
                p.y = 0;
            FillCircle(p.x, p.y, 4, olc::WHITE);
            p.y += p.vy * GetElapsedTime();
        }

        EnableLayer(mLayerParticle, true);
        SetDrawTarget(nullptr);
    }

    void DrawGlobalMenu()
    {
        Clear(olc::BLANK);

        // Draw all the snowlike particles
        DrawParticles();

        // Draw title and menu background
        int iXPos = (WINDOW_WIDTH * 0.5) - (GetTextSize(mMenuTitle).x * 0.5) * 4;
        int iYPos = 10;
        DrawStringDecal(olc::vi2d(iXPos, iYPos), mMenuTitle, olc::Pixel(172, 83, 194), olc::vf2d(4.0f, 4.0f));
        iXPos = (WINDOW_WIDTH * 0.5) - (mBackground.Sprite()->width * 0.5) * 4;
        iYPos += (GetTextSize(mMenuTitle).y * 1.5) * 4;
        DrawDecal(olc::vi2d(iXPos, iYPos), mBackground.Decal(), olc::vf2d(4.0f, 4.0f));
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
            iXPos = (WINDOW_WIDTH * 0.5) - (GetTextSize(Item).x * 0.5) * 4;
            if (i == 1)
                iYPos = (WINDOW_HEIGHT * 0.5) - (GetTextSize(Item).y * 0.5) * 4;
            else
                iYPos = (WINDOW_HEIGHT * 0.5) - ((GetTextSize(Item).y * 1.5) * 4) + ((GetTextSize(Item).y * i) * 4) + (mMenuPadding * (i - 1));
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
                iXPos = (WINDOW_WIDTH * 0.5) * 0.5 - (GetTextSize(title.first).x * 0.5) * 3.5;
            else
                iXPos = (WINDOW_WIDTH * 0.5) + (WINDOW_WIDTH * 0.5) * 0.5 - (GetTextSize(title.first).x * 0.5) * 3;
            iYPos = (WINDOW_HEIGHT * 0.5) - (GetTextSize(title.first).y * 4);
            DrawStringDecal(olc::vi2d(iXPos, iYPos), title.first, olc::Pixel(175, 175, 175), olc::vf2d(3.0f, 3.0f));
            int i = 2;
            for (auto& text : title.second)
            {
                if (title.first == "KEYBINDINGS")
                    iXPos = (WINDOW_WIDTH * 0.5) * 0.5 - (GetTextSize(title.first).x * 0.5) * 4;
                else
                    iXPos = (WINDOW_WIDTH * 0.5) + (WINDOW_WIDTH * 0.5) * 0.5 - (GetTextSize(title.first).x * 0.5) * 4;
                iYPos = (WINDOW_HEIGHT * 0.5) - ((GetTextSize(text).y * 1.5) * 4) + ((GetTextSize(text).y * i) * 4) + (mMenuPadding * (i - 1));
                DrawStringDecal(olc::vi2d(iXPos, iYPos), text, olc::Pixel(175, 175, 175), olc::vf2d(2.0f, 2.0f));
                i++;
            }
        }
        iXPos = (WINDOW_WIDTH * 0.5) - (GetTextSize("BACK").x * 0.5) * 4;
        iYPos = WINDOW_HEIGHT - (GetTextSize("BACK").y * 2) * 4;
        DrawStringDecal(olc::vi2d(iXPos, iYPos), "BACK", olc::Pixel(172, 83, 194), olc::vf2d(4.0f, 4.0f));

        if (GetKey(olc::ENTER).bPressed)
            mGameState = 1;
    }

    void DrawCreditsMenu()
    {
        // Make sure we draw particles in all submenus
        DrawParticles();

        // Call the global menu that contains the title and image
        DrawGlobalMenu();

        if (GetKey(olc::ENTER).bPressed)
            mGameState = 1;
    }


    bool CheckCollision(mCollider& left, mCollider& right)
    {
        return true
            && left.position.x == right.position.x
            && left.position.y == right.position.y;
    }

    bool CheckProjectileCollision(mCollider& p, mCollider& o)
    {
        return true
            && p.position.x + p.size.x > o.position.x
            && p.position.x < o.position.x + o.size.x
            && p.position.y + p.size.y > o.position.y
            && p.position.y < o.position.y + o.size.y;
    }

    bool CheckMonsterCollision(mMonster* monster)
    {
        if (monster->collider->tag == "_destroyed_")
            return false;
        if (!mProjectiles.empty() && CheckProjectileCollision(mProjectileCollider, *(monster->collider)))
        {
            monster->health -= 50;
            if (monster->health <= 0)
                monster->collider->tag = "_destroyed_";

            mProjectiles.pop_back();
            return true;
        }
        for (auto* m : mMonsters)
        {
            if (m == monster)
                continue;
            if (CheckProjectileCollision(*(monster->collider), *(m->collider)))
            {
                monster->position = monster->oPosition;
                m->position = m->oPosition;
                FindPath(monster);
                FindPath(m);
                return true;
            }
        }
        return false;
    }

    bool CheckCollisions()
    {
        mPossibleCollidables = 0;
        for (auto* c : mColliders)
        {
            // Only check collisions if there is a projectile
            if (!mProjectiles.empty())
            {
                if (CheckProjectileCollision(mProjectileCollider, *c))
                {
                    if (c->tag == "map_terrain")
                    {
                        c->tile->destroyed = true;
                        c->tag = "_destroyed_";
                        mProjectiles.pop_back();
                    }
                }
            }
            // only check collision on collidables within a tile from the player
            if (std::abs(c->position.y - player.nY) > TILE_SIZE ||
                std::abs(c->position.x - player.nX) > TILE_SIZE ||
                c->tag == "_destroyed_")
                continue;
            if (CheckCollision(mPlayerCollider, *c))
            {
                if (c->tag == "collectable")
                {
                    return true;
                }
                else if (c->tag == "map_terrain")
                {
                    return true;
                }
            }
            if (c->tag == "monster" && CheckProjectileCollision(mPlayerCollider, *c))
            {
                return true;
            }
            mPossibleCollidables += 1;
        }
        return false;
    }

    void UpdateProjectile(mProjectile& p)
    {
        if (CheckCollisions())
            return;

        if (p.position.x < camera.vecCamPos.x ||
            p.position.x > camera.vecCamPos.x + camera.vecCamViewSize.x ||
            p.position.y > camera.vecCamPos.y + camera.vecCamViewSize.y ||
            p.position.y < camera.vecCamPos.y ||
            std::abs(p.startPosition.x - p.position.x) >(6 * TILE_SIZE) ||
            std::abs(p.startPosition.y - p.position.y) > (6 * TILE_SIZE))
            mProjectiles.pop_back();

        mProjectileCollider.position = p.position;
        if (p.direction == "idle-left" || p.direction == "walking-left")
            p.position.x -= (SPEED * 2) * GetElapsedTime();
        if (p.direction == "idle-right" || p.direction == "walking-right")
            p.position.x += (SPEED * 2) * GetElapsedTime();
        if (p.direction == "idle-down" || p.direction == "walking-down")
            p.position.y += (SPEED * 2) * GetElapsedTime();
        if (p.direction == "idle-up" || p.direction == "walking-up")
            p.position.y -= (SPEED * 2) * GetElapsedTime();

        if (mProjectileRotation >= 360.0f)
            mProjectileRotation = 0.0f;

        mProjectileRotation += 20 * GetElapsedTime();
        DrawRotatedDecal(p.position - camera.vecCamPos, mProjectileSprite.Decal(), mProjectileRotation,
            { mProjectileSprite.Sprite()->width / 2.0f, mProjectileSprite.Sprite()->height / 2.0f });
    }

    void PlayerInput()
    {
        if ((GetKey(olc::LEFT).bPressed || GetKey(olc::LEFT).bHeld) && (!player.walkingX && !player.walkingY))
        {
            player.walkingX = true;
            player.nX = player.x - TILE_SIZE;
            mSpriteStateName = "idle-left";
        }
        if ((GetKey(olc::DOWN).bPressed || GetKey(olc::DOWN).bHeld) && (!player.walkingY && !player.walkingX))
        {
            player.walkingY = true;
            player.nY = player.y + TILE_SIZE;
            mSpriteStateName = "idle-down";
        }
        if ((GetKey(olc::UP).bPressed || GetKey(olc::UP).bHeld) && (!player.walkingY && !player.walkingX))
        {
            player.walkingY = true;
            player.nY = player.y - TILE_SIZE;
            mSpriteStateName = "idle-up";
        }
        if ((GetKey(olc::RIGHT).bPressed || GetKey(olc::RIGHT).bHeld) && (!player.walkingX && !player.walkingY))
        {
            player.walkingX = true;
            player.nX = player.x + TILE_SIZE;
            mSpriteStateName = "idle-right";
        }
        if (GetKey(olc::SPACE).bPressed && mProjectiles.empty())
        {
            mProjectiles.push_back(new mProjectile{ { player.x + 16.0f , player.y + 16.0f },
                { player.x + 16.0f , player.y + 16.0f },
                { static_cast<float>(mProjectileSprite.Sprite()->width), static_cast<float>(mProjectileSprite.Sprite()->height) }, false, mSpriteStateName });
            mProjectileCollider = { "projectile", { player.x + 16.0f, player.y + 16.0f },
                { static_cast<float>(mProjectileSprite.Sprite()->width - 2), static_cast<float>(mProjectileSprite.Sprite()->height) } };
            mProjectileRotation = 0.0f;
        }
    }

    void FixPlayerPosition()
    {
        if (player.walkingX && player.nX % TILE_SIZE != 0)
        {
            if (mSpriteStateName == "idle-right" || mSpriteStateName == "walking-right")
                player.x += 1;
            if (mSpriteStateName == "idle-left" || mSpriteStateName == "walking-left")
                player.x -= 1;
        }
        if (player.walkingY && player.nY % TILE_SIZE != 0)
        {
            if (mSpriteStateName == "idle-down" || mSpriteStateName == "walking-down")
                player.y += 1;
            if (mSpriteStateName == "idle-up" || mSpriteStateName == "walking-up")
                player.y -= 1;
        }
    }

    void UpdatePlayer()
    {
        mPlayerCollider.position = { static_cast<float>(player.nX), static_cast<float>(player.nY) };
        FixPlayerPosition();
        if (player.walkingX)
        {
            if (!CheckCollisions() && player.nX < (mMapSizeX * TILE_SIZE) && player.nX > -TILE_SIZE)
            {
                if (mSpriteStateName == "idle-right" || mSpriteStateName == "walking-right")
                {
                    player.x += SPEED * GetElapsedTime();
                    mSpriteStateName = "walking-right";
                }
                if (mSpriteStateName == "idle-left" || mSpriteStateName == "walking-left")
                {
                    player.x -= SPEED * GetElapsedTime();
                    mSpriteStateName = "walking-left";
                }

                if (std::abs(player.x - player.nX) < 1 || std::abs(player.x - player.nX) > 32)
                {
                    player.walkingX = false;
                    player.x = player.nX;
                    if (mSpriteStateName == "walking-left")
                        mSpriteStateName = "idle-left";
                    else
                        mSpriteStateName = "idle-right";
                }
            }
            else
            {
                player.walkingX = false;
                player.nX = player.x;
            }
        }
        if (player.walkingY)
        {
            if (!CheckCollisions() && player.nY < (mMapSizeY * TILE_SIZE) && player.nY > -TILE_SIZE)
            {
                if (mSpriteStateName == "idle-down" || mSpriteStateName == "walking-down")
                {
                    player.y += SPEED * GetElapsedTime();
                    mSpriteStateName = "walking-down";
                }
                if (mSpriteStateName == "idle-up" || mSpriteStateName == "walking-up")
                {
                    player.y -= SPEED * GetElapsedTime();
                    mSpriteStateName = "walking-up";
                }

                if (std::abs(player.y - player.nY) < 1 || std::abs(player.y - player.nY) > 32)
                {
                    player.walkingY = false;
                    player.y = player.nY;
                    if (mSpriteStateName == "walking-down")
                        mSpriteStateName = "idle-down";
                    else
                        mSpriteStateName = "idle-up";
                }
            }
            else
            {
                player.walkingY = false;
                player.nY = player.y;
            }
        }
        camera.vecCamPos = camera.LerpCamera(camera.ClampVector({ 0, 0 }, { static_cast<float>(mMapSizeX * TILE_SIZE), static_cast<float>(mMapSizeY * TILE_SIZE) },
            (olc::vf2d(player.x, player.y) - camera.vecCamViewSize * 0.5f)), 15.0f);
        camera.ClampCamera({ 0, 0 }, { static_cast<float>(mMapSizeX* TILE_SIZE), static_cast<float>(mMapSizeY* TILE_SIZE) });
        PlayerSprite.SetState(mSpriteStateName);
        PlayerSprite.Draw(GetElapsedTime(), olc::vf2d(player.x, player.y) - camera.vecCamPos);
    }

    void SpawnPlayer()
    {
        float tempX = mRandomPlayerPos(gen) * TILE_SIZE;
        float tempY = mRandomPlayerPos(gen) * TILE_SIZE;
        bool shouldSpawn = false;
        for (auto* c : mColliders)
        {
            if (tempX == c->position.x || tempY == c->position.y)
            {
                tempX = mRandomPlayerPos(gen);
                tempY = mRandomPlayerPos(gen);
                continue;
            }
            if (!shouldSpawn)
                shouldSpawn = true;
            continue;
        }
        if (fmod(tempX, static_cast<float>(TILE_SIZE)) == 0 && fmod(tempY, static_cast<float>(TILE_SIZE)) == 0)
        {
            player.x = 15 * 32;
            player.y = 15 * 32;
            player.nX = player.x;
            player.nY = player.y;
            std::cout << "Spawned at: " << player.x << " : " << player.y << std::endl;
            mSpawnPlayer = false;
        }
    }

    void FindPath(mMonster* monster)
    {
        ReloadObstacleMap(monster);
        auto p = [&](int x, int y) { return y * mMapSizeX + x; };
        std::list<std::tuple<int, int, int>> nodes;
        nodes.push_back({ monster->endPos.x, monster->endPos.y, 1 });

        while (!nodes.empty())
        {
            std::list<std::tuple<int, int, int>> newNodes;

            for (auto& n : nodes)
            {
                int x = std::get<0>(n);
                int y = std::get<1>(n);
                int d = std::get<2>(n);

                mFlowFieldZ[p(x, y)] = d;

                if ((x + 1) < mMapSizeX && mFlowFieldZ[p(x + 1, y)] == 0)
                    newNodes.push_back({ x + 1, y, d + 1 });
                if ((x - 1) >= 0 && mFlowFieldZ[p(x - 1, y)] == 0)
                    newNodes.push_back({ x - 1, y, d + 1 });
                if ((y + 1) < mMapSizeY && mFlowFieldZ[p(x, y + 1)] == 0)
                    newNodes.push_back({ x, y + 1, d + 1 });
                if ((y - 1) >= 0 && mFlowFieldZ[p(x, y - 1)] == 0)
                    newNodes.push_back({ x, y - 1, d + 1 });

            }

            newNodes.sort([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
                {
                    return p(std::get<0>(n1), std::get<1>(n1)) < p(std::get<0>(n2), std::get<1>(n2));
                });
            newNodes.unique([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
                {
                    return p(std::get<0>(n1), std::get<1>(n1)) == p(std::get<0>(n2), std::get<1>(n2));
                });

            nodes.clear();
            nodes.insert(nodes.begin(), newNodes.begin(), newNodes.end());
        }

        std::list<std::pair<int, int>> path;
        int locX = monster->startPos.x;
        int locY = monster->startPos.y;
        bool noPath = false;
        while (!(locX == monster->endPos.x && locY == monster->endPos.y) && !noPath)
        {
            std::list<std::tuple<int, int, int>> listNeighbours;

            // 4-Way Connectivity
            if ((locY - 1) >= 0 && mFlowFieldZ[p(locX, locY - 1)] >= 0)
                listNeighbours.push_back({ locX, locY - 1, mFlowFieldZ[p(locX, locY - 1)] });
            if ((locX + 1) <= mMapSizeX && mFlowFieldZ[p(locX + 1, locY)] >= 0)
                listNeighbours.push_back({ locX + 1, locY, mFlowFieldZ[p(locX + 1, locY)] });
            if ((locY + 1) <= mMapSizeY && mFlowFieldZ[p(locX, locY + 1)] >= 0)
                listNeighbours.push_back({ locX, locY + 1, mFlowFieldZ[p(locX, locY + 1)] });
            if ((locX - 1) >= 0 && mFlowFieldZ[p(locX - 1, locY)] >= 0)
                listNeighbours.push_back({ locX - 1, locY, mFlowFieldZ[p(locX - 1, locY)] });

            listNeighbours.sort([&](const std::tuple<int, int, int>& n1, const std::tuple<int, int, int>& n2)
                {
                    return std::get<2>(n1) < std::get<2>(n2);
                });

            if (listNeighbours.empty())
                noPath = true;
            else
            {
                locX = std::get<0>(listNeighbours.front());
                locY = std::get<1>(listNeighbours.front());
                path.push_back({ locX, locY });
            }
            if (path.size() > 250)
            {
                path.clear();
                noPath = true;
            }
        }
        if (!path.empty())
            path.pop_back();
        monster->path = path;
    }

    void MonsterMovement(mMonster* monster)
    {
        bool walkingX = false;
        bool walkingY = false;

        monster->collider->position.x = monster->position.x * TILE_SIZE;
        monster->collider->position.y = monster->position.y * TILE_SIZE;
        if (CheckMonsterCollision(monster))
            return;
        if (!monster->path.empty())
        {
            monster->oPosition = monster->position;
            monster->nPosition = { monster->path.front().first, monster->path.front().second };

            if (monster->position.x != monster->nPosition.x && monster->position.y == monster->nPosition.y)
                walkingX = true;
            else
                walkingY = true;
            if (monster->position.y != monster->nPosition.y && monster->position.x == monster->nPosition.x && !walkingX)
                walkingY = true;
            else
                walkingX = true;

            if (walkingX)
            {
                if (monster->position.x - monster->nPosition.x < 0)
                    monster->position.x += (SPEED / 2 * GetElapsedTime()) / TILE_SIZE;
                if (monster->position.x - monster->nPosition.x > 0)
                    monster->position.x -= (SPEED / 2 * GetElapsedTime()) / TILE_SIZE;

                if (std::abs(monster->position.x - monster->nPosition.x) < 0.1)
                {
                    monster->position.x = monster->nPosition.x;
                    if (!monster->path.empty())
                        monster->path.pop_front();
                }
            }

            if (walkingY)
            {
                if (monster->position.y - monster->nPosition.y < 0)
                    monster->position.y += (SPEED / 2 * GetElapsedTime()) / TILE_SIZE;
                if (monster->position.y - monster->nPosition.y > 0)
                    monster->position.y -= (SPEED / 2 * GetElapsedTime()) / TILE_SIZE;

                if (std::abs(monster->position.y - monster->nPosition.y) < 0.1)
                {                 
                    monster->position.y = monster->nPosition.y;
                    if (!monster->path.empty())
                        monster->path.pop_front();
                }
            }
        }
    }

    void DrawMonsters()
    {
        auto p = [&](int x, int y) { return y * mMapSizeX + x; };
        for (auto* monster : mMonsters)
        {
            if (monster->collider->tag == "_destroyed_")
                continue;
            FillRectDecal((monster->position * TILE_SIZE) - camera.vecCamPos, { TILE_SIZE, TILE_SIZE }, olc::BLUE);
            DrawStringDecal((monster->position * TILE_SIZE) - camera.vecCamPos, std::to_string(monster->health), olc::GREEN);
            if (std::abs(player.x / TILE_SIZE - monster->position.x) < FOV &&
                std::abs(player.y / TILE_SIZE - monster->position.y) < FOV &&
                (mStoredPlayerX != player.x || mStoredPlayerY != player.y))
            {
                monster->path.clear();

                monster->startPos = { monster->oPosition.x, monster->oPosition.y };
                monster->endPos = { player.nX / TILE_SIZE, player.nY / TILE_SIZE };

                FindPath(monster);

                mStoredPlayerX = player.x;
                mStoredPlayerY = player.y;
            }
            else if (std::abs(player.x / TILE_SIZE - monster->position.x) < FOV &&
		     std::abs(player.y / TILE_SIZE - monster->position.y) < FOV &&
		     monster->path.empty() && std::abs((player.nX / TILE_SIZE) - monster->oPosition.x) > 1 &&
		     std::abs((player.nY / TILE_SIZE) - monster->oPosition.y) > 1)
            {
		int endX = monster->oPosition.x - EnmDistr(gen);
		int endY = monster->oPosition.y - EnmDistr(gen);
		if (endX == monster->oPosition.x)
		    endX++;
		if (endY == monster->oPosition.y)
		    endY++;
                monster->startPos = { monster->oPosition.x, monster->oPosition.y };
                monster->endPos = { endX, endY };
                FindPath(monster);
            }
            if (mDebugMode)
            {
                for (const auto coordinate : monster->path)
                {
                    FillRectDecal({ static_cast<float>(coordinate.first * TILE_SIZE - camera.vecCamPos.x) + 2,
                            static_cast<float>(coordinate.second * TILE_SIZE - camera.vecCamPos.y) + 2 },
                        { TILE_SIZE - 4, TILE_SIZE - 4 }, olc::WHITE);
                }
            }
            MonsterMovement(monster);
        }
    }

    void DrawMap()
    {
        mTilesDrawnOnMap = 0;
        for (auto* tile : mTiles)
        {
            // Set rendering bounds, if tile that is suppose to be rendered is not within the viewport
            // Or if the tile is destroyed we shall not render it.
            if (tile->position.y + TILE_SIZE < camera.vecCamPos.y ||
                tile->position.y > camera.vecCamPos.y + camera.vecCamViewSize.y ||
                tile->position.x + TILE_SIZE < camera.vecCamPos.x ||
                tile->position.x > camera.vecCamPos.x + camera.vecCamViewSize.x)
                continue;
            if (tile->destroyed)
                continue;

            DrawPartialDecal(tile->position - camera.vecCamPos, mSpriteSheet.Decal(), tile->tilesheetPos, { 32, 32 });
            mTilesDrawnOnMap += 1;
        }
    }

    void RespawnColliders()
    {
        for (auto* c : mColliders)
        {
            if (c->tag == "_destroyed_")
            {
                c->tag = "map_terrain";
                if (c->tile)
                    c->tile->destroyed = false;
            }

        }
        for (auto* m : mMonsters)
        {
            m->health = 100;
        }
    }

    void RunGame()
    {
        if (mSpawnPlayer)
            SpawnPlayer();
        DrawMap();
        PlayerInput();
        UpdatePlayer();
        if (!mProjectiles.empty())
            UpdateProjectile(*mProjectiles.back());
        DrawMonsters();
        // Debug collidables
        if (mDebugMode)
        {
            for (auto* c : mColliders)
            {
                if (c->tag == "_destroyed_")
                    continue;
                FillRectDecal(c->position - camera.vecCamPos, c->size, olc::RED);
            }
            DrawStringDecal({ 1.0f, 30.0f }, "Collidables: " + std::to_string(mPossibleCollidables), olc::WHITE, { 2.0f, 2.0f });
            DrawStringDecal({ 1.0f, 50.0f }, "Tiles Drawn: " + std::to_string(mTilesDrawnOnMap), olc::WHITE, { 2.0f, 2.0f });
        }
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        // Panic Key
        if (GetKey(olc::ESCAPE).bPressed)
            mGameState = 5;
        if (GetKey(olc::F11).bPressed)
            mDebugMode = !mDebugMode;
        if (GetKey(olc::F2).bPressed)
            mSpawnPlayer = true;
        if (GetKey(olc::F3).bPressed)
            RespawnColliders();
        switch (mGameState)
        {
        case GameState::SPLASHSCREEN:
            if (mSplashScreen.AnimateSplashScreen(fElapsedTime))
                return true;
            mGameState++;
            break;
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

        if (mDebugMode)
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
