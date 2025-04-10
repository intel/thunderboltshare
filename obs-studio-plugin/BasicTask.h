#pragma once

class CBasicTask 
{
public:
    CBasicTask();
    virtual ~CBasicTask();

    virtual BOOL Start();

    virtual BOOL Cancel(BOOL waitUntilFinished);

    virtual BOOL IsCanceled() { return m_bCanceled; }

    virtual BOOL HasStarted();

    virtual BOOL IsRunning();

    virtual BOOL WaitUntilFinished(BOOL& taskResult);

    virtual BOOL GetResult(BOOL& taskResult);

    virtual BOOL Reset();

protected:

    virtual HANDLE GetCanceledEvent() { return m_hCanceledEvent; }

    //
    // Derived classes which maintain additional data
    // should implement this, and reset their additional data here
    //
    virtual VOID ResetTaskData() {}

    HANDLE m_hThread;

    static DWORD WINAPI ThreadProc(LPVOID Parameter);

    virtual BOOL Task();

    volatile BOOL m_bResult;

private:
    volatile BOOL m_bCanceled;

    HANDLE m_hCanceledEvent;
};

