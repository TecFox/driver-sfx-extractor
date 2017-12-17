/*****************************************************************************************
 * A tool which extracts all sounds from the BLK files used in Driver 1 (PSX) & Driver 2 *
 * Copyright (C) 2017 TecFox                                                             *
 *****************************************************************************************/

/***************************************************************************
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 2 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.             *
 *                                                                         *
 ***************************************************************************/

/******************************************************************************************************************************************
 *
 * Developer notes
 * ---------------
 *
 * I know the code is a bit of a mess but since it's a program of the kind "use-once-and-throw-away" I have no intention of optimizing it.
 * Most of the code is more or less a port of its VB6 prototype. It "just works" and that's all I care about.
 *
 * Please note: Don't run this program with any file other than VOICES.BLK and VOICES2.BLK. It WILL crash (Duh).
 * Since these files have no header it's impossible to verify them so the program will blindly assume that you're giving it a valid one.
 *
 ******************************************************************************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <math.h>

// WAV file header and smpl-chunk template - Quick and dirty solution because I'm lazy
int wavHeader[11] = {
    0x46464952, 0, 0x45564157, 0x20746D66,
    0x10, 0x10001, 0, 0,
    0x100002, 0x61746164, 0
};

int smplChunk[17] = {
    0x6C706D73, 0x3C, 0, 0,
    0x1D316, 0x3C, 0, 0,
    0, 1, 0, 0,
    0, 0, 0, 0,
    0
};

// PSX ADPCM coefficients
const double K0[5] = {0, 0.9375, 1.796875, 1.53125, 1.90625};
const double K1[5] = {0, 0, -0.8125, -0.859375, -0.9375};

// A wrapper for malloc so you don't have to explicitly check for a NULL pointer after every call
void* emalloc(size_t size)
{
    void* v = malloc(size);
    if(!v){
        fprintf(stderr, "An error occurred while allocating memory.\n");
        exit(EXIT_FAILURE);
    }
    return v;
}

// PSX ADPCM decoding routine - decodes a single sample
short vagToPcm(unsigned char soundParameter, int soundData, double* vagPrev1, double* vagPrev2)
{
    int resultInt = 0;
    double dTmp1 = 0.0;
    double dTmp2 = 0.0;
    double dTmp3 = 0.0;

    if (soundData > 7) soundData -= 16;

    dTmp1 = (double)soundData * pow(2, (double)(12 - (soundParameter & 0x0F)));

    dTmp2 = (*vagPrev1) * K0[(soundParameter >> 4) & 0x0F];
    dTmp3 = (*vagPrev2) * K1[(soundParameter >> 4) & 0x0F];

    (*vagPrev2) = (*vagPrev1);
    (*vagPrev1) = dTmp1 + dTmp2 + dTmp3;

    resultInt = (int)round((*vagPrev1));

    if (resultInt > 32767) resultInt = 32767;
    if (resultInt < -32768) resultInt = -32768;

    return (short)resultInt;
}

// Main decoding routine - Takes PSX ADPCM formatted audio data and converts it to PCM. It also extracts the looping information if used.
void decodeSound(unsigned char* iData, short** oData, int soundSize, int* loopStart, int* loopLength)
{
    unsigned char sp = 0;
    int sd = 0;
    double vagPrev1 = 0.0;
    double vagPrev2 = 0.0;
    int k = 0;

    for (int i=0; i<soundSize; i++)
    {
        if (i % 16 == 0)
        {
            sp = iData[i];
            if ((iData[i + 1] & 0x0E) == 6) (*loopStart) = k;
            if ((iData[i + 1] & 0x0F) == 3 || (iData[i + 1] & 0x0F) == 7) (*loopLength) = (k + 28) - (*loopStart);
            i += 2;
        }
        sd = (int)iData[i] & 0x0F;
        (*oData)[k++] = vagToPcm(sp, sd, &vagPrev1, &vagPrev2);
        sd = ((int)iData[i] >> 4) & 0x0F;
        (*oData)[k++] = vagToPcm(sp, sd, &vagPrev1, &vagPrev2);
    }
}

// Get the path without the file extension
void getFilepath(char* in, char** out)
{
    if (strlen(in) == 0)  // in is empty - create empty string and return
    {
        *out = emalloc(1);
        (*out)[0] = '\0';
        return;
    }

    int size;

    // Standardize directory separators
    char* pos = strchr(in, '/');
    while (pos)
    {
        *pos = '\\';
        pos = strchr(pos+1, '/');
    }

    char* pathbreak = strrchr(in, '\\');
    if (!pathbreak) pathbreak = in-1;  // No path before filename

    if (pathbreak+1 == in+strlen(in))  // No filename - exit with error
    {
        fprintf(stderr, "Error: The specified file name only contains a path.\n");
        exit(EXIT_FAILURE);
    }
    else  // Create a copy of the file path without its extension
    {
        char* extbreak = strrchr(in, '.');
        if (!extbreak || extbreak < pathbreak) size = strlen(in);  // File has no extension
        else size = extbreak - in;
        *out = emalloc(size + 1);
        strncpy(*out, in, size);
        (*out)[size] = '\0';
    }

    return;
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("Usage: driver-sfx-extract <filename>, or drag & drop the file onto the exe.\n");
        return 1;
    }
    else if (argc > 2)
    {
        printf("Error: Too many arguments.\n");
        return -1;
    }
    char* outputPath;
    getFilepath(argv[1], &outputPath);

    int* bankList;
    int numSoundbanks;
    int soundsExtracted = 0;

    FILE* fp1;
    fp1 = fopen(argv[1], "rb");
    if (!fp1)
    {
        printf("Unable to open file.");
        free(outputPath);
        return -1;
    }
    fread(&numSoundbanks, sizeof(int), 1, fp1);  // Read the number of sound banks
    numSoundbanks >>= 2;
    bankList = emalloc(numSoundbanks * 4);
    fseek(fp1, 0, SEEK_SET);
    fread(bankList, sizeof(int), numSoundbanks, fp1);  // Read the offset list
    numSoundbanks--;  // The last entry points to the end of the file.

    CreateDirectoryA(outputPath, NULL);  // Create the main directory

    for (int i=0; i<numSoundbanks; i++)
    {
        int numSounds;
        int startOffset;
        int* soundList;

        fseek(fp1, bankList[i], SEEK_SET);
        fread(&numSounds, sizeof(int), 1, fp1);  // Read the number of sounds
        startOffset = bankList[i] + numSounds * 16 + 4;
        soundList = emalloc(numSounds * 16);
        fread(soundList, sizeof(int), numSounds * 4, fp1);  // Read the list entries

        char bankPath[256];
        snprintf(bankPath, sizeof(bankPath), "%s\\Bank %d", outputPath, i);
        CreateDirectoryA(bankPath, NULL);  // Create the sound bank directory

        for (int k=0; k<numSounds; k++) // Yes I use k before j in nested loops. It's a weird habit of mine, don't judge me.
        {
            int soundSize = soundList[(k << 2) + 1];
            if (soundList[(k << 2) + 2] == 0) soundSize -= 16;  // One-shot sounds have a "silent"  loop block at the end which should be discarded.
                                                                // (By definition PSX ADPCM encoded data should also have a 16-byte zero padding at the beginning which doesn't exist in some cases - including the Driver games.)
            if (soundSize == 0) continue;  // No audio data - skip to next sound. NOTE: Unused entries may still have data which decodes to silence.
            int numSamples = (soundSize >> 4) * 28;  // PSX ADPCM data is stored in blocks of 16 bytes each containing 28 samples.
            unsigned char* iData = emalloc(soundSize);
            short* oData = emalloc(numSamples * sizeof(short));
            int loopStart = 0;
            int loopLength = 0;

            fseek(fp1, startOffset + soundList[k << 2], SEEK_SET);
            fread(iData, sizeof(char), soundSize, fp1);  // Read the audio data

            decodeSound(iData, &oData, soundSize, &loopStart, &loopLength);  // Convert the audio data

            if (soundSize > 0)
            {
                // Prepare the WAV file header
                wavHeader[1] = numSamples * 2 + 36;  // Size of the "RIFF" chunk
                if (loopLength > 0) wavHeader[1] += 68;
                wavHeader[6] = soundList[(k << 2) + 3];  // Sampling rate
                wavHeader[7] = wavHeader[6] * 2;  // Average bytes per second
                wavHeader[10] = numSamples * 2;  // Size of the "data" chunk

                char outputFile[256];
                snprintf(outputFile, sizeof(outputFile), "%s\\Bank %d\\%d.wav", outputPath, i, k);
                FILE* fp2;
                fp2 = fopen(outputFile, "wb");
                if (!fp2)
                {
                    printf("Unable to open file.");
                    free(iData);
                    free(oData);
                    free(bankList);
                    free(outputPath);
                    return -1;
                }

                // Write WAV file header, the converted audio data, and the loop positions if used
                fwrite(&wavHeader, sizeof(int), 11, fp2);
                fwrite(oData, sizeof(short), numSamples, fp2);
                if (loopLength > 0)
                {
                    smplChunk[13] = loopStart;
                    smplChunk[14] = loopStart + loopLength;
                    fwrite(&smplChunk, sizeof(int), 17, fp2);
                }
                fclose(fp2);
                free(iData);
                free(oData);
                soundsExtracted++;
            }
        }
        free(soundList);
    }
    free(bankList);
    free(outputPath);

    printf("Extracted %d sounds from %d sound banks.\n", soundsExtracted, numSoundbanks);
    return 0;
}
