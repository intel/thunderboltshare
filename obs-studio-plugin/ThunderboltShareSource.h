#pragma once

#include <obs-module.h>

#include "BasicTask.h"

class CThunderboltShareSource
{
public:
    CThunderboltShareSource(obs_source_t* source, obs_data_t* settings);
    virtual ~CThunderboltShareSource();

    void Update(obs_data_t* settings);

    void Show();
    void Hide();

private:
    class CVideoTask : virtual public CBasicTask
    {
    public:
        CVideoTask(CThunderboltShareSource* source);
        virtual ~CVideoTask();

        virtual BOOL Task();        

    private:        
        CThunderboltShareSource* m_pSource;
    };
    CVideoTask m_VideoTask;

    obs_source_t* m_pObsSource = NULL;
    obs_data_t* m_pObsSettings = NULL;

    volatile uint32_t m_MonitorIndex = 0;
    volatile uint32_t m_ColorDepth = 24;
    volatile uint32_t m_FpsLimit = 0;
    volatile BOOL m_bAudio = FALSE;
    volatile int64_t m_AudioDelayMS = 0;

    void ClearObsImage();

    void UpdateMemberVariables(obs_data_t* settings);
};

