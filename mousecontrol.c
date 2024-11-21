#include "raylib.h"
#include "raymath.h"  // possibly not necessary

#define FLT_MAX     340282346638528859811704183484516925440.0f     // Maximum value of a float, from bit pattern 01111111011111111111111111111111. I think Ray used this in the event of a rare edge case

// Player units which can be controlled
typedef struct Unit {
    Vector3 unitPos;  // X = East and West. Y = Up and Down. Z = North and South.
    Vector3 unitSize;
    float unitSpeed;
    bool selected;
} Unit;

BoundingBox createBoundingBox(Vector3 startPoint, Vector3 endPoint) // Creates a bounding box using its two corners
{
    BoundingBox boundingBox = { 0 };

    if (startPoint.x <= endPoint.x) boundingBox.min.x = startPoint.x, boundingBox.max.x = endPoint.x;  // Expected mouse drag direction
    else if (startPoint.x > endPoint.x) boundingBox.min.x = endPoint.x, boundingBox.max.x = startPoint.x;  // Unexpected 
    
    if (startPoint.z <= endPoint.z) boundingBox.min.z = startPoint.z, boundingBox.max.z = endPoint.z;  // Expected mouse drag direction
    else if (startPoint.z > endPoint.z) boundingBox.min.z = endPoint.z, boundingBox.max.z = startPoint.z;  // Unexpected direction

    boundingBox.min.y = 0.0f;
    boundingBox.max.y = 0.5f; // Arbitrary height value 
    
    return boundingBox;
}

