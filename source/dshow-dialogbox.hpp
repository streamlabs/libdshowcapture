#pragma once

#include "../dshowcapture.hpp"
#include "dshow-base.hpp"

#include <mutex>

namespace DShow {
    struct DeviceDialogBox {
        DeviceDialogBox();
        ~DeviceDialogBox();

        void Open(IUnknown* filter);
        void Close();
        DWORD Create();

        static DWORD WINAPI CallCreate(void* param) {
            DeviceDialogBox* obj = (DeviceDialogBox*) param;
            return obj->Create();
        }

        std::mutex mutex;
        ComPtr<IUnknown> deviceFilter;
        HANDLE threadHandle;
    };
};
