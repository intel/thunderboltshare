#include "pch.h"
#include "ThunderboltShareSource.h"
#include "SourceProperties.h"
#include "Util.h"

#include <mmreg.h>

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CThunderboltShareSource::CThunderboltShareSource(obs_source_t* source, obs_data_t* settings)
: m_pObsSource(source),
  m_pObsSettings(settings),
  m_VideoTask(this)
{
    UpdateMemberVariables(settings);
}

CThunderboltShareSource::~CThunderboltShareSource()
{
    m_VideoTask.Cancel(TRUE);
}

void CThunderboltShareSource::UpdateMemberVariables(obs_data_t* settings)
{
    m_MonitorIndex = obs_data_get_uint32(settings, S_MONITOR);
    m_ColorDepth = obs_data_get_uint32(settings, S_COLOR_DEPTH);
    m_FpsLimit = obs_data_get_uint32(settings, S_FPS_LIMIT);
    m_bAudio = obs_data_get_bool(settings, S_AUDIO);
    m_AudioDelayMS = obs_data_get_int(settings, S_AUDIO_DELAY);

    m_pObsSettings = settings;
}

void CThunderboltShareSource::Update(obs_data_t* settings)
{
    UpdateMemberVariables(settings);    

    ClearObsImage();

    Hide();
    Show();
}

void CThunderboltShareSource::Show()
{
    if (!m_VideoTask.IsRunning())
    {
        m_VideoTask.Start();
    }
}

void CThunderboltShareSource::Hide()
{
    m_VideoTask.Cancel(TRUE);
}

