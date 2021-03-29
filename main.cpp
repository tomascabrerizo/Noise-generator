#include "raylib.h"
#include "raymath.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

Image GenerateRandPattern(uint32_t screenWidth, uint32_t screenHeight)
{
    uint32_t imageWidth = screenWidth;
    uint32_t imageHeight = screenHeight;

    const uint32_t numColors = 3;
    Vector3 imageColors[numColors] = {
        { 0.4078, 0.4078, 0.3764 }, 
        { 0.7606, 0.6274, 0.6313 }, 
        { 0.8980, 0.9372, 0.9725 } 
    };

    Color* pixels = (Color*)malloc(imageWidth*imageHeight*sizeof(Color));
    for(int y = 0; y < imageHeight; y++)
    {
        for(int x = 0; x < imageWidth; x++)
        {
            uint32_t colorIndex = fminf(((float)rand()/RAND_MAX)*(float)numColors, 2);
            pixels[y*imageWidth+x].a = 255;
            pixels[y*imageWidth+x].r = imageColors[colorIndex].x * 255;
            pixels[y*imageWidth+x].g = imageColors[colorIndex].y * 255;
            pixels[y*imageWidth+x].b = imageColors[colorIndex].z * 255;
        }
    }

    Image resultImage = {};
    resultImage.data = pixels;
    resultImage.width = imageWidth;
    resultImage.height = imageHeight;
    resultImage.format = UNCOMPRESSED_R8G8B8A8;
    resultImage.mipmaps = 1;
    return resultImage;
}

Image GetImagePortion(Image image, uint32_t width, uint32_t height)
{

    Color* pixels = (Color*)malloc(width*height*sizeof(Color)); 
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            Color* otherPixels = (Color*)image.data;
            pixels[y*width+x] = otherPixels[y*image.width+x];
        }
    }
    Image resultImage = {};
    resultImage.data = pixels;
    resultImage.width = width;
    resultImage.height = height;
    resultImage.format = UNCOMPRESSED_R8G8B8A8;
    resultImage.mipmaps = 1;
    return resultImage;
}

Image MapImage(Image image, uint32_t width, uint32_t height)
{
    Color* pixels = (Color*)malloc(width*height*sizeof(Color)); 
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            uint32_t xIndex = ((float)x/(float)width)*image.width;
            uint32_t yIndex = ((float)y/(float)height)*image.height;
            Color* otherPixels = (Color*)image.data;
            pixels[y*width+x] = otherPixels[yIndex*image.width+xIndex];
        }
    }
    Image resultImage = {};
    resultImage.data = pixels;
    resultImage.width = width;
    resultImage.height = height;
    resultImage.format = UNCOMPRESSED_R8G8B8A8;
    resultImage.mipmaps = 1;
    return resultImage;
}

float SmoothStep(float t)
{
    return t * t * (3 - 2 * t);
}

Vector3 BilinearSmoothStep(Vector3 c00, Vector3 c10, Vector3 c01, Vector3 c11, float tx, float ty)
{
    float sx = SmoothStep(tx);
    float sy = SmoothStep(ty); 
    Vector3 a = Vector3Lerp(c00, c10, sx);
    Vector3 b = Vector3Lerp(c01, c11, sx);
    return Vector3Lerp(a, b, sy);
}

Vector3 Bilinear(Vector3 c00, Vector3 c10, Vector3 c01, Vector3 c11, float tx, float ty)
{
    Vector3 a = Vector3Lerp(c00, c10, tx);
    Vector3 b = Vector3Lerp(c01, c11, tx);
    return Vector3Lerp(a, b, ty);
}

