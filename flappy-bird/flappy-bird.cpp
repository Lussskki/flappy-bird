#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>

using namespace std;

int main() {
    sf::RenderWindow window(sf::VideoMode(400, 600), "Flappy Bird");
    srand(static_cast<unsigned>(time(nullptr)));

    bool gameStarted = false, gameOver = false;
    int score = 0;

    // Set icon here
    sf::Image icon;
    if (!icon.loadFromFile("assets\\favicon.png")) {
        std::cerr << "Failed to load icon\n";
    }
    else {
        window.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());
    }

    // message sprite
    sf::Texture messageTexture;
    if (!messageTexture.loadFromFile("assets\\messages\\message.png")) {
        cerr << "Failed to load message.png\n";
        return -1;
    }
    sf::Sprite messageSprite(messageTexture);
    messageSprite.setOrigin(messageSprite.getLocalBounds().width / 2, messageSprite.getLocalBounds().height / 2);
    messageSprite.setPosition(window.getSize().x / 2.f, window.getSize().y / 3.f);

    // base
    sf::Texture baseTexture;
    if (!baseTexture.loadFromFile("assets\\enviroment\\base.png")) {
        cerr << "Failed to load base.png\n";
        return -1;
    }
    sf::Sprite baseSprite(baseTexture);
    baseSprite.setScale((float)window.getSize().x / baseTexture.getSize().x, 1.0f);
    float baseY = window.getSize().y - baseTexture.getSize().y;
    baseSprite.setPosition(0, baseY);

    // bird textures
    sf::Texture birdUp, birdMid, birdDown;
    if (!birdUp.loadFromFile("assets\\bird\\bluebird-upflap.png") ||
        !birdMid.loadFromFile("assets\\bird\\bluebird-midflap.png") ||
        !birdDown.loadFromFile("assets\\bird\\bluebird-downflap.png")) {
        cerr << "Failed to load bird textures\n";
        return -1;
    }
    vector<sf::Texture*> birdFrames = { &birdMid, &birdDown, &birdUp };
    int currentFrame = 0;
    float animTimer = 0, animInterval = 0.15f;

    sf::Sprite bird(*birdFrames[0]);
    bird.setScale(30.f / birdFrames[0]->getSize().x, 30.f / birdFrames[0]->getSize().y);
    bird.setPosition(100.f, 300.f);

    // physics
    sf::Clock clock;
    float gravity = 900.f, velocity = 0.f, jumpStrength = -350.f, rotation = 0.f;

    // game over
    sf::Texture gameoverTexture;
    if (!gameoverTexture.loadFromFile("assets\\messages\\gameover.png")) {
        cerr << "Failed to load gameover.png\n";
        return -1;
    }
    sf::Sprite gameoverSprite(gameoverTexture);
    gameoverSprite.setOrigin(gameoverSprite.getLocalBounds().width / 2, gameoverSprite.getLocalBounds().height / 2);
    gameoverSprite.setPosition(window.getSize().x / 2.f, window.getSize().y / 2.5f);

    // pipe
    sf::Texture pipeTexture;
    if (!pipeTexture.loadFromFile("assets\\pipe\\pipe-red.png")) {
        cerr << "Failed to load pipe-red.png\n";
        return -1;
    }
    vector<sf::Sprite> pipes;
    float pipeTimer = 0.f, pipeInterval = 2.f, pipeSpeed = 150.f, pipeScaleX = 2.f, gapSize = 120.f;

    // background
    sf::Texture bgTexture;
    if (!bgTexture.loadFromFile("assets\\enviroment\\background.png")) {
        cerr << "Failed to load background.png\n";
        return -1;
    }
    sf::Sprite bg(bgTexture);
    bg.setScale(
        (float)window.getSize().x / bgTexture.getSize().x,
        (float)window.getSize().y / bgTexture.getSize().y
    );

    // digits
    vector<sf::Texture> digits(10);
    for (int i = 0; i < 10; i++) {
        if (!digits[i].loadFromFile("assets\\numbers\\" + to_string(i) + ".png")) {
            cerr << "Missing digit: " << i << endl;
            return -1;
        }
    }

    vector<bool> scoredPipes;

    // sounds
    sf::SoundBuffer hitBuffer;
    sf::SoundBuffer dieBuffer;
    sf::SoundBuffer pointBuffer;
    sf::Sound hitSound, dieSound, pointSound;

    if (!hitBuffer.loadFromFile("audio\\hit.ogg")) {
        cerr << "Failed to load hit.ogg\n";
    }
    if (!dieBuffer.loadFromFile("audio\\die.ogg")) {
        cerr << "Failed to load die.ogg\n";
    }
    if (!pointBuffer.loadFromFile("audio\\point.ogg")) {
        cerr << "Failed to load point.ogg\n";
    }
    hitSound.setBuffer(hitBuffer);
    dieSound.setBuffer(dieBuffer);
    pointSound.setBuffer(pointBuffer);

    // swoosh 
    sf::SoundBuffer swooshBuffer;
    sf::Sound swooshSound;

    if (!swooshBuffer.loadFromFile("audio\\swoosh.ogg")) {
        cerr << "Failed to load swoosh.ogg\n";
    }
    swooshSound.setBuffer(swooshBuffer);


    // Game loop
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            // start game
            if (!gameStarted && (event.type == sf::Event::MouseButtonPressed ||
                (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space)))
                gameStarted = true;
                
                
            // ingame space
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Space && !gameOver) {
                velocity = jumpStrength;
                rotation = -25.f;
                swooshSound.play();
            }

            // restart
            if (gameOver && event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                pipes.clear();
                scoredPipes.clear();
                bird.setPosition(100.f, 300.f);
                bird.setRotation(0.f);
                velocity = 0.f;
                rotation = 0.f;
                score = 0;
                gameOver = false;
                gameStarted = false;
                currentFrame = 0;
                bird.setTexture(*birdFrames[0]);
            }

        }

        float dt = clock.restart().asSeconds();

        if (gameStarted && !gameOver) {
            velocity += gravity * dt;
            bird.move(0, velocity * dt);
            rotation += 20.f * dt;
            bird.setRotation(min(rotation, 20.f));

            // spawn pipes
            pipeTimer += dt;
            if (pipeTimer >= pipeInterval) {
                pipeTimer = 0;
                float screenH = window.getSize().y;
                float minTop = 60.f;
                float maxTop = screenH - gapSize - baseTexture.getSize().y - 20.f;
                float topH = minTop + rand() % (int)(maxTop - minTop + 1);

                // Top pipe
                sf::Sprite top(pipeTexture);
                top.setScale(pipeScaleX, -topH / pipeTexture.getSize().y);
                top.setPosition(window.getSize().x, topH);
                pipes.push_back(top);

                // Bottom pipe
                float bottomH = screenH - topH - gapSize;
                sf::Sprite bottom(pipeTexture);
                bottom.setScale(pipeScaleX, bottomH / pipeTexture.getSize().y);
                bottom.setPosition(window.getSize().x, topH + gapSize);
                pipes.push_back(bottom);

                scoredPipes.push_back(false);
            }

            // Move pipes
            for (auto& p : pipes)
                p.move(-pipeSpeed * dt, 0);

            // Remove offscreen pipes & scored flags
            while (!pipes.empty() && pipes.front().getPosition().x + pipes.front().getGlobalBounds().width < 0) {
                pipes.erase(pipes.begin(), pipes.begin() + 2); // remove top+bottom pair
                if (!scoredPipes.empty())
                    scoredPipes.erase(scoredPipes.begin());
            }

            // Collision box with some margin
            sf::FloatRect birdBox = bird.getGlobalBounds();
            birdBox.left += birdBox.width * 0.1f;
            birdBox.top += birdBox.height * 0.1f;
            birdBox.width *= 0.8f;
            birdBox.height *= 0.8f;

            // Check collision with pipes
            for (auto& p : pipes) {
                if (birdBox.intersects(p.getGlobalBounds())) {
                    if (!gameOver) hitSound.play();
                    gameOver = true;
                }
            }

            // Check collision with ground or top
            if (bird.getPosition().y + bird.getGlobalBounds().height / 2 >= baseY || bird.getPosition().y < 0) {
                if (!gameOver) hitSound.play();
                gameOver = true;
            }

            // Scoring
            for (size_t i = 0; i < scoredPipes.size(); ++i) {
                if (!scoredPipes[i]) {
                    float pipeX = pipes[2 * i].getPosition().x + pipes[2 * i].getGlobalBounds().width;
                    if (pipeX < bird.getPosition().x) {
                        score++;
                        pointSound.play();
                        scoredPipes[i] = true;
                    }
                }
            }

            if (gameOver) {
                dieSound.play();
            }
        }

        if (!gameOver) {
            animTimer += dt;
            if (animTimer >= animInterval) {
                animTimer = 0;
                currentFrame = (currentFrame + 1) % birdFrames.size();
                bird.setTexture(*birdFrames[currentFrame]);
            }
        }

        // Draw
        window.clear();
        window.draw(bg);
        for (auto& p : pipes) window.draw(p);
        window.draw(baseSprite);
        window.draw(bird);

        if (gameStarted && !gameOver) {
            string scoreStr = to_string(score);
            float scale = 2.f, totalW = 0;
            vector<sf::Sprite> scoreDigits;

            // Calculate total width for centering score digits
            for (char ch : scoreStr) {
                int d = ch - '0';
                sf::Sprite s(digits[d]);
                s.setScale(scale, scale);
                totalW += s.getGlobalBounds().width;
                scoreDigits.push_back(s);
            }

            float x = window.getSize().x / 2.f - totalW / 2.f;
            for (auto& s : scoreDigits) {
                s.setPosition(x, 50);
                window.draw(s);
                x += s.getGlobalBounds().width;
            }
        }

        if (!gameStarted && !gameOver)
            window.draw(messageSprite);
        else if (gameOver)
            window.draw(gameoverSprite);

        window.display();
    }

    return 0;
}
