//
// Created by Vlad Dancea on 28.03.24.
//

#include "first_app.h"

namespace vk {

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

        Camera camera{};

        // switch between looking at a position and looking in a direction
        //camera.setViewDirection(glm::vec3{0.0f}, glm::vec3{0.5f, 0.0f, 1.0f});
        camera.setViewTarget(glm::vec3(-1.0f, -2.0f, -2.0f), glm::vec3{0.0f, 0.0f, 2.5f});


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
                camera.setViewYXZ(viewerObject.transform.translation, viewerObject.transform.rotation);

                float aspect = renderer.getAspectRatio();
                // switch between orthographic and perspective projection
                // camera.setOrthographicProjection(-aspect, aspect, -1, 1, -1, 1);
                camera.setPerspectiveProjection(glm::radians(50.0f), aspect, 0.1f,
                                                100.0f); // objects further away than 100 are clipped

                // simulate physics step
                physicsSimulation->simulate();

                if (auto commandBuffer = renderer.beginFrame()) {
                    int frameIndex = renderer.getFrameIndex();
                    FrameInfo frameInfo{deltaTime,
                                        commandBuffer,
                                        camera,
                                        globalDescriptorSets[frameIndex],
                                        *sceneManager};
                    movementController.handleClicking(window.getGLFWWindow(), deltaTime, frameInfo);
                    //update
                    GlobalUbo ubo{};
                    ubo.projection = camera.getProjection();
                    ubo.view = camera.getView();
                    ubo.inverseView = camera.getInverseView();
                    ubo.aspectRatio = aspect;
                    pointLightSystem.update(frameInfo, ubo);
                    simpleRenderSystem.update(frameInfo, ubo, camera);
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
                                        camera,
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

        // couple scene manager and physics simulation
        this->sceneManager = make_unique<SceneManager>();
        this->physicsSimulation = make_unique<physics::PhysicsSimulation>(this->sceneManager.get());
        this->sceneManager->setPhysicsSystem(physicsSimulation->getPhysicsSystem());

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

        CharacterCameraSettings cameraSettings = {};
        cameraSettings.fov = field_of_view;
        cameraSettings.aspectRatio = aspect_ratio;
        cameraSettings.nearPlane = near_plane_distance;
        cameraSettings.farPlane = far_plane_distance;
        cameraSettings.initialYaw = camera_yaw;
        cameraSettings.initialPitch = camera_pitch;
        cameraSettings.cameraOffsetFromCharacter = glm::vec3(0.0f, 0.8f, 0.0f);

        PlayerSettings playerSettings = {};

        CharacterSettings characterSettings = {};
        characterSettings.mGravityFactor = 1.0f;
        characterSettings.mFriction = 10.0f;
        characterSettings.mShape = characterShape;
        characterSettings.mLayer = Layers::MOVING;
        characterSettings.mSupportingVolume = Plane(Vec3::sAxisY(), -playerRadius); // Accept contacts that touch the lower sphere of the capsule

        PlayerCreationSettings playerCreationSettings = {};
        playerCreationSettings.characterSettings = &characterSettings;
        playerCreationSettings.cameraSettings = &cameraSettings;
        playerCreationSettings.playerSettings = &playerSettings;

        sceneManager->setPlayer(std::move(make_unique<Player>(&playerCreationSettings, physicsSimulation->getPhysicsSystem())));

        // add terrain to scene
        sceneManager->addManagedPhysicsEntity(std::move(make_unique<Terrain>(*physicsSimulation->getPhysicsSystem())));

        glfwSetWindowUserPointer(window.getGLFWWindow(), sceneManager->getPlayer());


        GameObject gameObject1{};
        gameObject1.model = flatVaseModel;
        gameObject1.transform.translation = {-0.5f, 0.5f, 0.0f};
        gameObject1.transform.scale = {3.0f, 1.5f, 3.0f};
        gameObject1.isEntity = std::make_unique<bool>(true);
        gameObjects.emplace(gameObject1.getId(), std::move(gameObject1));

        auto gameObject2 = GameObject::createGameObject();
        gameObject2.model = smoothVaseModel;
        gameObject2.transform.translation = {0.5f, 0.5f, 0.0f};
        gameObject2.transform.scale = {3.0f, 1.5f, 3.0f};
        gameObject2.isEntity = std::make_unique<bool>(true);
        gameObjects.emplace(gameObject2.getId(), std::move(gameObject2));

        auto gameObject3 = GameObject::createGameObject();
        gameObject3.model = floorModel;
        gameObject3.transform.translation = {0.0f, 0.5f, 0.0f};
        gameObject3.transform.scale = {3.0f, 1.0f, 3.0f};
        gameObject3.isEntity = std::make_unique<bool>(true);
        gameObjects.emplace(gameObject3.getId(), std::move(gameObject3));

        for (int i = 0; i < 5; ++i) {
            auto gameObject4 = GameObject::createGameObject();
            gameObject4.model = humanModel;
            gameObject4.transform.translation = {3.0f * i, 0.0f, 0.0f};
            gameObject4.transform.scale = {1.0f, 1.0f, 1.0f};
            gameObject4.isEntity = std::make_unique<bool>(true);
            gameObject4.isEnemy = std::make_unique<bool>(true);
            // set bounding box with regard to translation
            gameObject4.boundingBox = boundingBox;
            gameObject4.boundingBox[0] += gameObject4.transform.translation;
            gameObject4.boundingBox[1] += gameObject4.transform.translation;
            gameObjects.emplace(gameObject4.getId(), std::move(gameObject4));
        }


        auto crossHair = GameObject::createGameObject();
        crossHair.model = crossHairModel;
        crossHair.transform.translation = {0.0f, 0.0f, 0.0f};
        crossHair.isCrossHair = std::make_unique<bool>(true);
        gameObjects.emplace(crossHair.getId(), std::move(crossHair));

        auto blackScreenTextObject = GameObject::createGameObject();
        blackScreenTextObject.model = blackScreenTextModel;
        blackScreenTextObject.transform.translation = {-1.0f, -1.0f, 0.0f};
        blackScreenTextObject.transform.scale = {30.0f, 30.0f, 30.0f};
        blackScreenTextObject.isHud = std::make_unique<bool>(true);
        gameObjects.emplace(blackScreenTextObject.getId(), std::move(blackScreenTextObject));

        auto closeTextObject = GameObject::createGameObject();
        closeTextObject.model = closeTextModel;
        closeTextObject.transform.translation = {-0.9f, 0.9f, 0.0f};
        closeTextObject.transform.scale = {0.1f, 0.1f, 0.1f};
        closeTextObject.isHud = std::make_unique<bool>(true);
        gameObjects.emplace(closeTextObject.getId(), std::move(closeTextObject));

        auto toggleFullscreenTextObject = GameObject::createGameObject();
        toggleFullscreenTextObject.model = toggleFullscreenTextModel;
        toggleFullscreenTextObject.transform.translation = {-0.5f, 0.9f, 0.0f};
        toggleFullscreenTextObject.transform.scale = {0.1f, 0.1f, 0.1f};
        toggleFullscreenTextObject.isHud = std::make_unique<bool>(true);
        gameObjects.emplace(toggleFullscreenTextObject.getId(), std::move(toggleFullscreenTextObject));



        auto pointLight1 = GameObject::makePointLight(1.2f);
        pointLight1.color = {1.0f, 0.0f, 0.0f};
        gameObjects.emplace(pointLight1.getId(), std::move(pointLight1));

        auto pointLight2 = GameObject::makePointLight(1.2f);
        pointLight2.color = {0.0f, 1.0f, 0.0f};
        pointLight2.transform.translation = {1.0f, 0.0f, 0.0f};
        gameObjects.emplace(pointLight2.getId(), std::move(pointLight2));

        auto pointLight3 = GameObject::makePointLight(1.2f);
        pointLight3.color = {0.0f, 0.0f, 1.0f};
        pointLight3.transform.translation = {2.0f, 0.0f, 0.0f};
        gameObjects.emplace(pointLight3.getId(), std::move(pointLight3));
    }



}