#include "pch.h"
#include "BasicTask.h"

CBasicTask::CBasicTask()
: m_hThread(NULL),
  m_bResult(FALSE),
  m_bCanceled(FALSE)
{
    m_hCanceledEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CBasicTask::~CBasicTask()
{
    if (m_hThread != NULL)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    CloseHandle(m_hCanceledEvent);
    m_hCanceledEvent = NULL;
}

BOOL CBasicTask::Start()
{
    if (m_hThread != NULL)
    {
        //
        // Try to reset the task.  If the task is currently running,
        // then this call will fail, and m_hThread will not be set to NULL.
        //
        Reset();
    }

    if (m_hThread == NULL)
    {
        m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ThreadProc, (LPVOID)this, 0, NULL);

        if (m_hThread != NULL)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL CBasicTask::Cancel(BOOL waitUntilFinished)
{
    m_bCanceled = TRUE;
    SetEvent(m_hCanceledEvent);

    if (waitUntilFinished)
    {
        if (m_hThread != NULL)
        {
            WaitForSingleObject(m_hThread, INFINITE);            
        }
    }

    return TRUE;
}

BOOL CBasicTask::HasStarted()
{
    return (m_hThread != NULL);
}

BOOL CBasicTask::IsRunning()
{
    if (m_hThread != NULL)
    {
        DWORD r = WaitForSingleObject(m_hThread, 0);

        if (r == WAIT_TIMEOUT)
        {
            return TRUE; //the task thread is still running
        }
        else if (r == WAIT_OBJECT_0)
        {
            return FALSE; //the thread completed normally
        }
        else //WAIT_FAILED - unlikely to get this
        {
            return FALSE;
        }
    }

    return FALSE;
}

BOOL CBasicTask::WaitUntilFinished(BOOL& taskResult)
{
    taskResult = FALSE;

    if (m_hThread != NULL)
    {
        WaitForSingleObject(m_hThread, INFINITE);
        taskResult = m_bResult;

        return TRUE;
    }

    return FALSE;
}

BOOL CBasicTask::GetResult(BOOL& taskResult)
{
    if (!HasStarted() || IsRunning())
    {
        taskResult = FALSE;
        return FALSE;
    }
    else
    {
        taskResult = m_bResult;

        return TRUE;
    }
}

BOOL CBasicTask::Reset()
{
    if (m_hThread != NULL)
    {
        if (WaitForSingleObject(m_hThread, 0) != WAIT_OBJECT_0)
        {
            return FALSE; //we can't, the task is running
        }

        CloseHandle(m_hThread);
    }

    m_hThread = NULL;
    m_bResult = FALSE;
    m_bCanceled = FALSE;
    ResetEvent(m_hCanceledEvent);

    ResetTaskData();

    return TRUE;
}

DWORD WINAPI CBasicTask::ThreadProc(LPVOID Parameter)
{
    CBasicTask* pTask = (CBasicTask*)Parameter;

    pTask->m_bResult = pTask->Task();

    return (pTask->m_bResult == TRUE);
}

BOOL CBasicTask::Task()
{
    return TRUE;
}