#include <iostream>
#include <string>

#include "Mp4Recorder.h"

int main()
{
    int nFrameToRecord = 600; // 10s

    // Find window video folder
    const char filepath[] = "./test.mp4";

    // Create pseudo video memory
    int width = 100;
    int height = 100;
    int fps = 60;
    char* buffer = new char[width * height * 4];
    memset(buffer, 0, width * height * 4);

    Mp4Recorder rec(filepath, width, height, fps);

    if (!rec.init())
    {
        std::cout << "Fail to init mp4recorder\n";
        goto end;
    }

    if (!rec.start())
    {
        std::cout << "Fail to start mp4recorder\n";
        goto end;
    }

    // Record loop
    int i, j, k;

    for (k = 0; k < nFrameToRecord; k++)
    {
        // Write frame to memory
        for (i = 0; i < height; i++)
        {
            int startRowIdx = width * 4 * i;
            for (j = 0; j < width; j++)
            {
                int startIdx = startRowIdx + 4 * j;

                if (buffer[startIdx + 0] >= 255) buffer[startIdx + 0] = 0;
                else buffer[startIdx + 0]++;

                if (buffer[startIdx + 1] >= 255) buffer[startIdx + 1] = 0;
                else buffer[startIdx + 1]++;

                buffer[startIdx + 2] = 0;
            }
        }

        // Record
        rec.put(buffer);
    }


end :
    rec.end();
    std::cout << "Finish!\n";
}