void CThunderboltShareSource::ClearObsImage()
{
    //
    // Clear the video image
    //
    if (m_pObsSource != NULL)
    {
        obs_source_output_video(m_pObsSource, NULL);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define OBS_SERVER_PIPE_NAME L"\\\\.\\pipe\\ThunderboltShareObsServer"

struct ObsPluginRequest
{
    UINT32 m_MonitorIndex = 0;
    UINT32 m_ColorDepth = 24;
    UINT32 m_FpsLimit = 0;
    BOOL m_bAudio = FALSE;
};

enum TBTSHARE_FRAME_HEADER_TYPE
{
    TFHT_NONE = 0,
    THFT_DISPLAY = 1,
    THFT_AUDIO = 2,
};

struct CThunberboltShareDisplayFrameHeader
{
    UINT32 m_FrameType = THFT_DISPLAY;
    UINT32 m_BufferByteLength = 0;
    UINT32 m_Width = 0;
    UINT32 m_Height = 0;
    UINT32 m_BitsPerPixel = 0;
    UINT64 m_TimestampNS = 0;
};

struct CThunberboltShareAudioFrameHeader
{
    UINT32 m_FrameType = THFT_AUDIO;
    UINT32 m_BufferByteLength = 0;
    WAVEFORMATEXTENSIBLE m_Format = {};
    DWORD m_Flags = 0;
    UINT64 m_DevicePosition = 0;
    UINT64 m_QPCPosition = 0;
};

union CThunberboltShareFrameHeader
{
    CThunberboltShareFrameHeader() { memset(this, 0, sizeof(CThunberboltShareFrameHeader)); }

    CThunberboltShareDisplayFrameHeader m_DisplayFrameHeader;
    CThunberboltShareAudioFrameHeader m_AudioFrameHeader;
};

CThunderboltShareSource::CVideoTask::CVideoTask(CThunderboltShareSource* source)
: m_pSource(source)
{
}

CThunderboltShareSource::CVideoTask::~CVideoTask()
{
}

BOOL CThunderboltShareSource::CVideoTask::Task()
{
    static UINT64 frameCount = 0;

    HANDLE hPipe = NULL;

    LogInfo("CVideoTask started");

    CByteArray mainBuffer(4 * 1024 * 1024);
    CByteArray audioBuffer(100 * 1024);

    while (!IsCanceled())
    {
        //
        // If we are not connected to the pipe server, try to connect to it
        //
        if (hPipe == NULL)
        {
            //
            // Try to connect a pipe to m_PipeName
            //
            hPipe = CreateFile(
                OBS_SERVER_PIPE_NAME,               // pipe name 
                GENERIC_READ | GENERIC_WRITE,       // read and write access             
                FILE_SHARE_READ | FILE_SHARE_WRITE, // no sharing 
                NULL,                               // default security attributes
                OPEN_EXISTING,                      // opens existing pipe 
                FILE_ATTRIBUTE_NORMAL,              // default attributes 
                NULL);                              // no template file 

            //
            // The CreateFile function can fail for various reasons
            //
            if (hPipe == INVALID_HANDLE_VALUE)
            {
                hPipe = NULL;              

                WaitNamedPipe(OBS_SERVER_PIPE_NAME, 250);
                WaitForSingleObject(GetCanceledEvent(), 350);
                continue;
            }

            //
            // We must write the requested monitor and color depth first
            //
            ObsPluginRequest request;
            request.m_MonitorIndex = m_pSource->m_MonitorIndex;
            request.m_ColorDepth = m_pSource->m_ColorDepth;
            request.m_FpsLimit = m_pSource->m_FpsLimit;
            request.m_bAudio = m_pSource->m_bAudio;

            DWORD bytesWritten = 0;

            if (!WriteFile(hPipe, &request, sizeof(request), &bytesWritten, NULL))
            {
                CloseHandle(hPipe);
                hPipe = NULL;
                continue;
            }

            LogInfo("CVideoTask connected to Thunderbolt Share application");
        }

        //
        // Try to read the frame header information
        //
        CThunberboltShareFrameHeader frameHeader = {};

        UINT32 bytesRead = 0;

        if (!CFileUtil::ReadEntireBuffer(hPipe, (BYTE*)(void*)&frameHeader, sizeof(frameHeader), bytesRead, GetCanceledEvent()))
        {
            CloseHandle(hPipe);
            hPipe = NULL;
            continue;
        };

        if (frameHeader.m_DisplayFrameHeader.m_FrameType == THFT_DISPLAY)
        {
            if (frameHeader.m_DisplayFrameHeader.m_BufferByteLength > 0)
            {
                //
                // Try to read the bitmap image
                //
                mainBuffer.Reallocate(frameHeader.m_DisplayFrameHeader.m_BufferByteLength);

                if (!CFileUtil::ReadEntireBuffer(hPipe, mainBuffer.Buffer, frameHeader.m_DisplayFrameHeader.m_BufferByteLength, bytesRead, GetCanceledEvent()))
                {
                    CloseHandle(hPipe);
                    hPipe = NULL;
                    continue;
                }

                //
                // Show the image
                //
                struct obs_source_frame frame = {};

                //frame.timestamp = frameCount++;
                frame.timestamp = frameHeader.m_DisplayFrameHeader.m_TimestampNS;
                frame.width = frameHeader.m_DisplayFrameHeader.m_Width;
                frame.height = frameHeader.m_DisplayFrameHeader.m_Height;
                frame.linesize[0] = CBitmapImage::ComputeImageRowSize(frameHeader.m_DisplayFrameHeader.m_Width, frameHeader.m_DisplayFrameHeader.m_BitsPerPixel);
                frame.data[0] = mainBuffer.Buffer;
                frame.format = VIDEO_FORMAT_BGRX;

                obs_source_output_video(m_pSource->m_pObsSource, &frame);
            }            
        }
        else if (frameHeader.m_AudioFrameHeader.m_FrameType == THFT_AUDIO)
        {
            //
            // Try to read the audio buffer
            //
            audioBuffer.Reallocate(frameHeader.m_AudioFrameHeader.m_BufferByteLength);

            if (!CFileUtil::ReadEntireBuffer(hPipe, audioBuffer.Buffer, frameHeader.m_AudioFrameHeader.m_BufferByteLength, bytesRead, GetCanceledEvent()))
            {
                CloseHandle(hPipe);
                hPipe = NULL;
                continue;
            }

            //
            // Play the audio buffer
            //
            UINT32 numFrames = (UINT32)(audioBuffer.Length / frameHeader.m_AudioFrameHeader.m_Format.Format.nBlockAlign);

            obs_source_audio data = {};
            data.data[0] = audioBuffer.Buffer;
            data.frames = numFrames;
            data.speakers = SPEAKERS_STEREO;
            data.samples_per_sec = frameHeader.m_AudioFrameHeader.m_Format.Format.nSamplesPerSec;
            data.format = AUDIO_FORMAT_FLOAT;

            INT64 audioDelay = m_pSource->m_AudioDelayMS * 1000 * 1000;
            data.timestamp = frameHeader.m_AudioFrameHeader.m_QPCPosition * 100 - audioDelay;

            obs_source_output_audio(m_pSource->m_pObsSource, &data);            
        }
    }

    CloseHandle(hPipe);

    //
    // Clear the video image
    //
    m_pSource->ClearObsImage();

    LogInfo("CVideoTask stopped");

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
