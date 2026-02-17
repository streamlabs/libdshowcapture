#include "dshow-dialogbox.hpp"
#include "log.hpp"

namespace DShow {
    DeviceDialogBox::DeviceDialogBox() :
        threadHandle(nullptr)
    {
    }

    DeviceDialogBox::~DeviceDialogBox()
    {
        Close();
    }

    void DeviceDialogBox::Open(IUnknown* filter) {
        std::lock_guard<std::mutex> lock(mutex);

        // Cleaning up a previously finished dialog thread, if any
        if (threadHandle &&
            WaitForSingleObject(threadHandle, 0) == WAIT_OBJECT_0) {
            CloseHandle(threadHandle);
            threadHandle = nullptr;
        }

        if (threadHandle) {
            return;
        }

        deviceFilter = filter;
        threadHandle = CreateThread(nullptr, 0, CallCreate, (void*) this, 0, nullptr);

        if (threadHandle == nullptr) {
            deviceFilter.Clear();
            Warning(L"Could not create thread for filter dialog box");
        }
    }

    void DeviceDialogBox::Close() {
        HANDLE handle = nullptr;
        DWORD tid = 0;

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!threadHandle) {
                deviceFilter.Clear();
                return;
            }

            handle = threadHandle;
            tid = GetThreadId(handle);
            if (!tid) {
                // Note: it is not a fatal error overall, and, if threadHandle was valid, it will not fail,
                // so no early return here
                DWORD err = GetLastError();
                Warning(L"Could not get thread id for filter dialog box (error %lu)", err);
            }
        }

        if (tid) {
            PostThreadMessage(tid, WM_QUIT, 0, 0);
        }

        WaitForSingleObject(handle, INFINITE);

        {
            std::lock_guard<std::mutex> lock(mutex);
            if (threadHandle == handle) {
                threadHandle = nullptr;
                deviceFilter.Clear();
            }
        }

        // Always close the handle we waited on, even if a newer thread started
        // while waiting, to avoid leaking the old handle.
        CloseHandle(handle);
    }

    DWORD DeviceDialogBox::Create() {
        ComPtr<IUnknown> filter;
        {
            std::lock_guard<std::mutex> lock(mutex);
            filter = deviceFilter;
        }

        ComQIPtr<ISpecifyPropertyPages> pages(filter.Get());
        CAUUID cauuid;

        if (pages != nullptr) {
            if (SUCCEEDED(pages->GetPages(&cauuid)) && cauuid.cElems) {
                IUnknown* objects[] = {filter.Get()};
                OleCreatePropertyFrame(nullptr, 0, 0, nullptr, 1,
                                       objects,
                                       cauuid.cElems, cauuid.pElems, 0,
                                       0, nullptr);
                CoTaskMemFree(cauuid.pElems);
            }
            pages.Clear();
        }
        {
            std::lock_guard<std::mutex> lock(mutex);
            deviceFilter.Clear();
        }

        return 0;
    }
}