Image BilinearInterpolatedImage(Image grid, uint32_t width, uint32_t height)
{

    uint32_t gridSizeX = grid.width-1;
    uint32_t gridSizeY = grid.height-1;
    
    Color* pixels = (Color*)malloc(width*height*sizeof(Color)); 
    for(int y = 0; y < height; y++)
    {
        for(int x = 0; x < width; x++)
        {
            float tx = ((float)x/(float)width) * (float)gridSizeX; 
            float ty = ((float)y/(float)height) * (float)gridSizeY; 
            int txi = (int)tx;
            int tyi = (int)ty;

            Color* otherPixels = (Color*)grid.data;
            Color color00 = otherPixels[tyi * grid.width + txi];
            Color color10 = otherPixels[tyi * grid.width + (txi+1)];
            Color color01 = otherPixels[(tyi+1) * grid.width + txi];
            Color color11 = otherPixels[(tyi+1) * grid.width + (txi+1)];
            
            Vector3 c00 = {color00.r/255.0f, color00.g/255.0f, color00.b/255.0f};
            Vector3 c10 = {color10.r/255.0f, color10.g/255.0f, color10.b/255.0f};
            Vector3 c01 = {color01.r/255.0f, color01.g/255.0f, color01.b/255.0f};
            Vector3 c11 = {color11.r/255.0f, color11.g/255.0f, color11.b/255.0f};
             
            Vector3 bilinearColor = BilinearSmoothStep(c00, c10, c01, c11, tx-txi, ty-tyi);
            pixels[y*width+x].a = 255;
            pixels[y*width+x].r = bilinearColor.x * 255;
            pixels[y*width+x].g = bilinearColor.y * 255;
            pixels[y*width+x].b = bilinearColor.z * 255;
        }
    }

    Image resultImage = {};
    resultImage.data = pixels;
    resultImage.width = width;
    resultImage.height = height;
    resultImage.format = UNCOMPRESSED_R8G8B8A8;
    resultImage.mipmaps = 1;
    return resultImage;
}

const int MAX_RAND_VALUES = 256;
float rValues[MAX_RAND_VALUES];
float rValues2d[MAX_RAND_VALUES*MAX_RAND_VALUES];
Vector2 rValuesPerlin[MAX_RAND_VALUES*MAX_RAND_VALUES];

void InitValueNoise1D(int seed)
{
    srand(seed);
    for(int i = 0; i < MAX_RAND_VALUES; i++)
    {
        rValues[i] = (float)rand()/(float)RAND_MAX; 
    }
}

void InitValueNoise2D(int seed)
{
    srand(seed);
    for(int i = 0; i < MAX_RAND_VALUES*MAX_RAND_VALUES; i++)
    {
        rValues2d[i] = (float)rand()/(float)RAND_MAX; 
    }
}

void InitPerlinNoise2D(int seed)
{
    srand(seed);
    for(int i = 0; i < MAX_RAND_VALUES*MAX_RAND_VALUES; i++)
    {
        //Init all values with random values between -1 .. 1
        rValuesPerlin[i].x = 2*((float)rand()/(float)RAND_MAX)-1;
        rValuesPerlin[i].y = 2*((float)rand()/(float)RAND_MAX)-1;
    }
}

float ValueNoise1D(float x)
{
    const int MASK = MAX_RAND_VALUES - 1;
    int xi = floorf(x);
    int xMin = xi & MASK;
    int xMax = (xMin + 1) & MASK;
    
    assert(xMin <= MAX_RAND_VALUES-1);
    assert(xMax <= MAX_RAND_VALUES-1);
    
    float t = x - xi;

    float tRemap = SmoothStep(t);
    return Lerp(rValues[xMin], rValues[xMax], tRemap);
}

float ValueNoise2D(float x, float y)
{
    const int MASK = MAX_RAND_VALUES - 1;
    int xi = floorf(x);
    int yi = floorf(y);
    
    int rx0 = xi & MASK;
    int rx1 = (rx0+1) & MASK;
    int ry0 = yi & MASK;
    int ry1 = (ry0+1) & MASK;
    
    float c00 = rValues2d[ry0*MASK+rx0];
    float c10 = rValues2d[ry0*MASK+rx1];
    float c01 = rValues2d[ry1*MASK+rx0];
    float c11 = rValues2d[ry1*MASK+rx1];

    float tx = x - xi;
    float ty = y - yi;
    
    float sx = SmoothStep(tx);
    float sy = SmoothStep(ty); 
    
    float lerpX0 = Lerp(c00, c10, sx); 
    float lerpX1 = Lerp(c01, c11, sx); 
    return Lerp(lerpX0, lerpX1, sy);
}

