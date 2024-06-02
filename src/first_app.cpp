//
// Created by Vlad Dancea on 28.03.24.
//

#include "first_app.h"

namespace vk {

    glm::mat2x3 FirstApp::loadBoundingBoxFromFile(const std::string& filename) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::vector<Model::Vertex> vertices;
        std::vector<uint32_t> indices;

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                Model::Vertex vertex{};

                if (index.vertex_index >= 0) {
                    vertex.position = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                    };

                    vertex.color = {
                            attrib.colors[3 * index.vertex_index + 0],
                            attrib.colors[3 * index.vertex_index + 1],
                            attrib.colors[3 * index.vertex_index + 2]
                    };
                }

                if (index.normal_index >= 0) {
                    vertex.normal = {
                            attrib.normals[3 * index.normal_index + 0],
                            attrib.normals[3 * index.normal_index + 1],
                            attrib.normals[3 * index.normal_index + 2]
                    };
                }

                if (index.texcoord_index >= 0) {
                    vertex.uv = {
                            attrib.texcoords[2 * index.normal_index + 0],
                            attrib.texcoords[2 * index.normal_index + 1],
                    };
                }

                vertices.push_back(vertex);
                indices.push_back(vertices.size() - 1);

            }
        }

        //calculate bounding box
        glm::vec3 currentMin = glm::vec3(std::numeric_limits<float>::max());
        glm::vec3 currentMax = glm::vec3(std::numeric_limits<float>::min());
        for (auto& vertex : vertices) {
            currentMin = glm::min(currentMin, vertex.position);
            currentMax = glm::max(currentMax, vertex.position);
        }

        return glm::mat2x3(currentMin, currentMax);
    }



    FirstApp::FirstApp() {

        globalPool = DescriptorPool::Builder(device)
                .setMaxSets(SwapChain::MAX_FRAMES_IN_FLIGHT)
                .addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, SwapChain::MAX_FRAMES_IN_FLIGHT)
                .build();
        loadGameObjects();

    }

    FirstApp::~FirstApp() {}

    void FirstApp::run() {


        std::vector<std::unique_ptr<Buffer>> uboBuffers(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < uboBuffers.size(); i++) {
            uboBuffers[i] = std::make_unique<Buffer>(device,
                                                     sizeof(GlobalUbo),
                                                     1,
                                                     VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
            uboBuffers[i]->map();
        }

        auto globalSetLayout = DescriptorSetLayout::Builder(device)
                .addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS)
                .build();

        std::vector<VkDescriptorSet> globalDescriptorSets(SwapChain::MAX_FRAMES_IN_FLIGHT);
        for (int i = 0; i < globalDescriptorSets.size(); i++) {
            auto bufferInfo = uboBuffers[i]->descriptorInfo();
            DescriptorWriter(*globalSetLayout, *globalPool)
                    .writeBuffer(0, &bufferInfo)
                    .build(globalDescriptorSets[i]);
        }

        SimpleRenderSystem simpleRenderSystem{device,
                                              renderer.getSwapChainRenderPass(),
                                              globalSetLayout->getDescriptorSetLayout()};

        PointLightSystem pointLightSystem{device,
                                          renderer.getSwapChainRenderPass(),
                                          globalSetLayout->getDescriptorSetLayout()};

        CrossHairSystem crossHairSystem{device,
                                        renderer.getSwapChainRenderPass(),
                                        globalSetLayout->getDescriptorSetLayout()};

        HudSystem hudSystem{device,
                            renderer.getSwapChainRenderPass(),
                            globalSetLayout->getDescriptorSetLayout()};


        glfwSetInputMode(window.getGLFWWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        KeyboardMovementController movementController{ WIDTH, HEIGHT };


        auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = startTime;

        int currentSecond = 0;
        float gameTimer = 0;
        
        while (!window.shouldClose()) {
            glfwPollEvents();

            // TODO callbacks for jumping, shooting and menu for better responsiveness

            auto newTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(newTime - currentTime).count();
            currentTime = newTime;

            deltaTime = std::min(deltaTime, MAX_FRAME_TIME);

            movementController.handleEscMenu(window.getGLFWWindow());


            if (!movementController.escapeMenuOpen) {

                gameTimer += deltaTime;

                // debug timer output
                int newSecond = static_cast<int>(gameTimer);
                if (newSecond > currentSecond) {
                    currentSecond = newSecond;
                    std::cout << "Time since start: " << currentSecond << "s" << std::endl;
                }

                movementController.handleMovement(window.getGLFWWindow(), *sceneManager->getPlayer());
                movementController.handleRotation(window.getGLFWWindow(), deltaTime, *sceneManager->getPlayer());

                for (std::weak_ptr<physics::Enemy> weak_enemy : sceneManager->getActiveEnemies()) {
                    std::shared_ptr<physics::Enemy> enemy = weak_enemy.lock();
                    if (!enemy) {
                        continue;
                    }
                    enemy->update();
                }

                // TODO maybe read this in via a settings file and recalculate only if setting is changed via menu (performance, typically no dynamic window scaling during runtime in games)
                float aspect = renderer.getAspectRatio();
                // switch between orthographic and perspective projection
                // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
                sceneManager->getPlayer()->setPerspectiveProjection(glm::radians(60.0f), aspect, 0.1f,
                                                100.0f); // objects further away than 100 are clipped

                // simulate physics step
                physicsSimulation->simulate();

                if (auto commandBuffer = renderer.beginFrame()) {
                    int frameIndex = renderer.getFrameIndex();
                    FrameInfo frameInfo{deltaTime,
                                        commandBuffer,
                                        globalDescriptorSets[frameIndex],
                                        *sceneManager};

                    // TODO handle clicking (raycast + damage)
                    // movementController.handleClicking(window.getGLFWWindow(), deltaTime, frameInfo);

                    //update
                    GlobalUbo ubo{};
                    ubo.projection = sceneManager->getPlayer()->getProjMat();
                    ubo.view = sceneManager->getPlayer()->calculateViewMat();
                    ubo.inverseView = glm::inverse(ubo.view);
                    ubo.aspectRatio = aspect;
                    pointLightSystem.update(frameInfo, ubo);
                    uboBuffers[frameIndex]->writeToBuffer(&ubo);
                    uboBuffers[frameIndex]->flush();

                    //render
                    renderer.beginSwapChainRenderPass(commandBuffer);
                    simpleRenderSystem.renderGameObjects(frameInfo);
                    pointLightSystem.render(frameInfo);
                    crossHairSystem.renderGameObjects(frameInfo);
                    hudSystem.renderGameObjects(frameInfo, movementController.escapeMenuOpen);
                    renderer.endSwapChainRenderPass(commandBuffer);
                    renderer.endFrame();
                }
            } else {

                if (auto commandBuffer = renderer.beginFrame()) {
                    int frameIndex = renderer.getFrameIndex();
                    FrameInfo frameInfo{deltaTime,
                                        commandBuffer,
                                        globalDescriptorSets[frameIndex],
                                        *sceneManager};

                    //render
                    renderer.beginSwapChainRenderPass(commandBuffer);
                    simpleRenderSystem.renderGameObjects(frameInfo);
                    pointLightSystem.render(frameInfo);
                    crossHairSystem.renderGameObjects(frameInfo);
                    hudSystem.renderGameObjects(frameInfo, movementController.escapeMenuOpen);
                    renderer.endSwapChainRenderPass(commandBuffer);
                    renderer.endFrame();
                }
            }
            vkDeviceWaitIdle(device.device());
        }
    }

    void FirstApp::loadGameObjects() {

        // inject scene manager into physics simulation
        this->sceneManager = make_shared<SceneManager>();
        this->physicsSimulation = make_unique<physics::PhysicsSimulation>(this->sceneManager);

        bool usingTriangles = true;
        std::shared_ptr<Model> flatVaseModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/flat_vase.obj");
        std::shared_ptr<Model> smoothVaseModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/smooth_vase.obj");
        std::shared_ptr<Model> floorModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/quad.obj");
        std::shared_ptr<Model> humanModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/Char_Base.obj");
        std::shared_ptr<Model> crossHairModel = Model::createModelFromFile(!usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/crosshair.obj");
        std::shared_ptr<Model> closeTextModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/CloseText.obj");
        std::shared_ptr<Model> toggleFullscreenTextModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/ToggleFullScreenText.obj");
        std::shared_ptr<Model> blackScreenTextModel = Model::createModelFromFile(usingTriangles, device, std::string(PROJECT_SOURCE_DIR) + "/assets/models/BlackScreen.obj");
        glm::mat2x3 boundingBox = loadBoundingBoxFromFile(std::string(PROJECT_SOURCE_DIR) + "/assets/models/Char_Base.obj");


        // 2m player
        float playerHeight = 1.40f;
        float playerRadius = 0.3f;
        Ref<Shape> characterShape = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * playerHeight + playerRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * playerHeight, playerRadius)).Create().Get();

        std::unique_ptr<CharacterCameraSettings> cameraSettings = std::make_unique<CharacterCameraSettings>();
        cameraSettings->cameraOffsetFromCharacter = glm::vec3(0.0f, 0.8f, 0.0f);

        std::unique_ptr<physics::PlayerSettings> playerSettings = std::make_unique<physics::PlayerSettings>();

        std::unique_ptr<JPH::CharacterSettings> characterSettings = std::make_unique<JPH::CharacterSettings>();
        characterSettings->mGravityFactor = 0.5f;
        characterSettings->mFriction = 10.0f;
        characterSettings->mShape = characterShape;
        characterSettings->mLayer = physics::Layers::MOVING;
        characterSettings->mSupportingVolume = Plane(Vec3::sAxisY(), -playerRadius); // Accept contacts that touch the lower sphere of the capsule

        std::unique_ptr<physics::PlayerCreationSettings> playerCreationSettings = std::make_unique<physics::PlayerCreationSettings>();
        playerCreationSettings->characterSettings = std::move(characterSettings);
        playerCreationSettings->cameraSettings = std::move(cameraSettings);
        playerCreationSettings->playerSettings = std::move(playerSettings);

        sceneManager->setPlayer(std::move(std::make_unique<physics::Player>(std::move(playerCreationSettings), physicsSimulation->getPhysicsSystem())));

        // add terrain to scene
        sceneManager->addManagedPhysicsEntity(std::move(std::make_unique<physics::Terrain>(physicsSimulation->getPhysicsSystem(), glm::vec3{ 0.569, 0.29, 0 }, floorModel, glm::vec3{ 0.0, -1.0, 0.0 }, glm::vec3{ 50.0f, 1.0f, 50.0f })));
 
        // add point lights
        sceneManager->addLight(std::move(make_unique<lighting::PointLight>(1.2f, 0.1f, glm::vec3{ 1.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f })));
        sceneManager->addLight(std::move(make_unique<lighting::PointLight>(1.2f, 0.1f, glm::vec3{ 0.0f, 1.0f, 0.0f }, glm::vec3{ 1.0f, 1.0f, 0.0f })));
        sceneManager->addLight(std::move(make_unique<lighting::PointLight>(1.2f, 0.1f, glm::vec3{ 0.0f, 0.0f, 1.0f }, glm::vec3{ 2.0f, 1.0f, 0.0f })));

        // add ui
        sceneManager->addUIObject(std::move(make_unique<vk::UIComponent>(crossHairModel, true, false)));
        sceneManager->addUIObject(std::move(make_unique<vk::UIComponent>(blackScreenTextModel, false, true, glm::vec3{ -1.0f, -1.0f, 0.0f }, glm::vec3{ 30.0f, 30.0f, 30.0f })));
        sceneManager->addUIObject(std::move(make_unique<vk::UIComponent>(closeTextModel, false, true, glm::vec3{ -0.9f, 0.9f, 0.0f }, glm::vec3{ 0.1f, 0.1f, 0.1f })));
        sceneManager->addUIObject(std::move(make_unique<vk::UIComponent>(toggleFullscreenTextModel, false, true, glm::vec3{ -0.5f, 0.9f, 0.0f }, glm::vec3{ 0.1f, 0.1f, 0.1f })));

        // this must fit the model
        float enemyHullHeight = 1.25f;
        float enemyRadius = 0.3f;

        JPH::RotatedTranslatedShapeSettings enemyShapeSettings = RotatedTranslatedShapeSettings(Vec3(0, 0.5f * enemyHullHeight + enemyRadius, 0), Quat::sIdentity(), new CapsuleShape(0.5f * enemyHullHeight, enemyRadius));

        for (int i = 0; i < 5; ++i) {

            Ref<Shape> enemyShape = enemyShapeSettings.Create().Get();

            std::unique_ptr<physics::SprinterSettings> sprinterSettings = std::make_unique<physics::SprinterSettings>();
            sprinterSettings->model = humanModel;

            std::unique_ptr<JPH::CharacterSettings> enemyCharacterSettings = std::make_unique<JPH::CharacterSettings>();
            enemyCharacterSettings->mLayer = physics::Layers::MOVING;
            enemyCharacterSettings->mSupportingVolume = Plane(Vec3::sAxisY(), -enemyRadius); // Accept contacts that touch the lower sphere of the capsule
            enemyCharacterSettings->mFriction = 10.0f;
            enemyCharacterSettings->mShape = enemyShape;

            std::unique_ptr<physics::SprinterCreationSettings> sprinterCreationSettings = std::make_unique<physics::SprinterCreationSettings>();
            sprinterCreationSettings->sprinterSettings = std::move(sprinterSettings);
            sprinterCreationSettings->characterSettings = std::move(enemyCharacterSettings);
            sprinterCreationSettings->position = RVec3(3.0f * i, 3.0f, 0.0f);

            sceneManager->addEnemy(std::move(make_unique<physics::Sprinter>(std::move(sprinterCreationSettings), physicsSimulation->getPhysicsSystem())));
        }

        glfwSetWindowUserPointer(window.getGLFWWindow(), sceneManager.get());
    }
}