int main(void)
{
    // Screen
    const int screenWidth = 1400;
    const int screenHeight = 850;
    InitWindow(screenWidth, screenHeight, "raylib example");

    // Camera
    Camera camera = { { 0.0f, 10.0f, 10.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, 0 };
    float cameraSpeed = 0.5;
    Ray ray = { 0 };


    // Ground quad. Allows mouse rays to collide with the ground and return a 3D coordinate
    Vector3 g0 = (Vector3){ -50.0f, 0.0f, -50.0f };
    Vector3 g1 = (Vector3){ -50.0f, 0.0f,  50.0f };
    Vector3 g2 = (Vector3){  50.0f, 0.0f,  50.0f };
    Vector3 g3 = (Vector3){  50.0f, 0.0f, -50.0f };

    RayCollision objectCollision = { 0 };  // For casting a ray at an OBJECT. Stores collision hit info
    RayCollision groundCollision = { 0 };  // For casting a ray at the GROUND in order to return 3D coordinates 
    Vector3 wayPoint = { 0 };  // Saves a 3D coordinate. Used for unit control 

    // Player
    Unit playerUnit = { { 0.0f, 1.0f, -4.0f }, { 1.0f, 2.0f, 1.0f }, 0.05f, false}; 
    bool moveUnit = false;  // will turn true when wayPoint is set and will be set to false when wayPoint coordinates are matched by unit coordinates

    // For drawing a visual aid rectangle with the mouse
    Vector2 saveMousePos2D = { 0 };
    // For drawing a bounding box in 3D space behind the above rectangle. Cannot draw the bounding box until mouse is released because user may drag upwards or downwards, left or right, and we cannot know where to put the min and the max until we have defined the intended collision space
    Vector3 save3DMouseStartPos = { 0 };
    Vector3 save3DMouseEndPos = { 0 };

    bool showDebug = false;  // option to show debug stats

    SetTargetFPS(60);  // Set our game to run at 60 frames-per-second

    // Main game loop -------------------------------------------------------------------------------
    while (!WindowShouldClose())
    {
        // Camera controls
        if (IsKeyDown(KEY_W)) camera.position.z -= cameraSpeed, camera.target.z -= cameraSpeed;
        if (IsKeyDown(KEY_A)) camera.position.x -= cameraSpeed, camera.target.x -= cameraSpeed;
        if (IsKeyDown(KEY_S)) camera.position.z += cameraSpeed, camera.target.z += cameraSpeed;
        if (IsKeyDown(KEY_D)) camera.position.x += cameraSpeed, camera.target.x += cameraSpeed;   
        if (IsKeyDown(KEY_Q)) camera.position.y += cameraSpeed, camera.target.y += cameraSpeed;  // CAMERA UP
        if (IsKeyDown(KEY_E)) camera.position.y -= cameraSpeed, camera.target.y -= cameraSpeed;  // CAMERA DOWN
        if ((IsKeyPressed(KEY_SPACE)) && (!showDebug)) showDebug = true;
        if ((IsKeyPressed(KEY_P)) && (showDebug)) showDebug = false;
        
        // re-implement the mechanic which returned the name of the object being pointed at (eg, tower, triangle, ground, etc.)
        groundCollision.distance = FLT_MAX;
        groundCollision.hit = false;

        // Get ray and test against objects
        ray = GetMouseRay(GetMousePosition(), camera);  // CAST THE RAY
        RayCollision groundHitInfo = GetRayCollisionQuad(ray, g0, g1, g2, g3);  // I THINK WE RETURN WHERE THE RAY IS TOUCHING THE GROUND QUAD 
        if ((groundHitInfo.hit) && (groundHitInfo.distance < groundCollision.distance)) groundCollision = groundHitInfo;  // just confirming that the cursor is making contact with the ground i think


        // MOUSE CONTROLS ----------------------------------------------------------------------
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            // At the start of each click, we assume that no object collision has occurred until we re-evaluate with below functions
            objectCollision.hit = false;

            // Saving coordinate to start as the top left corner of a visual aid rectangle
            saveMousePos2D.x = GetMousePosition().x;
            saveMousePos2D.y = GetMousePosition().y;  
            
            // Deciding where to put ONE CORNER of the cuboid which will we will use to define a bounding box. Y coordinate is static
            save3DMouseStartPos.x = groundCollision.point.x;
            save3DMouseStartPos.z = groundCollision.point.z;

            // Check collision between ray and an object (except the ground)
            objectCollision = GetRayCollisionBox(ray,
                                        (BoundingBox){(Vector3){ playerUnit.unitPos.x - playerUnit.unitSize.x/2, playerUnit.unitPos.y - playerUnit.unitSize.y/2, playerUnit.unitPos.z - playerUnit.unitSize.z/2 },
                                        (Vector3){ playerUnit.unitPos.x + playerUnit.unitSize.x/2, playerUnit.unitPos.y + playerUnit.unitSize.y/2, playerUnit.unitPos.z + playerUnit.unitSize.z/2 }});
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) 
        {
            // Defining where the other corner of our bounding box will be
            save3DMouseEndPos.x = groundCollision.point.x;
            save3DMouseEndPos.z = groundCollision.point.z;  // set these to 0 each time we iterate through the loop otherwise the boundingbox may persist and continue to register collision

            BoundingBox boundingBox = createBoundingBox(save3DMouseStartPos, save3DMouseEndPos);

            if (CheckCollisionBoxes(boundingBox, (BoundingBox){(Vector3){ playerUnit.unitPos.x - playerUnit.unitSize.x/2, playerUnit.unitPos.y - playerUnit.unitSize.y/2, playerUnit.unitPos.z - playerUnit.unitSize.z/2 },
                                            (Vector3){ playerUnit.unitPos.x + playerUnit.unitSize.x/2, playerUnit.unitPos.y + playerUnit.unitSize.y/2, playerUnit.unitPos.z + playerUnit.unitSize.z/2 }})) objectCollision.hit = true;
            
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
        {
            // Draws a rectangle as a visual aid to the player
            DrawRectangleLines(saveMousePos2D.x, saveMousePos2D.y, GetMousePosition().x - saveMousePos2D.x, GetMousePosition().y - saveMousePos2D.y, BLUE);
        }

        if ((IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) && (objectCollision.hit))// Move collided units with the mouse
        {
            // this method ties in movement speed with the framerate. possibly bad form!!!! once we have a timer, we will tie it to that
            if ((playerUnit.unitPos.x != groundCollision.point.x) || (playerUnit.unitPos.z != groundCollision.point.z)) 
            {
                moveUnit = true;
                wayPoint.x = groundCollision.point.x;
                wayPoint.z = groundCollision.point.z;
            }
        }
        // END OF MOUSE CONTROLS ----------------------------------------------------------------

        if (moveUnit) 
        {   // Checks to see whether the unit has reached the intended coordinates, in which case, moveUnit = false
            if ((playerUnit.unitPos.x == groundCollision.point.x) && (playerUnit.unitPos.z == groundCollision.point.z)) moveUnit = false;
            else 
            {
                if (playerUnit.unitPos.x < wayPoint.x) playerUnit.unitPos.x += playerUnit.unitSpeed;
                if (playerUnit.unitPos.x > wayPoint.x) playerUnit.unitPos.x -= playerUnit.unitSpeed;
                if (playerUnit.unitPos.z < wayPoint.z) playerUnit.unitPos.z += playerUnit.unitSpeed;
                if (playerUnit.unitPos.z > wayPoint.z) playerUnit.unitPos.z -= playerUnit.unitSpeed;
            }
        }

        // Draw
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                // Draw unit
                if (objectCollision.hit)  
                {
                    DrawCube(playerUnit.unitPos, playerUnit.unitSize.x, playerUnit.unitSize.y, playerUnit.unitSize.z, RED);
                    DrawCubeWires(playerUnit.unitPos, playerUnit.unitSize.x, playerUnit.unitSize.y, playerUnit.unitSize.z, MAROON);

                    DrawCubeWires(playerUnit.unitPos, playerUnit.unitSize.x + 0.2f, playerUnit.unitSize.y + 0.2f, playerUnit.unitSize.z + 0.2f, GREEN);
                }
                else
                {
                    DrawCube(playerUnit.unitPos, playerUnit.unitSize.x, playerUnit.unitSize.y, playerUnit.unitSize.z, GRAY);
                    DrawCubeWires(playerUnit.unitPos, playerUnit.unitSize.x, playerUnit.unitSize.y, playerUnit.unitSize.z, DARKGRAY);
                }

                // This lets me see where the invisible bounding box will be created for debugging purposes
                // DrawCube((Vector3){save3DMouseStartPos.x, 0.0f, save3DMouseStartPos.z}, 0.1f, 0.1f, 0.1f, GRAY);
                // DrawCube((Vector3){save3DMouseEndPos.x, 0.0f, save3DMouseEndPos.z}, 0.1f, 0.1f, 0.1f, GRAY);

                // Vector3 normalEnd;
                // normalEnd.x = groundCollision.point.x + groundCollision.normal.x;
                // normalEnd.y = groundCollision.point.y + groundCollision.normal.y;
                // normalEnd.z = groundCollision.point.z + groundCollision.normal.z;

                // CREATING A LITTLE CUBE THAT STICKS TO THE GROUND AND FOLLOWS THE MOUSE
                // DrawCube(groundCollision.point, 0.1f, 0.1f, 0.1f, GRAY);
                // DrawLine3D(groundCollision.point, normalEnd, RED);
                
                DrawGrid(10, 1.0f);        // Draw a grid (slices, spacing)

            EndMode3D();

            DrawText("Select cuboid with left click and control it with right click.", 400, 100, 20, GRAY);

            if (showDebug) 
            {
                DrawFPS(10, 10);
                DrawText(TextFormat("GetMousePosition()X = %f,\n\nY= %f", GetMousePosition().x, GetMousePosition().y), 30, 20, 30, LIGHTGRAY);
                DrawText(TextFormat("save3DStartPosX = %f,\n\nZ= %f", save3DMouseStartPos.x, save3DMouseStartPos.z), 30, 140, 30, LIGHTGRAY);
                DrawText(TextFormat("save3DEndPosX = %f,\n\nZ = %f", save3DMouseEndPos.x, save3DMouseEndPos.z), 30, 250, 30, LIGHTGRAY);
                DrawText(TextFormat("ray.direction.x = %f,\n\nray.direction.y = %f\n\nray.direction.z = %f", ray.direction.x, ray.direction.y, ray.direction.z), 30, 350, 30, LIGHTGRAY);
                DrawText(TextFormat("Hit Pos: %3.2f %3.2f %3.2f",
                                    groundCollision.point.x,
                                    groundCollision.point.y,
                                    groundCollision.point.z), 10, 70 + 15, 10, BLACK);

                DrawText(TextFormat("Hit Norm: %3.2f %3.2f %3.2f",
                                    groundCollision.normal.x,
                                    groundCollision.normal.y,
                                    groundCollision.normal.z), 10, 70 + 30, 10, BLACK);
            }

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}