float PerlinNoise2D(float x, float y)
{
    const int MASK = MAX_RAND_VALUES - 1;
    int xi = floorf(x);
    int yi = floorf(y);
    
    int xi0 = xi & MASK;
    int xi1 = (xi0+1) & MASK;
    int yi0 = yi & MASK;
    int yi1 = (yi0+1) & MASK;
    
    //Gradient at the corner of the cell
    Vector2 c00 = rValuesPerlin[yi0*MASK+xi0];
    Vector2 c10 = rValuesPerlin[yi0*MASK+xi1];
    Vector2 c01 = rValuesPerlin[yi1*MASK+xi0];
    Vector2 c11 = rValuesPerlin[yi1*MASK+xi1];

    float tx = x - xi;
    float ty = y - yi;
    
    //Generate vector going from the grid point to p
    float x0 = tx; 
    float x1 = tx - 1; 
    float y0 = ty; 
    float y1 = ty - 1; 
    
    Vector2 p00 = {x0, y0};
    Vector2 p10 = {x1, y0};
    Vector2 p01 = {x0, y1};
    Vector2 p11 = {x1, y1};

    float sx = SmoothStep(tx);
    float sy = SmoothStep(ty); 

    float lerpX0 = Lerp(Vector2DotProduct(c00, p00), Vector2DotProduct(c10, p10), sx);
    float lerpX1 = Lerp(Vector2DotProduct(c01, p01), Vector2DotProduct(c11, p11), sx);
    return Lerp(lerpX0, lerpX1, sy);
}

int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 800;
    const int screenHeight = 450;

    InitWindow(screenWidth, screenHeight, "More noise");
    SetTargetFPS(60);

    Image randomImage = GenerateRandPattern(256, 256);
    Image portionImage = GetImagePortion(randomImage, 10, 10);
    Image mapedImage = MapImage(portionImage, 256, 256);
    Image interpolatedImage = BilinearInterpolatedImage(portionImage, 256, 256);

    Texture2D randomTexture = LoadTextureFromImage(randomImage);
    Texture2D portionTexture = LoadTextureFromImage(portionImage);
    Texture2D mapedTexture = LoadTextureFromImage(mapedImage);
    Texture2D interpolatedTexture = LoadTextureFromImage(interpolatedImage);
    
    UnloadImage(randomImage);
    UnloadImage(portionImage);
    UnloadImage(mapedImage);
    UnloadImage(interpolatedImage);
    
    InitValueNoise1D(16);
    InitValueNoise2D(16);
    InitPerlinNoise2D(16);

    //--------------------------------------------------------------------------------------
    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------


        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(DARKGRAY);
        
        DrawTexture(interpolatedTexture, 0, 0, WHITE);
       
        static float offsetX = 0.0f;
        static float offsetY = 0.0f;
        float speed = 0.3f;
        
        {   
            const int numSteps2d = 256;
            float step2d = 0.04f;
            int y2d = 0;
            int posX = 256;
            for(int y = 0.0; y < numSteps2d; y++)
            {
                int x2d = 0;
                for(int x = 0.0; x < numSteps2d; x++)
                {
                    float randValue = ValueNoise2D(x*step2d+offsetX, y*step2d+offsetY);
                    Color randColor = {};
                    randColor.a = 255;
                    randColor.r = randValue * 255;
                    randColor.g = randValue * 255;
                    randColor.b = randValue * 255;
                    DrawPixel(x2d+posX, y2d, randColor);
                    x2d++;
                }
                y2d++;
            }
        }
        
        {   
            const int numSteps2d = 256;
            float step2d = 0.04f;
            int y2d = 0;
            int posX = 512;
            for(int y = 0.0; y < numSteps2d; y++)
            {
                int x2d = 0;
                for(int x = 0.0; x < numSteps2d; x++)
                {
                    float randValue = (PerlinNoise2D(x*step2d+offsetX, y*step2d+offsetY) + 1) / 2;
                    Color randColor = {};
                    randColor.a = 255;
                    randColor.r = randValue * 255;
                    randColor.g = randValue * 255;
                    randColor.b = randValue * 255;
                    DrawPixel(x2d+posX, y2d, randColor);
                    x2d++;
                }
                y2d++;
            }
        }


        const int numSteps = 10;
        int x = 0;
        int x1 = 1;
        float step = 0.05f;
        for(float i = -10; i < numSteps; i+=step)
        {
            float randValue = ValueNoise1D(i+offsetX);
            float randValue1 = ValueNoise1D(i+step+offsetX);
            int y = randValue * 30 + 300;
            int y1 = randValue1 * 30 + 300;
            DrawLine(x++, y, x1++, y1, RED);
        }

        if(IsKeyDown(KEY_RIGHT)) offsetX += speed;
        if(IsKeyDown(KEY_LEFT)) offsetX -= speed;
        if(IsKeyDown(KEY_DOWN)) offsetY += speed;
        if(IsKeyDown(KEY_UP)) offsetY -= speed;

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(randomTexture);
    UnloadTexture(mapedTexture);
    UnloadTexture(portionTexture);
    UnloadTexture(interpolatedTexture);
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}
