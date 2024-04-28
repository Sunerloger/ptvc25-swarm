int main(int argc, char** argv) {


    // set up physics system
    std::unique_ptr<physics::PhysicsSimulation> physicsSimulation(new physics::PhysicsSimulation());

    // add player
    PhysicsSystem* physics_system = physicsSimulation->getPhysicsSystem();

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

    std::shared_ptr<Player> player(new Player{&playerCreationSettings, physics_system});
    physicsSimulation->setPlayer(player.get());

    // create terrain
    std::shared_ptr<Terrain> terrain(new Terrain{*physics_system});

    // create scenes
    std::shared_ptr<Scene> terrainScene(new Scene());
    // name is used to identify a scene and to remove it afterwards
    terrainScene->name = "terrainScene";
    terrainScene->physicsObjects.push_back(terrain);

    // add scenes to physics system
    physicsSimulation->addScene(terrainScene.get());

    // manually keep track of active scenes and active player -> hand only active GameObjects to renderer ! (active = added to physics system)
    // TODO improve this by keeping track of active game objects automatically


    glfwSetWindowUserPointer(window, player.get());

    // Establish a callback function for handling mouse scroll events:
    glfwSetScrollCallback(window, scrollCallbackFromGlfw);
    /* --------------------------------------------- */
    // Subtask 1.10: Set-up the Render Loop
    // Subtask 1.11: Register a Key Callback
    /* --------------------------------------------- */

    glfwSetKeyCallback(window, keyCallbackFromGlfw);

    float health = 100.0f;

    double lastTime = glfwGetTime();

    //FPV
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouseCallbackFromGlfw);


    vklEnablePipelineHotReloading(window, GLFW_KEY_F5);
    while (!glfwWindowShouldClose(window)) {

        // Handle user input:
        glfwPollEvents();

        double currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        Vec3 movement_direction = getMovementDirection();
        bool playerIsJump = getPlayerIsJump();

        // only update if something happened
        if (movement_direction != Vec3{ 0,0,0 } || playerIsJump) {
            player->handleMovement(movement_direction, playerIsJump);
        }


        physicsSimulation->simulate();