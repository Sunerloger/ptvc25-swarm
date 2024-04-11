
enum KeyState {
    KEY_NOT_PRESSED,
    KEY_PRESSED,
    KEY_HELD_DOWN
};

// static = limited to this .c file
static float g_zoom = 5.0f;

// factor deltaTime does nothing if not set (default=1)
static double deltaTime = 1;

void mouseCallbackFromGlfw(GLFWwindow* window, double xpos, double ypos) {
    static double lastX = 800, lastY = 800;
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // right = negative
    double xoffset = lastX - xpos;

    // down = negative
    double yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    Player* player = static_cast<Player*>(glfwGetWindowUserPointer(window));
    player->handleRotation(xoffset, yoffset, deltaTime);
}


/*!
 *	This callback function gets invoked by GLFW during glfwPollEvents() if there was
 *	mouse scroll input that can be processed by our application.
 */
void scrollCallbackFromGlfw(GLFWwindow* glfw_window, double xoffset, double yoffset) { g_zoom -= static_cast<float>(yoffset) * 0.5f; }



KeyState keys[1024];

void keyCallbackFromGlfw(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, true);
        return;
    }

    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS) {
            keys[key] = KEY_PRESSED;
        }
        else if (action == GLFW_RELEASE) {
            keys[key] = KEY_NOT_PRESSED;
        }
        else if (action == GLFW_REPEAT) {
            keys[key] = KEY_HELD_DOWN;
        }
    }

    if (key == GLFW_KEY_F1) {
        g_polygon_mode_index = 1 - g_polygon_mode_index;
    }
    if (key == GLFW_KEY_F2) {
        g_culling_index = (g_culling_index + 1) % 3;
    }
    if (key == GLFW_KEY_N) {
        g_draw_normals = !g_draw_normals;
    }
    if (key == GLFW_KEY_T) {
        g_draw_texcoords = !g_draw_texcoords;
    }
}

Vec3 getMovementDirection() {
    Vec3 movementDirection = RVec3::sZero();

    if (keys[GLFW_KEY_UP] != KEY_NOT_PRESSED || keys[GLFW_KEY_W] != KEY_NOT_PRESSED) {
        movementDirection += Vec3{ 0,0,-1 };
    }
    if (keys[GLFW_KEY_DOWN] != KEY_NOT_PRESSED || keys[GLFW_KEY_S] != KEY_NOT_PRESSED) {
        movementDirection += Vec3{ 0,0,1 };
    }
    if (keys[GLFW_KEY_LEFT] != KEY_NOT_PRESSED || keys[GLFW_KEY_A] != KEY_NOT_PRESSED) {
        movementDirection += Vec3{ -1,0,0 };
    }
    if (keys[GLFW_KEY_RIGHT] != KEY_NOT_PRESSED || keys[GLFW_KEY_D] != KEY_NOT_PRESSED) {
        movementDirection += Vec3{ 1,0,0 };
    }

    return movementDirection.NormalizedOr(Vec3{0,0,0});
}

bool getPlayerIsJump() {
    bool isJump = false;

    if (keys[GLFW_KEY_SPACE] != KEY_NOT_PRESSED) {
        isJump = true;
    }

    return isJump;
}

/* --------------------------------------------- */
// Main
/* --------------------------------------------- */

int main(int argc, char** argv) {
    VKL_LOG(":::::: WELCOME TO GCG 2023 ::::::");

    CMDLineArgs cmdline_args;
    gcgParseArgs(cmdline_args, argc, argv);

    /* --------------------------------------------- */
    // Subtask 1.1: Load Settings From File
    /* --------------------------------------------- */

    int window_width = 900;
    int window_height = 900;
    bool fullscreen = false;
    std::string window_title = "Swarm";
    INIReader window_reader("assets/settings/window.ini");

    std::string init_camera_filepath = "assets/settings/camera_front.ini";
    if (cmdline_args.init_camera) {
        init_camera_filepath = cmdline_args.init_camera_filepath;
    }
    INIReader camera_reader(init_camera_filepath);

    float field_of_view = static_cast<float>(camera_reader.GetReal("camera", "fov", 60.0f));
    float near_plane_distance = static_cast<float>(camera_reader.GetReal("camera", "near", 0.1f));
    float far_plane_distance = static_cast<float>(camera_reader.GetReal("camera", "far", 100.0f));
    float aspect_ratio = static_cast<float>(window_width) / static_cast<float>(window_height);
    float camera_yaw = static_cast<float>(camera_reader.GetReal("camera", "yaw", 0.0f));
    float camera_pitch = static_cast<float>(camera_reader.GetReal("camera", "pitch", 0.0f));
    std::string init_renderer_filepath = "assets/settings/renderer_standard.ini";
    if (cmdline_args.init_renderer) {
        init_renderer_filepath = cmdline_args.init_renderer_filepath;
    }
    INIReader renderer_reader(init_renderer_filepath);
    bool as_wireframe = renderer_reader.GetBoolean("renderer", "wireframe", false);
    if (as_wireframe) {
        g_polygon_mode_index = 1;
    }
    bool with_backface_culling = renderer_reader.GetBoolean("renderer", "backface_culling", false);
    if (with_backface_culling) {
        g_culling_index = 1;
    }
    g_draw_normals = renderer_reader.GetBoolean("renderer", "normals", false);
    g_draw_texcoords = renderer_reader.GetBoolean("renderer", "texcoords", false);
    bool depthtest = renderer_reader.GetBoolean("renderer", "depthtest", true);

   




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